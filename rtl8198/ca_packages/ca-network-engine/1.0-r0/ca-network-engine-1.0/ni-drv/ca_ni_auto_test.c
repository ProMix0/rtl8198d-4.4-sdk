#include <linux/ip.h>
#include <linux/udp.h>

#include "ca_ni_auto_test.h"
#include "aal_fdb.h"
#include "ca_ni.h"
#include "ca_ni_proc.h"
#include "rx.h"
#include "pkt_tx.h"

ca_uint16_t ca_ni_rx_get_vid(ca_uint8_t *skb_data);

#define CA_RGMII_PORT CA_PORT_ID_NI4
#define AUTO_TEST_SERVER_PORT 9877

static ca_uint8_t pseudoUdpHdr[42]={
        0xff,0xff,0xff,0xff,0xff,0xff,  //DMAC
        0x00,0x13,0x25,0x00,0x00,0x02,  //SMAC
        0x08,0x00, //ETH
        0x45,0x00,
        0x00,0x00, //total length (ip hdr+l3 payload)
        0x00,0x01, //hops
        0x00,0x00, //flags & offset
        0x80, //TTL
        0x11, //PROTO: UDP
        0x00,0x00, //checksum
        0xc0,0xa8,0x01,0x01, //sip
        0xff,0xff,0xff,0xff, //dip
        0x27,0x0f,0x27,0x0f, //s_port,d_port
        0x00,0x00, //length (udp hdr+l4 payload)
        0x00,0x00, //checksum
};

static ca_status_t ca_ni_get_source_port(ca_uint8_t *data, ca_uint8_t *port_id)
{
        ca_status_t ret;
        ca_mac_addr_t mac;
        ca_uint16_t key_vid;
        aal_fdb_entry_cfg_t fdb_entry;

        key_vid = ca_ni_rx_get_vid(data);
        memcpy(mac, data + 6, sizeof(ca_mac_addr_t));

        ret = aal_fdb_entry_get(0, mac, key_vid, &fdb_entry);
        if (ret == CA_E_OK) {
                *port_id = fdb_entry.port_id;
                AUTO_TOOL_DEBUG("%s: port_id=%d\n", __func__, fdb_entry.port_id);
        }
        return ret;
}

ca_status_t ca_ni_tx_auto_test(HEADER_A_T *hdr_a, HEADER_CPU_T *hdr_cpu, ca_uint8_t *data, ca_uint32_t ori_pkt_len)
{
        ca_uint8_t      src_port_id;
        ca_uint8_t      dst_port_id;
        ca_pkt_t        pkt;
        ca_pkt_block_t  pkt_blk;
        ca_uint8_t      *pkt_data;
        ca_uint32_t     pkt_len = ori_pkt_len;
        ca_status_t     ret;
        struct iphdr    *ip_hdr;
        struct udphdr   *udp_hdr;
        if (ca_ni_debug & NI_DBG_AUTO_TOOL) {
                /*** header A ***/
                ca_ni_dump_hdr_a(hdr_a, true);

                /*** packet ***/
                print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, ori_pkt_len, true);
        }

        if ((ret = ca_ni_get_source_port(data, &src_port_id)) != CA_E_OK)
        {
                AUTO_TOOL_DEBUG("%s: Get source port failed, ret = %d\n", __func__, ret);
                return ret;
        }

        if (src_port_id == CA_RGMII_PORT)
        {
                udp_hdr = (struct udphdr*)(data + sizeof(struct ethhdr) + sizeof(struct iphdr));
                if (ntohs(udp_hdr->source) == AUTO_TEST_SERVER_PORT
                                && ntohs(udp_hdr->dest) == AUTO_TEST_SERVER_PORT)
                {
                        /* Remove pseudo header and dst port */
                        dst_port_id = data[pkt_len - sizeof(dst_port_id)];
                        data += sizeof(pseudoUdpHdr);
                        pkt_len -= sizeof(pseudoUdpHdr) + sizeof(dst_port_id);
                        pkt_data = kmalloc(pkt_len, GFP_KERNEL);
                        if (pkt_data == NULL)
                        {
                                printk("%s: Not enough memory to allocate skb of size %lu.\n",
                                        __func__, pkt_len);
                                return -ENOMEM;
                        }
                        memcpy(pkt_data, data, pkt_len);
                        AUTO_TOOL_DEBUG("%s: [from RGMII] send to port %d\n", __func__, dst_port_id);
                }
                else {
                        AUTO_TOOL_DEBUG("%s: Drop packet due to udp port is not 9877!!!!!!!!\n", __func__, dst_port_id);
                        return CA_E_ERROR;
                }
        }
        else if (src_port_id <= CA_PORT_ID_NI7)
        {
                /* Add pseudo header in front of the packet */
                pkt_len += sizeof(pseudoUdpHdr) + sizeof(src_port_id);
                ip_hdr = (struct iphdr*)(pseudoUdpHdr + sizeof(struct ethhdr));
                udp_hdr = (struct udphdr*)(pseudoUdpHdr + sizeof(struct ethhdr) + sizeof(struct iphdr));
                ip_hdr->id += 1;
                ip_hdr->tot_len = htons(pkt_len - sizeof(struct ethhdr));
                udp_hdr->len = htons(pkt_len - sizeof(struct ethhdr) - sizeof(struct iphdr));
                pkt_data = kmalloc(pkt_len, GFP_KERNEL);
                if (pkt_data == NULL)
                {
                        printk("%s: Not enough memory to allocate skb of size %lu.\n",
                                __func__, pkt_len);
                        return -ENOMEM;
                }
                memcpy(pkt_data, pseudoUdpHdr, sizeof(pseudoUdpHdr));

                /* Add real data in the payload of the packet */
                memcpy(pkt_data + sizeof(pseudoUdpHdr), data, ori_pkt_len);

                /* Add source port in the end of the packet */
                memcpy(pkt_data + sizeof(pseudoUdpHdr) + ori_pkt_len, &src_port_id, sizeof(src_port_id));
                dst_port_id = CA_RGMII_PORT;
                AUTO_TOOL_DEBUG("%s: [from Port%d] send to RGMII Port\n", __func__, src_port_id);
        } else {
                AUTO_TOOL_DEBUG("%s: Drop packet due to src port %d!!!!!!!!\n", __func__, dst_port_id);
                return CA_E_ERROR;
        }

        if (ca_ni_debug & NI_DBG_AUTO_TOOL) {
                /*** packet ***/
                printk("Packet:\n");
                print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, pkt_data, pkt_len, true);
                printk("===============================================================\n");
        }

        memset(&pkt, 0, sizeof(ca_pkt_t));
        memset(&pkt_blk, 0, sizeof(ca_pkt_block_t));
        pkt_blk.data = pkt_data;
        pkt_blk.len = pkt_len;

        pkt.pkt_data = &pkt_blk;
        pkt.block_count = 1;
        pkt.device_id = 0;
        if (dst_port_id < CA_PORT_ID_NI7)
        {
                pkt.src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU1);
        } else
        {
                pkt.src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU0);
        }
        pkt.dst_port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, dst_port_id);
        pkt.dst_sub_port_id = 0;
        pkt.pkt_len = pkt_len;
        pkt.flags = CA_TX_BYPASS_FWD_ENGINE;
        pkt.cos = 6;

        ret = __ca_tx(0, &pkt);
        kfree(pkt_data);
        return ret;
}
