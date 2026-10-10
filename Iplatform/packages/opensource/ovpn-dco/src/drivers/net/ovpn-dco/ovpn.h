/* SPDX-License-Identifier: GPL-2.0-only */
/* OpenVPN data channel accelerator
 *
 *  Copyright (C) 2019-2023 OpenVPN, Inc.
 *
 *  Author:	James Yonan <james@openvpn.net>
 *		Antonio Quartulli <antonio@openvpn.net>
 */

#ifndef _NET_OVPN_DCO_OVPN_H_
#define _NET_OVPN_DCO_OVPN_H_

#include "main.h"
#include "peer.h"
#include "sock.h"
#include "ovpnstruct.h"

#include <linux/workqueue.h>
#include <linux/types.h>
#include <net/sock.h>

struct ovpn_struct;
struct net_device;

int ovpn_struct_init(struct net_device *dev);

u16 ovpn_select_queue(struct net_device *dev, struct sk_buff *skb,
		      struct net_device *sb_dev);

void ovpn_keepalive_xmit(struct ovpn_peer *peer);
void ovpn_explicit_exit_notify_xmit(struct ovpn_peer *peer);

netdev_tx_t ovpn_net_xmit(struct sk_buff *skb, struct net_device *dev);

int ovpn_recv(struct ovpn_struct *ovpn, struct ovpn_peer *peer, struct sk_buff *skb);

void ovpn_encrypt_work(struct work_struct *work);
void ovpn_decrypt_work(struct work_struct *work);
int ovpn_napi_poll(struct napi_struct *napi, int budget);
void ovpn_packet_tx_worker(struct work_struct *work);

int ovpn_send_data(struct ovpn_struct *ovpn, u32 peer_id, const u8 *data, size_t len);

enum packet_state {
	PACKET_STATE_UNCRYPTED,
	PACKET_STATE_CRYPTED,
	PACKET_STATE_KEEPALIVE,
	PACKET_STATE_DEAD
};

struct packet_cb {
	atomic_t state;
	u32 pkt_id;
	struct ovpn_crypto_key_slot *ks;
};

#define PACKET_CB(skb) ((struct packet_cb *)((skb)->cb))
#define MAX_QUEUED_PACKETS 2048

static inline int ovpn_cpumask_choose_online(int *stored_cpu, unsigned int id)
{
	unsigned int cpu = *stored_cpu, cpu_index, i;

	if (unlikely(cpu == nr_cpumask_bits ||
		    !cpumask_test_cpu(cpu, cpu_online_mask))) {
		cpu_index = ++id % cpumask_weight(cpu_online_mask);
		cpu = cpumask_first(cpu_online_mask);
		for (i = 0; i < cpu_index; ++i)
			cpu = cpumask_next(cpu, cpu_online_mask);
		*stored_cpu = cpu;
	}
	return cpu;
}

/* This function is racy, in the sense that it's called while last_cpu is
 * unlocked, so it could return the same CPU twice. Adding locking or using
 * atomic sequence numbers is slower though, and the consequences of racing are
 * harmless, so live with it.
 */
static inline int ovpn_cpumask_next_online(int *last_cpu)
{
	int cpu = *last_cpu;

	while (unlikely(!cpumask_test_cpu(cpu, cpu_online_mask)))
		cpu = cpumask_next(cpu, cpu_online_mask) % nr_cpumask_bits;
	*last_cpu = cpumask_next(cpu, cpu_online_mask) % nr_cpumask_bits;
	return cpu;
}

void ovpn_prev_queue_init(struct prev_queue *queue);

/* Multi producer */
bool ovpn_prev_queue_enqueue(struct prev_queue *queue, struct sk_buff *skb);

/* Single consumer */
struct sk_buff *ovpn_prev_queue_dequeue(struct prev_queue *queue);

/* Single consumer */
static inline struct sk_buff *ovpn_prev_queue_peek(struct prev_queue *queue)
{
	if (queue->peeked)
		return queue->peeked;
	queue->peeked = ovpn_prev_queue_dequeue(queue);
	return queue->peeked;
}

/* Single consumer */
static inline void ovpn_prev_queue_drop_peeked(struct prev_queue *queue)
{
	queue->peeked = NULL;
}

/* clean */
void ovpn_prev_queue_clean(struct prev_queue *queue);

#endif /* _NET_OVPN_DCO_OVPN_H_ */
