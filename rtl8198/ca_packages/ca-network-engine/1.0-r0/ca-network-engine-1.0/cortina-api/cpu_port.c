#include <linux/module.h>
#include <asm/uaccess.h>
#include "osal_cmn.h"
#include "osal_common.h"
#include "aal_port.h"
#include "aal_ni.h"
#include "pkt_tx.h"
#include "rx.h"
#include "ca_ni_tx.h"

//#define CA_RX_REGISTER_CALLBACK_TEST 1
//#define CA_TX_KERNEL_MODE_TEST 	1

typedef struct cpu_rx_cb_list_t{
	ca_pkt_type_t		type;
	ca_rx_cb                fn;
	ca_uint8_t              priority;
	void                    *cookie;
	ca_uint32_t             flags;
	struct cpu_rx_cb_list_t *p_next;
}cpu_rx_cb_list_t;

/* there are two priority list one is high another is low */
cpu_rx_cb_list_t *cpu_rx_cb_high_head = NULL;
cpu_rx_cb_list_t *cpu_rx_cb_low_head = NULL;

CA_CPU_RX_MODE_t ca_ni_rx_mode = CA_CPU_RX_MODE_CALLBACK;

static spinlock_t g_ca_tx_lock;
static spinlock_t g_ca_rx_lock;

#if defined(CONFIG_NE_TX_LLID_MAP)
ca_ni_tx_src_mac_llid_map_t ca_ni_tx_src_mac_llid_map[CA_NI_TX_SRC_MAC_LLID_MAP_MAX];
#endif

#if defined(CONFIG_NE_ARP_FLOODING)
ca_ni_intf_ipaddr_t ca_ni_intf_ipaddr[CA_NI_INTF_IPADDR_MAX];
#endif

/* used for replacing the DA MAC of OAM packet */
//static ca_mac_addr_t puc_da = {0x00, 0x13, 0x25, 0x00, 0x00, 0x00};

int ca_ni_send_packet(ca_pkt_t *pkt, ca_uint8_t bypass_fwd_engine, int kernel_mode);
ca_status_t ca_ni_netlink_get_rx_type(ca_uint8_t *data, ca_pkt_type_t *type);
ca_status_t __ca_ni_send_single_pkt(ca_pkt_t *pkt, int go_9th_queue, ca_uint32_t pol_id);
int ca_ni_send_dying_gasp_packet(ca_pkt_t *pkt);

#ifdef CA_TX_KERNEL_MODE_TEST
static void ca_tx_test__ca_tx(void);
static void ca_tx_test_dying_gasp(void);
#endif

#ifdef CA_RX_REGISTER_CALLBACK_TEST
static ca_int32_t default_ca_rx_cb(
		CA_IN   ca_device_id_t device_id,
		CA_OUT  ca_pkt_t    *pkt,
		CA_IN   void        *cookie)
{
	int i;
	ca_uint8_t *data;

	ca_printf("%s: pkt=%p, cookie=%p\n", __func__, pkt, cookie);

	/* print out input parameter */
	ca_printf("%s: pkt_len=%d, pkt_type=%d, src_port_id=0x%x, src_sub_port_id=0x%x, dst_port_id=0x%x\n", __func__,
			pkt->pkt_len, pkt->pkt_type, pkt->src_port_id, pkt->src_sub_port_id, pkt->dst_port_id);

	data = pkt->pkt_data->data;
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, (void *)data, pkt->pkt_len, true);

	return CA_RX_HANDLED;
}

static void __call_ca_rx_register(void)
{
        ca_uint32_t cookie = 88;

        __ca_rx_register(0, CA_PKT_TYPE_IGMP, (ca_rx_cb)default_ca_rx_cb, 0, &cookie, 0);
	__ca_rx_register(0, CA_PKT_TYPE_DHCP, (ca_rx_cb)default_ca_rx_cb, 0, &cookie, 0);
	__ca_rx_register(0, CA_PKT_TYPE_ARP, (ca_rx_cb)default_ca_rx_cb, 0, &cookie, 0);
	__ca_rx_register(0, CA_PKT_TYPE_ICMP, (ca_rx_cb)default_ca_rx_cb, 0, &cookie, 0);
}

static void __call_ca_rx_unregister(void)
{
        ca_uint32_t cookie = 88;

        __ca_rx_unregister(0, CA_PKT_TYPE_IGMP, (ca_rx_cb)default_ca_rx_cb, 0);
        __ca_rx_unregister(0, CA_PKT_TYPE_DHCP, (ca_rx_cb)default_ca_rx_cb, 0);
        __ca_rx_unregister(0, CA_PKT_TYPE_ARP, (ca_rx_cb)default_ca_rx_cb, 0);
        __ca_rx_unregister(0, CA_PKT_TYPE_ICMP, (ca_rx_cb)default_ca_rx_cb, 0);
}
#endif

ca_status_t ca_tx(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_pkt_t    *pkt
)
{
	ca_pkt_block_t	tmp_pkt_blk;
	ca_uint8_t	bypass_fwd_engine;

	/* check the range of CoS of packet, the valid range is 0 to 8 */
	if (pkt->cos > 8)
		return CA_E_PARAM;

	/* port type of source port must be CPU port type */
	if (PORT_TYPE(pkt->src_port_id) != CA_PORT_TYPE_CPU) {
		return CA_E_PARAM;
	}

	/* the source port id should be one of CPU ports */
	if (PORT_ID(pkt->src_port_id) < 0x20) {
	if (PORT_ID(pkt->src_port_id) < AAL_LPORT_CPU_0 || (PORT_ID(pkt->src_port_id) >= (AAL_LPORT_CPU_0 + ca_ni_cpu_cpu_port_count))) {
		return CA_E_PARAM;
	}
	}
	else {
		if (PORT_ID(pkt->src_port_id) > 0x3f) {
			return CA_E_PARAM;
		}
	}

	copy_from_user(&tmp_pkt_blk, pkt->pkt_data, sizeof(ca_pkt_block_t));
	pkt->pkt_data = &tmp_pkt_blk;

	bypass_fwd_engine = ((pkt->flags & CA_TX_BYPASS_FWD_ENGINE) == CA_TX_BYPASS_FWD_ENGINE) ? 1 : 0;

	ca_ni_send_packet(pkt, bypass_fwd_engine, 0);

#ifdef CA_RX_REGISTER_CALLBACK_TEST
	if (PORT_ID(pkt->src_port_id) == AAL_LPORT_CPU_0) {
		ca_printf("%s: AAL_LPORT_CPU_0\n", __func__);
		__call_ca_rx_register();
	}

	if (PORT_ID(pkt->src_port_id) == AAL_LPORT_CPU_1) {
		ca_printf("%s: AAL_LPORT_CPU_1\n", __func__);
		__call_ca_rx_unregister();
	}

	if (PORT_ID(pkt->src_port_id) == AAL_LPORT_CPU_2) {
		pkt->pkt_type = CA_PKT_TYPE_IGMP;
		__ca_rx_register_callback(pkt);
	}
#endif

#ifdef CA_TX_KERNEL_MODE_TEST
	if (PORT_ID(pkt->src_port_id) == AAL_LPORT_CPU_0) {
		//ca_tx_test__ca_tx();
		ca_tx_test_dying_gasp();
	}
#endif

	return CA_E_OK;
}

/* the function used for calling from kernel mode to transmit packets out */
ca_status_t __ca_tx(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_pkt_t    *pkt
)
{
	ca_uint8_t      bypass_fwd_engine;

	/* check the range of CoS of packet, the valid range is 0 to 8 */
	if (pkt->cos > 8)
		return CA_E_PARAM;

	/* port type of source port must be CPU port type */
	if (PORT_TYPE(pkt->src_port_id) != CA_PORT_TYPE_CPU) {
		return CA_E_PARAM;
	}

	/* the source port id should be one of CPU ports */
        if (PORT_ID(pkt->src_port_id) < 0x20) {
	if (PORT_ID(pkt->src_port_id) < AAL_LPORT_CPU_0 || (PORT_ID(pkt->src_port_id) >= (AAL_LPORT_CPU_0 + ca_ni_cpu_cpu_port_count))) {
		return CA_E_PARAM;
	}
        }
        else {
                if (PORT_ID(pkt->src_port_id) > 0x3f) {
                        return CA_E_PARAM;
                }
        }

	bypass_fwd_engine = ((pkt->flags & CA_TX_BYPASS_FWD_ENGINE) == CA_TX_BYPASS_FWD_ENGINE) ? 1 : 0;

	spin_lock_bh(&g_ca_tx_lock);
	ca_ni_send_packet(pkt, bypass_fwd_engine, 1);
	spin_unlock_bh(&g_ca_tx_lock);

	return CA_E_OK;
}

static void __ca_rx_register_dump_all(void)
{
	int i;
	cpu_rx_cb_list_t  *p_node = NULL;

	ca_printf("%s: === dump RX high priority register list ===\n", __func__);
	i = 0;
	for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {
		ca_printf("%s: p_node=%p, cpu_rx_cb_high_head=%p\n", __func__, p_node, cpu_rx_cb_high_head);
		ca_printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
				i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
		i++;
	}

	ca_printf("%s: === dump RX low priority register list ===\n", __func__);
	i = 0;
	for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {
		ca_printf("%s: p_node=%p, cpu_rx_cb_low_head=%p\n", __func__, p_node, cpu_rx_cb_low_head);
		ca_printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
				i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
		i++;
	}
}

/* ca_rx_register() and ca_rx_unregister() have been moved to ca-iros */
ca_status_t __ca_rx_register(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_pkt_type_t  type,
	CA_IN ca_rx_cb    fn,
	CA_IN ca_uint8_t  priority,
	CA_IN void        *cookie,
	CA_IN ca_uint32_t flags
)
{
	//cpu_rx_cb_list_t  *p_head_node = NULL;
	cpu_rx_cb_list_t  *p_node = NULL;
	cpu_rx_cb_list_t  *p_new_node = NULL;
	cpu_rx_cb_list_t  *p_prev_node = NULL;
	ca_status_t        ret = CA_E_OK;

	/* check input parameters */
	if (fn == NULL) {
		ca_printf("%s: call back function is NULL!!\n", __func__);
		return CA_E_PARAM;
	}

	spin_lock_bh(&g_ca_rx_lock);

	if (priority) {
		if (cpu_rx_cb_high_head != NULL){
			for (p_node = cpu_rx_cb_high_head; p_node; p_node = p_node->p_next) {
				if (p_node->type == type && p_node->fn == fn && p_node->priority == priority) {
					ca_printf("%s: type=%d already exists!!\n", __func__, type);
					spin_unlock_bh(&g_ca_rx_lock);
					return CA_E_EXISTS;
				}
			}
		}
	}
	else {
		if (cpu_rx_cb_low_head != NULL){
			for (p_node = cpu_rx_cb_low_head; p_node; p_node = p_node->p_next) {
				if (p_node->type == type && p_node->fn == fn && p_node->priority == priority) {
					ca_printf("%s: type=%d already exists!!\n", __func__, type);
					spin_unlock_bh(&g_ca_rx_lock);
					return CA_E_EXISTS;
				}
			}
		}
	}

	p_new_node = ca_malloc(sizeof(cpu_rx_cb_list_t));
	if (p_new_node == NULL){
		ca_printf("%s: Out of memory!!\n", __func__);
		spin_unlock_bh(&g_ca_rx_lock);
		return CA_E_MEMORY;
	}
	memset(p_new_node, 0, sizeof(cpu_rx_cb_list_t));

	if (priority) {
		if (cpu_rx_cb_high_head == NULL) {
			p_new_node->p_next = cpu_rx_cb_high_head;
			cpu_rx_cb_high_head = p_new_node;
		}
		else {
			/* add to end of list */
			for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {
				p_prev_node = p_node;
			}
			p_prev_node->p_next = p_new_node;
		}
	}
	else {
		if (cpu_rx_cb_low_head == NULL) {
			p_new_node->p_next = cpu_rx_cb_low_head;
			cpu_rx_cb_low_head = p_new_node;
		}
		else {
			/* add to end of list */
			for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {
				p_prev_node = p_node;
			}
			p_prev_node->p_next = p_new_node;
		}
	}

	p_new_node->type        = type;
	p_new_node->fn          = fn;
	p_new_node->priority    = priority;
	p_new_node->cookie      = cookie;
	p_new_node->flags       = flags;

	ca_printf("%s: p_new_node=%p, cpu_rx_cb_low_head=%p, cpu_rx_cb_high_head=%p\n",
			__func__, p_new_node, cpu_rx_cb_low_head, cpu_rx_cb_high_head);
	__ca_rx_register_dump_all();

	spin_unlock_bh(&g_ca_rx_lock);
	return ret;
}

ca_status_t __ca_rx_unregister(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_pkt_type_t  type,
	CA_IN ca_rx_cb    fn,
	CA_IN ca_uint8_t  priority
)
{
	cpu_rx_cb_list_t  *p_node = NULL;
	//cpu_rx_cb_list_t  *p_new_node = NULL;
	cpu_rx_cb_list_t  *p_prev_node = NULL;
	//ca_status_t        ret = CA_E_OK;

	spin_lock_bh(&g_ca_rx_lock);
	if (priority) {
		if (cpu_rx_cb_high_head != NULL){
			for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {
				if (p_node->type == type && p_node->fn == fn && p_node->priority == priority) {
					if (p_node->p_next == NULL) {
						cpu_rx_cb_high_head = NULL;
					}
					else {
						if (p_prev_node != NULL)
							p_prev_node->p_next = p_node->p_next;
					}
					ca_free(p_node);

					///TODO: for test
					__ca_rx_register_dump_all();

					spin_unlock_bh(&g_ca_rx_lock);
					return CA_E_OK;
				}
				p_prev_node = p_node;
			}
			spin_unlock_bh(&g_ca_rx_lock);
			return CA_E_NOT_FOUND;
		}
	}
	else {
		if (cpu_rx_cb_low_head != NULL){
			for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {
				if (p_node->type == type && p_node->fn == fn && p_node->priority == priority) {
					if (p_node->p_next == NULL) {
						cpu_rx_cb_low_head = NULL;
					}
					else {
						if (p_prev_node != NULL)
							p_prev_node->p_next = p_node->p_next;
					}
					ca_free(p_node);

					///TODO: for test
					__ca_rx_register_dump_all();

					spin_unlock_bh(&g_ca_rx_lock);
					return CA_E_OK;
				}
				p_prev_node = p_node;
			}
			spin_unlock_bh(&g_ca_rx_lock);
			return CA_E_NOT_FOUND;
		}
	}
	spin_unlock_bh(&g_ca_rx_lock);
	return CA_E_NOT_FOUND;
}

ca_status_t __ca_rx_register_callback_check_pkt_type(ca_uint8_t *data, ca_pkt_type_t *pkt_type)
{
        cpu_rx_cb_list_t  *p_node = NULL;

	if (cpu_rx_cb_high_head == NULL && cpu_rx_cb_low_head == NULL)
		return CA_E_NOT_FOUND;
#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
	if (ca_ni_netlink_get_rx_type(data, pkt_type) != CA_E_OK)
		return CA_E_NOT_FOUND;

        spin_lock_bh(&g_ca_rx_lock);

        /* search high priority RX register call back routine */
        for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {

                /* find the registered pkt_type */
                if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == *pkt_type) {
			spin_unlock_bh(&g_ca_rx_lock);
			return CA_E_OK;
                }
        }

        /* search low priority RX register call back routine */
        for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {

                /* find the registered pkt_type */
                if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == *pkt_type) {
			spin_unlock_bh(&g_ca_rx_lock);
			return CA_E_OK;
                }
        }

        spin_unlock_bh(&g_ca_rx_lock);
#endif

	return CA_E_NOT_FOUND;
}

void __ca_rx_register_callback(ca_pkt_t *pkt)
{
	int i;
	cpu_rx_cb_list_t  *p_node = NULL;

	spin_lock_bh(&g_ca_rx_lock);

	/* search high priority RX register call back routine */
	i = 0;
	for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {

		/* find the registered pkt_type */
		if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == pkt->pkt_type) {

			ca_printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
					i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);

			p_node->fn(0, pkt, p_node->cookie);
		}
		i++;
	}

	/* search low priority RX register call back routine */
	i = 0;
	for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {

		/* find the registered pkt_type */
		if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == pkt->pkt_type) {

			ca_printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
					i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);

			p_node->fn(0, pkt, p_node->cookie);
		}
		i++;
	}

	spin_unlock_bh(&g_ca_rx_lock);
}

ca_status_t ca_tx_rx_init(void)
{
	spin_lock_init(&g_ca_tx_lock);
	spin_lock_init(&g_ca_rx_lock);

#if defined(CONFIG_NE_TX_LLID_MAP)
	memset(&(ca_ni_tx_src_mac_llid_map[0]), 0, sizeof(ca_ni_tx_src_mac_llid_map_t) * CA_NI_TX_SRC_MAC_LLID_MAP_MAX);
#endif

#if defined(CONFIG_NE_ARP_FLOODING)
	memset(&(ca_ni_intf_ipaddr[0]), 0, sizeof(ca_ni_intf_ipaddr_t) * CA_NI_INTF_IPADDR_MAX);
#endif

	return CA_E_OK;
}

/* test code for __ca_tx() test */
#ifdef CA_TX_KERNEL_MODE_TEST

ca_uint8 ni_test_dying_gasp_packet[] = {
	0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x13, 0x25, 0x00, 0x00, 0x01, 0x88, 0x09, 0x03, 0x00,
	0x52, 0x01, 0x00, 0x11, 0xfe, 0x0e, 0x00, 0x0d, 0xb6, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void ca_tx_test__ca_tx(void)
{
	ca_pkt_t        pkt;
	ca_pkt_block_t    pkt_blk;
	int cos;
	int i;
	ca_uint32_t dest_port, src_port, dest_sub_port;
	int ret;

	pkt_blk.data = &(ni_test_dying_gasp_packet[0]);
	pkt_blk.len = 64;

	pkt.pkt_data = &pkt_blk;
	pkt.block_count = 1;
	pkt.device_id = 0;
	pkt.src_port_id = 0x40010;
	pkt.dst_port_id = 0x30003;
	pkt.dst_sub_port_id = 0;
	pkt.pkt_len = 64;
	pkt.sw_id[0]=0x5566;
	pkt.sw_id[1]=0x7788;
	pkt.sw_id[2]=0x99aa;
	pkt.sw_id[3]=0xbbcc;
	pkt.flags = CA_TX_BYPASS_FWD_ENGINE;
	pkt.pkt_type = CA_PKT_TYPE_OAM;
	//pkt.dst_sub_port_id = 0x33;
	//pkt.pkt_len = 30;

        pkt.cos = 8;
        ret = __ca_tx(0, &pkt);
	printk("%s: __ca_tx() ret=%d\n", __func__, ret);
}

static void ca_tx_test_dying_gasp(void)
{
        ca_pkt_t        pkt;
        ca_pkt_block_t    pkt_blk;
        int cos;
        int i;
        ca_uint32_t dest_port, src_port, dest_sub_port;
        int ret;

	pkt_blk.data = kmalloc(2048, GFP_KERNEL);
	if (pkt_blk.data == NULL) {
		ca_printf("%s: kmalloc() failed!!\n", __func__);
		return;
	}
        memcpy(pkt_blk.data, &(ni_test_dying_gasp_packet[0]), 1500);

        pkt_blk.len = 64;
        pkt.pkt_data = &pkt_blk;
        pkt.block_count = 1;
        pkt.device_id = 0;
        pkt.src_port_id = 0x40010;
        pkt.dst_port_id = 0x30003;
        pkt.dst_sub_port_id = 0;
        pkt.pkt_len = 64;
        pkt.sw_id[0]=0x5566;
        pkt.sw_id[1]=0x7788;
        pkt.sw_id[2]=0x99aa;
        pkt.sw_id[3]=0xbbcc;
        pkt.flags = CA_TX_BYPASS_FWD_ENGINE;
	pkt.pkt_type = CA_PKT_TYPE_OAM;
        pkt.cos = 7;
        ret = __ca_tx_dying_gasp(0, &pkt);
        printk("%s: __ca_tx_dying_gasp() ret=%d\n", __func__, ret);
	kfree(pkt_blk.data);
}
#endif

ca_status_t __ca_tx_dying_gasp(
        CA_IN ca_device_id_t device_id,
        CA_IN ca_pkt_t    *pkt
)
{
        //ca_pkt_block_t    tmp_pkt_blk;
        //ca_uint8_t      bypass_fwd_engine;
	ca_status_t 	ret;

        /* check the range of CoS of packet, the valid range is 0 to 8 */
        if (pkt->cos > 8)
                return CA_E_PARAM;

        /* port type of source port must be CPU port type */
        if (PORT_TYPE(pkt->src_port_id) != CA_PORT_TYPE_CPU) {
                return CA_E_PARAM;
        }

        /* the source port id should be one of CPU ports */
        if (PORT_ID(pkt->src_port_id) < AAL_LPORT_CPU_0 || (PORT_ID(pkt->src_port_id) >= (AAL_LPORT_CPU_0 + ca_ni_cpu_cpu_port_count))) {
                return CA_E_PARAM;
        }
	//ret = __ca_ni_send_single_pkt(pkt, 1, CA_UINT32_INVALID);
	ret = ca_ni_send_dying_gasp_packet(pkt);
	return ret;
}

void ca_ni_set_rx_to_block_mode(void);
ca_status_t ca_rx_mode_set(
        CA_IN ca_device_id_t	device_id,
        CA_IN CA_CPU_RX_MODE_t	mode
)
{
	if ((mode != CA_CPU_PKT_RX_MODE_POLL) && (mode != CA_CPU_RX_MODE_CALLBACK)) {
		return CA_E_PARAM;
	}
	if (mode == CA_CPU_PKT_RX_MODE_POLL){
		ca_ni_set_rx_to_block_mode();
	}
	ca_ni_rx_mode = mode;
	return CA_E_OK;
}

ca_status_t ca_rx_mode_get(
	CA_IN  ca_device_id_t    device_id,
        CA_OUT CA_CPU_RX_MODE_t  *mode
)
{
	if (mode == NULL) {
                return CA_E_PARAM;
        }
        *mode = ca_ni_rx_mode;
        return CA_E_OK;
}

int ca_ni_rx_poll_block_mode(int cpu_port, u8 voq, int budget, ca_pkt_t *pkt);
ca_status_t ca_rx_poll(
        CA_IN ca_device_id_t 	device_id,
	CA_OUT ca_uint32_t 	*count,
        CA_IN ca_pkt_t    	*pkt
)
{
	int pkt_received;
#if 0
	int i;
	ca_pkt_block_t  tmp_pkt_blk;
#endif
	ca_pkt_t	*p_pkt = *(ca_pkt_t **)pkt;
	ca_pkt_t *p_orig_pkt = p_pkt;

#if 0
	printk("%s: count = %d, p_pkt=%p\n", __func__, *count, p_pkt);
	for (i = 0; i < *count; i++) {

		//copy_from_user(&tmp_pkt_blk, p_pkt->pkt_data, sizeof(ca_pkt_block_t));

		printk("[%d] p_pkt=0x%p, p_pkt->pkt_data=0x%p, p_pkt->pkt_data->data=0x%p\n", i, p_pkt, p_pkt->pkt_data, p_pkt->pkt_data);

		//p_pkt->pkt_data = &tmp_pkt_blk;

		p_pkt++;
	}
#endif

	pkt_received = ca_ni_rx_poll_block_mode(1, 0, *count, p_orig_pkt);

	*count = pkt_received;
	return CA_E_OK;
}

ca_status_t ca_tx_set_llid(
        CA_IN           ca_device_id_t  device_id,
        CA_IN           ca_mac_addr_t	source_mac,
        CA_IN           ca_uint32_t     llid)
{
#if defined(CONFIG_NE_TX_LLID_MAP)
	int i;

	/* check whether entry exists already */
	for (i = 0; i < CA_NI_TX_SRC_MAC_LLID_MAP_MAX; i++) {
		if (ca_ni_tx_src_mac_llid_map[i].valid == 1) {
			if (!memcmp(ca_ni_tx_src_mac_llid_map[i].source_mac, source_mac, sizeof(ca_mac_addr_t))) {
				return CA_E_EXISTS;
			}
		}
	}

	/* serach a empty entry */
	for (i = 0; i < CA_NI_TX_SRC_MAC_LLID_MAP_MAX; i++) {
		if (ca_ni_tx_src_mac_llid_map[i].valid == 0) {
			ca_ni_tx_src_mac_llid_map[i].valid = 1;
			memcpy(ca_ni_tx_src_mac_llid_map[i].source_mac, source_mac, sizeof(ca_mac_addr_t));
			ca_ni_tx_src_mac_llid_map[i].llid = llid;
			return CA_E_OK;
		}
	}
	return CA_E_RESOURCE;
#else
	return CA_E_RESOURCE;
#endif
}

ca_status_t ca_tx_get_llid(
        CA_IN           ca_device_id_t  device_id,
        CA_IN           ca_mac_addr_t   source_mac,
        CA_OUT          ca_uint32_t     *llid)
{
#if defined(CONFIG_NE_TX_LLID_MAP)
	int i;

	/* serach a empty entry */
	for (i = 0; i < CA_NI_TX_SRC_MAC_LLID_MAP_MAX; i++) {
		if (ca_ni_tx_src_mac_llid_map[i].valid == 1 && !memcmp(ca_ni_tx_src_mac_llid_map[i].source_mac, source_mac, sizeof(ca_mac_addr_t))) {
			*llid = ca_ni_tx_src_mac_llid_map[i].llid;
			//printk("%s: llid=%d\n", __func__, *llid);
			return CA_E_OK;
		}
	}
	return CA_E_NOT_FOUND;
#else
	return CA_E_NOT_FOUND;
#endif
}

EXPORT_SYMBOL(ca_tx);
EXPORT_SYMBOL(__ca_tx);
EXPORT_SYMBOL(__ca_tx_dying_gasp);
EXPORT_SYMBOL(__ca_rx_register);
EXPORT_SYMBOL(__ca_rx_unregister);
EXPORT_SYMBOL(__ca_rx_register_callback);
EXPORT_SYMBOL(ca_rx_poll);
EXPORT_SYMBOL(ca_rx_mode_set);
EXPORT_SYMBOL(ca_rx_mode_get);
EXPORT_SYMBOL(ca_tx_set_llid);
EXPORT_SYMBOL(ca_tx_get_llid);
