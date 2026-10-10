// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel accelerator
 *
 *  Copyright (C) 2019-2023 OpenVPN, Inc.
 *
 *  Author:	James Yonan <james@openvpn.net>
 *		Antonio Quartulli <antonio@openvpn.net>
 */

#include "main.h"
#include "bind.h"
#include "netlink.h"
#include "ovpn.h"
#include "sock.h"
#include "peer.h"
#include "stats.h"
#include "proto.h"
#include "crypto.h"
#include "crypto_aead.h"
#include "skb.h"
#include "tcp.h"
#include "udp.h"

#include <linux/workqueue.h>
#include <linux/smp.h>
#include <uapi/linux/if_ether.h>

static const unsigned char ovpn_keepalive_message[] = {
	0x2a, 0x18, 0x7b, 0xf3, 0x64, 0x1e, 0xb4, 0xcb,
	0x07, 0xed, 0x2d, 0x0a, 0x98, 0x1f, 0xc7, 0x48
};

static const unsigned char ovpn_explicit_exit_notify_message[] = {
	0x28, 0x7f, 0x34, 0x6b, 0xd4, 0xef, 0x7a, 0x81,
	0x2d, 0x56, 0xb8, 0xd3, 0xaf, 0xc5, 0x45, 0x9c,
	6 // OCC_EXIT
};

/* Is keepalive message?
 * Assumes that single byte at skb->data is defined.
 */
static bool ovpn_is_keepalive(struct sk_buff *skb)
{
	if (*skb->data != OVPN_KEEPALIVE_FIRST_BYTE)
		return false;

	if (!pskb_may_pull(skb, sizeof(ovpn_keepalive_message)))
		return false;

	return !memcmp(skb->data, ovpn_keepalive_message,
		       sizeof(ovpn_keepalive_message));
}

int ovpn_struct_init(struct net_device *dev)
{
	struct ovpn_struct *ovpn = netdev_priv(dev);
	int err;

	memset(ovpn, 0, sizeof(*ovpn));

	ovpn->dev = dev;

	err = ovpn_netlink_init(ovpn);
	if (err < 0)
		return err;

	spin_lock_init(&ovpn->lock);
	spin_lock_init(&ovpn->peers.lock);

	ovpn->crypto_wq = alloc_workqueue("ovpn-crypto-wq-%s",
					  WQ_CPU_INTENSIVE | WQ_MEM_RECLAIM, 0,
					  dev->name);
	if (!ovpn->crypto_wq)
		return -ENOMEM;

	ovpn->events_wq = alloc_workqueue("ovpn-event-wq-%s", WQ_MEM_RECLAIM, 0, dev->name);
	if (!ovpn->events_wq)
		return -ENOMEM;

	dev->tstats = netdev_alloc_pcpu_stats(struct pcpu_sw_netstats);
	if (!dev->tstats)
		return -ENOMEM;

	err = security_tun_dev_alloc_security(&ovpn->security);
	if (err < 0)
		return err;

	/* kernel -> userspace tun queue length */
	ovpn->max_tun_queue_len = OVPN_MAX_TUN_QUEUE_LEN;

	return 0;
}

/* Called after decrypt to write IP packet to tun netdev.
 * This method is expected to manage/free skb.
 */
static void tun_netdev_write(struct ovpn_peer *peer, struct sk_buff *skb)
{
	/* packet integrity was verified on the VPN layer - no need to perform
	 * any additional check along the stack
	 */
#ifndef OVPN_CHECKSUM_NONE
	skb->ip_summed = CHECKSUM_UNNECESSARY;
#else
	skb->ip_summed = CHECKSUM_NONE;
#endif
	skb->csum_level = ~0;

	/* skb hash for transport packet no longer valid after decapsulation */
	skb_clear_hash(skb);

	/* post-decrypt scrub -- prepare to inject encapsulated packet onto tun
	 * interface, based on __skb_tunnel_rx() in dst.h
	 */
	skb->dev = peer->ovpn->dev;
	skb_set_queue_mapping(skb, 0);
	skb_scrub_packet(skb, true);

	skb_reset_network_header(skb);
	skb_reset_transport_header(skb);
	skb_probe_transport_header(skb);
	skb_reset_inner_headers(skb);

	/* update per-cpu RX stats with the stored size of encrypted packet */

	/* we are in softirq context - hence no locking nor disable preemption needed */
	dev_sw_netstats_rx_add(peer->ovpn->dev, skb->len);

	/* cause packet to be "received" by tun interface */
	napi_gro_receive(&peer->napi, skb);
}

int ovpn_napi_poll(struct napi_struct *napi, int budget)
{
	struct ovpn_peer *peer = container_of(napi, struct ovpn_peer, napi);
	struct sk_buff *skb;
	int work_done = 0;
	enum packet_state state;
	bool free;

	if (unlikely(budget <= 0))
		return 0;
	/* this function should schedule at most 'budget' number of
	 * packets for delivery to the tun interface.
	 * If in the queue we have more packets than what allowed by the
	 * budget, the next polling will take care of those
	 */
	while ((skb = ovpn_prev_queue_peek(&peer->rx_queue)) != NULL &&
			(state = atomic_read_acquire(&PACKET_CB(skb)->state)) !=
				PACKET_STATE_UNCRYPTED) {
		ovpn_prev_queue_drop_peeked(&peer->rx_queue);
		free = true;

		if (unlikely(state == PACKET_STATE_KEEPALIVE)) {
			if (skb->next)
				consume_skb(skb->next);
			consume_skb(skb);
			free = false;
		}

		if (unlikely(state != PACKET_STATE_CRYPTED))
			goto next;

		if (skb->next) {
			tun_netdev_write(peer, skb->next);
			consume_skb(skb);
			free = false;
		}
		else {
			tun_netdev_write(peer, skb);
			free = false;
		}
next:
		if (unlikely(free)) {
			if (skb->next)
				kfree_skb(skb->next);
			kfree_skb(skb);
		}

		if (++work_done >= budget)
			break;
	}

	if (work_done < budget)
		napi_complete_done(napi, work_done);

	return work_done;
}

/* Entry point for processing an incoming packet (in skb form)
 *
 * Enqueue the packet and schedule RX consumer.
 * Reference to peer is dropped only in case of success.
 *
 * Return 0  if the packet was handled (and consumed)
 * Return <0 in case of error (return value is error code)
 */
int ovpn_recv(struct ovpn_struct *ovpn, struct ovpn_peer *peer, struct sk_buff *skb)
{
	int cpu;
#ifdef OVPN_CRYPTO_USING_THREE_CORES	/* for qual CPU Socs. */
	int cpu_recv = smp_processor_id();
#endif

	atomic_set_release(&PACKET_CB(skb)->state, PACKET_STATE_UNCRYPTED);
	/* We first queue this up for the peer ingestion, but the consumer
	 * will wait for the state to change to CRYPTED or DEAD before.
	 */
	if (unlikely(!ovpn_prev_queue_enqueue(&peer->rx_queue, skb)))
		return -ENOSPC;

	/* Then we queue it up in the device queue, which consumes the
	 * packet as soon as it can.
	 */
	cpu = ovpn_cpumask_next_online(&peer->decrypt_queue.last_cpu);
#ifdef OVPN_CRYPTO_USING_THREE_CORES	/* for qual CPU Socs. */
	if (cpu == cpu_recv)
		cpu = ovpn_cpumask_next_online(&peer->decrypt_queue.last_cpu);
#endif
	if (unlikely(ptr_ring_produce_bh(&peer->decrypt_queue.ring, skb))) {
		// net_err_ratelimited("%s: cannot handle incoming packet from peer %d: %d\n",
		// 		    __func__, peer->id, -EPIPE);
		atomic_set_release(&PACKET_CB(skb)->state, PACKET_STATE_DEAD);
		/* Because this skb has been enqueued in rx_queue, free it by napi */
		local_bh_disable();
		napi_schedule(&peer->napi);
		local_bh_enable();

		ovpn_peer_put(peer);
		return 0;
	}

	if (!queue_work_on(cpu, ovpn->crypto_wq, &per_cpu_ptr(peer->decrypt_queue.worker, cpu)->work))
		ovpn_peer_put(peer);

	return 0;
}

static int ovpn_decrypt_one(struct ovpn_peer *peer, struct sk_buff *skb)
{
	struct ovpn_peer *allowed_peer = NULL;
	struct sk_buff *skb_used = NULL;
	struct ovpn_crypto_key_slot *ks;
	__be16 proto;
	int ret = -1;
	u8 key_id;

	ovpn_peer_stats_increment_rx(&peer->link_stats, skb->len);

	/* get the key slot matching the key Id in the received packet */
	key_id = ovpn_key_id_from_skb(skb);
	ks = ovpn_crypto_key_id_to_slot(&peer->crypto, key_id);
	if (unlikely(!ks)) {
		net_info_ratelimited("%s: no available key for peer %u, key-id: %u\n", __func__,
				    peer->id, key_id);
		goto drop;
	}

	/* decrypt */
	ret = ovpn_aead_decrypt(ks, skb);

	ovpn_crypto_key_slot_put(ks);

	if (unlikely(ret < 0)) {
		net_err_ratelimited("%s: error during decryption for peer %u, key-id %u: %d\n",
				   __func__, peer->id, key_id, ret);
		goto drop;
	}

	if (peer->compress.comp_enabled)
	{
		ret = ovpn_decompress(skb, peer->compress.comp_alg);
		if (unlikely(ret < 0)) {
			net_err_ratelimited("%s: error during decompress for peer %u\n",
						__func__, peer->id);
			goto drop;
		}
	}

	if (skb->next)
		skb_used = skb->next;
	else
		skb_used = skb;

	/* note event of authenticated packet received for keepalive */
	ovpn_peer_keepalive_recv_reset(peer);

	/* update source and destination endpoint for this peer */
	if (peer->sock->sock->sk->sk_protocol == IPPROTO_UDP)
		ovpn_peer_update_local_endpoint(peer, skb_used);

	/* increment RX stats */
	ovpn_peer_stats_increment_rx(&peer->vpn_stats, skb_used->len);

	/* check if this is a valid datapacket that has to be delivered to the
	 * tun interface
	 */
	skb_reset_network_header(skb_used);
	proto = ovpn_ip_check_protocol(skb_used);
	if (unlikely(!proto)) {
		/* check if null packet */
		if (unlikely(!pskb_may_pull(skb_used, 1))) {
			ret = -EINVAL;
			goto drop;
		}

		/* check if special OpenVPN message */
		if (ovpn_is_keepalive(skb_used)) {
			netdev_dbg(peer->ovpn->dev, "%s: ping received from peer with id %u\n",
				   __func__, peer->id);
			/* not an error */
			/* consume_skb(skb); */
			/* inform the caller that NAPI should not be scheduled
			 * for this packet
			 */
			return 1;
		}

		ret = -EPROTONOSUPPORT;
		goto drop;
	}
	skb_used->protocol = proto;

	/* perform Reverse Path Filtering (RPF) */
	allowed_peer = ovpn_peer_lookup_vpn_addr(peer->ovpn, skb_used, true);
	if (unlikely(allowed_peer != peer)) {
		ret = -EPERM;
		goto drop;
	}

	/* ret = ptr_ring_produce_bh(&peer->netif_rx_ring, skb); */
drop:
	if (likely(allowed_peer))
		ovpn_peer_put(allowed_peer);

	/* if (unlikely(ret < 0)) */
		/* kfree_skb(skb); */

	return ret;
}

/* pick packet from RX queue, decrypt and forward it to the tun device */
void ovpn_decrypt_work(struct work_struct *work)
{
	struct ovpn_peer *peer;
	struct sk_buff *skb;
	struct crypt_queue *queue;

	queue = container_of(work, struct multicore_worker, work)->ptr;
	peer = container_of(queue, struct ovpn_peer, decrypt_queue);
	while ((skb = ptr_ring_consume_bh(&queue->ring))) {
		enum packet_state state;

		int ret = ovpn_decrypt_one(peer, skb);
		if (ret == 0) {
			state = PACKET_STATE_CRYPTED;
		} else if (ret > 0) {
			state = PACKET_STATE_KEEPALIVE;
		} else {
			state = PACKET_STATE_DEAD;
		}

		atomic_set_release(&PACKET_CB(skb)->state, state);
		/* if a packet has been enqueued for NAPI, signal
		 * availability to the networking stack.
		 */
		local_bh_disable();
		napi_schedule(&peer->napi);
		local_bh_enable();

		/* give a chance to be rescheduled if needed */
		cond_resched();
	}
	ovpn_peer_put(peer);
}

void ovpn_packet_tx_worker(struct work_struct *work)
{
	struct ovpn_peer *peer = container_of(work, struct ovpn_peer, transmit_packet_work);
	enum packet_state state;
	struct sk_buff *skb, *curr = NULL, *next = NULL;

	while ((skb = ovpn_prev_queue_peek(&peer->tx_queue)) != NULL &&
		 	(state = atomic_read_acquire(&PACKET_CB(skb)->state)) !=
		 		PACKET_STATE_UNCRYPTED) {
		ovpn_prev_queue_drop_peeked(&peer->tx_queue);

		if (likely(state == PACKET_STATE_CRYPTED)) {
			skb_list_walk_safe(skb, curr, next) {
				skb_mark_not_on_list(curr);

				switch (peer->sock->sock->sk->sk_protocol) {
				case IPPROTO_UDP:
					ovpn_udp_send_skb(peer->ovpn, peer, curr);
					break;
				case IPPROTO_TCP:
					ovpn_tcp_send_skb(peer, curr);
					break;
				default:
					/* no transport configured yet */
					consume_skb(skb);
					break;
				}
			}
			/* note event of authenticated packet xmit for keepalive */
			ovpn_peer_keepalive_xmit_reset(peer);
		}else {
			kfree_skb_list(skb);
		}
		/* give a chance to be rescheduled if needed */
		if (need_resched())
			cond_resched();
	}
}


static bool ovpn_encrypt_one(struct ovpn_peer *peer, struct sk_buff *skb,
								struct ovpn_crypto_key_slot *ks)
{
	bool success = false;
	int ret;

	if (unlikely(skb->ip_summed == CHECKSUM_PARTIAL &&
		     skb_checksum_help(skb))) {
		net_err_ratelimited("%s: cannot compute checksum for outgoing packet\n", __func__);
		goto err;
	}

	ovpn_peer_stats_increment_tx(&peer->vpn_stats, skb->len);

	if (peer->compress.comp_enabled)
	{
		ret = ovpn_compress(skb, peer->compress.comp_alg);
		if (unlikely(ret < 0)) {
			net_err_ratelimited("%s: error during compress for peer %u\n",
						__func__, peer->id);
			goto err;
		}
	}

	/* encrypt */
	ret = ovpn_aead_encrypt(ks, skb, peer->id);
	if (unlikely(ret < 0)) {
		/* if we ran out of IVs we must kill the key as it can't be used anymore */
		if (ret == -ERANGE) {
			netdev_warn(peer->ovpn->dev,
				    "%s: killing primary key as we ran out of IVs\n", __func__);
			ovpn_crypto_kill_primary(&peer->crypto);
			goto err;
		}
		net_err_ratelimited("%s: error during encryption for peer %u, key-id %u: %d\n",
				   __func__, peer->id, ks->key_id, ret);
		goto err;
	}

	success = true;

	ovpn_peer_stats_increment_tx(&peer->link_stats, skb->len);
err:
	return success;
}

/* Process packets in TX queue in a transport-specific way.
 *
 * UDP transport - encrypt and send across the tunnel.
 * TCP transport - encrypt and put into TCP TX queue.
 */
void ovpn_encrypt_work(struct work_struct *work)
{
	struct sk_buff *skb, *curr = NULL, *next = NULL;
	struct ovpn_peer *peer;
	struct crypt_queue *queue;

	queue = container_of(work, struct multicore_worker, work)->ptr;
	peer = container_of(queue, struct ovpn_peer, encrypt_queue);
	while ((skb = ptr_ring_consume_bh(&queue->ring))) {
		enum packet_state state = PACKET_STATE_CRYPTED;
		int cpu;
		struct ovpn_crypto_key_slot *ks = PACKET_CB(skb)->ks;
		/* this might be a GSO-segmented skb list: process each skb
		 * independently
		 */
		skb_list_walk_safe(skb, curr, next) {
			/* if one segment fails encryption, we drop the entire
			 * packet, because it does not really make sense to send
			 * only part of it at this point
			 */
			if (unlikely(!ovpn_encrypt_one(peer, curr, ks))) {
				state = PACKET_STATE_DEAD;
				break;
			}
		}

		atomic_set_release(&PACKET_CB(skb)->state, state);

		cpu = ovpn_cpumask_choose_online(&peer->serial_work_cpu, peer->id);
		queue_work_on(cpu, peer->ovpn->crypto_wq, &peer->transmit_packet_work);

		ovpn_crypto_key_slot_put(ks);
	}
	ovpn_peer_put(peer);
}

/* Put skb into TX queue and schedule a consumer */
static void ovpn_queue_skb(struct ovpn_struct *ovpn, struct sk_buff *skb, struct ovpn_peer *peer)
{
	int ret, cpu;
#ifdef OVPN_CRYPTO_USING_THREE_CORES	/* for qual CPU Socs. */
	int cpu_xmit;
#endif
	u32 pktid;
	struct ovpn_crypto_key_slot *ks = NULL;
	struct sk_buff *curr = NULL, *next = NULL;

	if (likely(!peer))
		peer = ovpn_peer_lookup_vpn_addr(ovpn, skb, false);
	if (unlikely(!peer)) {
		net_dbg_ratelimited("%s: no peer to send data to\n", ovpn->dev->name);
		goto drop;
	}

	atomic_set_release(&PACKET_CB(skb)->state, PACKET_STATE_UNCRYPTED);

	/* get primary key to be used for encrypting data */
	ks = ovpn_crypto_key_slot_primary(&peer->crypto);
	if (unlikely(!ks)) {
		net_warn_ratelimited("%s: error while retrieving primary key slot\n", __func__);
		goto drop;
	}

	skb_list_walk_safe(skb, curr, next) {
		ret = ovpn_pktid_xmit_next(&ks->pid_xmit, &pktid);
		if (unlikely(ret < 0)) {
			net_warn_ratelimited("%s: the 32bit space for packet id is over\n", __func__);
			break;
		}
		PACKET_CB(curr)->pkt_id = pktid;
	}
	if (unlikely(ret < 0))
		goto drop;

	PACKET_CB(skb)->ks = ks;

	/* We first queue this up for the peer ingestion, but the consumer
	 * will wait for the state to change to CRYPTED or DEAD before.
	 */
	if (unlikely(!ovpn_prev_queue_enqueue(&peer->tx_queue, skb)))
		goto drop;

	/* Then we queue it up in the device queue, which consumes the
	 * packet as soon as it can.
	 */
	cpu = ovpn_cpumask_next_online(&peer->encrypt_queue.last_cpu);
#ifdef OVPN_CRYPTO_USING_THREE_CORES	/* for qual CPU Socs. */
	cpu_xmit = ovpn_cpumask_choose_online(&peer->serial_work_cpu, peer->id);
	if (cpu == cpu_xmit)
		cpu = ovpn_cpumask_next_online(&peer->encrypt_queue.last_cpu);
#endif

	ret = ptr_ring_produce_bh(&peer->encrypt_queue.ring, skb);
	if (unlikely(ret < 0)) {
		// net_err_ratelimited("%s: cannot queue packet to TX ring\n", __func__);
		atomic_set_release(&PACKET_CB(skb)->state, PACKET_STATE_DEAD);
		/* Because this skb has been enqueued in tx_queue, free it by transmit_packet_work */
		cpu = ovpn_cpumask_choose_online(&peer->serial_work_cpu, peer->id);
		queue_work_on(cpu, peer->ovpn->crypto_wq, &peer->transmit_packet_work);

		ovpn_peer_put(peer);
		ovpn_crypto_key_slot_put(ks);
		return;
	}

	if (!queue_work_on(cpu, ovpn->crypto_wq, &per_cpu_ptr(peer->encrypt_queue.worker, cpu)->work))
		ovpn_peer_put(peer);

	return;
drop:
	if (peer)
		ovpn_peer_put(peer);
	if (ks)
		ovpn_crypto_key_slot_put(ks);
	kfree_skb_list(skb);
}

/* Net device start xmit
 */
netdev_tx_t ovpn_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ovpn_struct *ovpn = netdev_priv(dev);
	struct sk_buff *segments, *tmp, *curr = NULL, *next = NULL;
	struct sk_buff_head skb_list;
	__be16 proto;
	int ret;

	/* reset netfilter state */
	nf_reset_ct(skb);

	/* verify IP header size in network packet */
	skb_reset_network_header(skb);
	proto = ovpn_ip_check_protocol(skb);
	if (unlikely(!proto || skb->protocol != proto)) {
		net_err_ratelimited("%s: dropping malformed payload packet\n",
				    dev->name);
		goto drop;
	}

	if (skb_is_gso(skb)) {
		segments = skb_gso_segment(skb, 0);
		if (IS_ERR(segments)) {
			ret = PTR_ERR(segments);
			net_err_ratelimited("%s: cannot segment packet: %d\n", dev->name, ret);
			goto drop;
		}

		consume_skb(skb);
		skb = segments;
	}

	/* from this moment on, "skb" might be a list */

	__skb_queue_head_init(&skb_list);
	skb_list_walk_safe(skb, curr, next) {
		skb_mark_not_on_list(curr);

		tmp = skb_share_check(curr, GFP_ATOMIC);
		if (unlikely(!tmp)) {
			kfree_skb_list(next);
			net_err_ratelimited("%s: skb_share_check failed\n", dev->name);
			goto drop_list;
		}

		__skb_queue_tail(&skb_list, tmp);
	}
	skb_list.prev->next = NULL;

	ovpn_queue_skb(ovpn, skb_list.next, NULL);

	return NETDEV_TX_OK;

drop_list:
	skb_queue_walk_safe(&skb_list, curr, next)
		kfree_skb(curr);
drop:
	skb_tx_error(skb);
	kfree_skb_list(skb);
	return NET_XMIT_DROP;
}

/* Encrypt and transmit a special message to peer, such as keepalive
 * or explicit-exit-notify.  Called from softirq context.
 * Assumes that caller holds a reference to peer.
 */
static void ovpn_xmit_special(struct ovpn_peer *peer, const void *data,
			      const unsigned int len)
{
	struct ovpn_struct *ovpn;
	struct sk_buff *skb;

	ovpn = peer->ovpn;
	if (unlikely(!ovpn))
		return;

	skb = alloc_skb(256 + len, GFP_ATOMIC);
	if (unlikely(!skb))
		return;

	skb_reserve(skb, 128);
	skb->priority = TC_PRIO_BESTEFFORT;
	memcpy(__skb_put(skb, len), data, len);

	/* increase reference counter when passing peer to sending queue */
	if (!ovpn_peer_hold(peer)) {
		netdev_dbg(ovpn->dev, "%s: cannot hold peer reference for sending special packet\n",
			   __func__);
		kfree_skb(skb);
		return;
	}

	ovpn_queue_skb(ovpn, skb, peer);
}

void ovpn_keepalive_xmit(struct ovpn_peer *peer)
{
	ovpn_xmit_special(peer, ovpn_keepalive_message,
			  sizeof(ovpn_keepalive_message));
}

/* Transmit explicit exit notification.
 * Called from process context.
 */
void ovpn_explicit_exit_notify_xmit(struct ovpn_peer *peer)
{
	ovpn_xmit_special(peer, ovpn_explicit_exit_notify_message,
			  sizeof(ovpn_explicit_exit_notify_message));
}

/* Copy buffer into skb and send it across the tunnel.
 *
 * For UDP transport: just sent the skb to peer
 * For TCP transport: put skb into TX queue
 */
int ovpn_send_data(struct ovpn_struct *ovpn, u32 peer_id, const u8 *data, size_t len)
{
	u16 skb_len = SKB_HEADER_LEN + len;
	struct ovpn_peer *peer;
	struct sk_buff *skb;
	bool tcp = false;
	int ret = 0;

	peer = ovpn_peer_lookup_id(ovpn, peer_id);
	if (unlikely(!peer)) {
		netdev_dbg(ovpn->dev, "no peer to send data to\n");
		return -EHOSTUNREACH;
	}

	if (peer->sock->sock->sk->sk_protocol == IPPROTO_TCP) {
		skb_len += sizeof(u16);
		tcp = true;
	}

	skb = alloc_skb(skb_len, GFP_ATOMIC);
	if (unlikely(!skb)) {
		ret = -ENOMEM;
		goto out;
	}

	skb_reserve(skb, SKB_HEADER_LEN);
	skb_put_data(skb, data, len);

	/* prepend TCP packet with size, as required by OpenVPN protocol */
	if (tcp) {
		*(__be16 *)__skb_push(skb, sizeof(u16)) = htons(len);
		ovpn_queue_tcp_skb(peer, skb);
	} else {
		ovpn_udp_send_skb(ovpn, peer, skb);
	}
out:
	ovpn_peer_put(peer);
	return ret;
}

#define NEXT(skb) ((skb)->prev)
#define STUB(queue) ((struct sk_buff *)&queue->empty)

void ovpn_prev_queue_init(struct prev_queue *queue)
{
	NEXT(STUB(queue)) = NULL;
	queue->head = queue->tail = STUB(queue);
	queue->peeked = NULL;
	atomic_set(&queue->count, 0);
	BUILD_BUG_ON(
		offsetof(struct sk_buff, next) != offsetof(struct prev_queue, empty.next) -
							offsetof(struct prev_queue, empty) ||
		offsetof(struct sk_buff, prev) != offsetof(struct prev_queue, empty.prev) -
							 offsetof(struct prev_queue, empty));
}

static void __ovpn_prev_queue_enqueue(struct prev_queue *queue, struct sk_buff *skb)
{
	WRITE_ONCE(NEXT(skb), NULL);
	WRITE_ONCE(NEXT(xchg_release(&queue->head, skb)), skb);
}

bool ovpn_prev_queue_enqueue(struct prev_queue *queue, struct sk_buff *skb)
{
	if (!atomic_add_unless(&queue->count, 1, MAX_QUEUED_PACKETS))
		return false;
	__ovpn_prev_queue_enqueue(queue, skb);
	return true;
}

struct sk_buff *ovpn_prev_queue_dequeue(struct prev_queue *queue)
{
	struct sk_buff *tail = queue->tail, *next = smp_load_acquire(&NEXT(tail));

	if (tail == STUB(queue)) {
		if (!next)
			return NULL;
		queue->tail = next;
		tail = next;
		next = smp_load_acquire(&NEXT(next));
	}
	if (next) {
		queue->tail = next;
		atomic_dec(&queue->count);
		return tail;
	}
	if (tail != READ_ONCE(queue->head))
		return NULL;
	__ovpn_prev_queue_enqueue(queue, STUB(queue));
	next = smp_load_acquire(&NEXT(tail));
	if (next) {
		queue->tail = next;
		atomic_dec(&queue->count);
		return tail;
	}
	return NULL;
}

void ovpn_prev_queue_clean(struct prev_queue *queue)
{
	struct sk_buff *skb = NULL;
	enum packet_state state;

	while((skb = ovpn_prev_queue_peek(queue)) != NULL)
	{
		ovpn_prev_queue_drop_peeked(queue);

		state = atomic_read_acquire(&PACKET_CB(skb)->state);
		if (state != PACKET_STATE_UNCRYPTED)
		{
			kfree_skb_list(skb);
		}
	}
}

#undef NEXT
#undef STUB