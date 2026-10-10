#include <soc/cortina/ca_ipc_pe.h>

#include "osal_cmn.h"
#include "osal_common.h"
#include "aal_common.h"
#include "ca-scfg/scfg.h"
#include "port.h"
#include "aal_port.h"
#include "sw_rate_limiter.h"

#define SW_RATE_LIMITER_IPC_MAX				4
#define SW_RATE_LIMITER_IPC_INIT_DEFAULT_RL		0
#define SW_RATE_LIMITER_IPC_ADD_ENTRY			1
#define SW_RATE_LIMITER_IPC_DEL_ENTRY			2
#define SW_RATE_LIMITER_IPC_DEL_ALL_ENTRY		3

typedef struct ca_sw_rate_limiter_cb {
        struct ca_sw_rate_limiter_cb *p_next;
	ca_sw_rate_limiter_entry_t   entry;
} ca_sw_rate_limiter_cb_t;

static ca_uint 			sw_rate_limiter_lock;
static ca_uint8_t 		sw_shpaer_init_done = 0;
static ca_uint8_t		g_sw_rate_limiter_enable = 0;
static ca_uint32_t		g_sw_rate_limiter_max_rate = 0;
static ca_uint32_t		g_max_rate_limiter_supported = CA_SW_RATE_LIMITER_MAX;
static ca_uint32_t		g_sw_rate_limiter_default_rate = MAX_PE_FWD_RATE;

static ca_sw_rate_limiter_cb_t	*p_sw_rate_limiter_list = NULL;

static ca_uint8_t	g_sw_rate_limiter_voq_list[CA_SW_RATE_LIMITER_MAX];

/* IPC communicate with Taroko */
#ifdef CONFIG_CORTINA_IPC2RCPU
static int ca_sw_rate_limiter_ipc_debug_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
	ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
	ca_uint32_t i;
	const char *data = msg_data;
	ca_printf("msg_no %d trans_id %d msg_size %d\n", msg_no, trans_id, *msg_size);
	ca_printf("session_id %d cpu_id %d \n\t", peer.session_id, peer.cpu_id);
	for (i = 0; i < *msg_size; i++) {
		ca_printf("%02X ", data[i]);
		if ((i % 32) == 31)
			ca_printf("\n\t");
	}
	ca_printf("\n");
	return CA_E_OK;
}

static ca_ipc_msg_handle_t sw_rate_limiter_ipc_msg[SW_RATE_LIMITER_IPC_MAX] = {
	{ .msg_no = SW_RATE_LIMITER_IPC_INIT_DEFAULT_RL, .proc = ca_sw_rate_limiter_ipc_debug_handle},
	{ .msg_no = SW_RATE_LIMITER_IPC_ADD_ENTRY, .proc = ca_sw_rate_limiter_ipc_debug_handle},
	{ .msg_no = SW_RATE_LIMITER_IPC_DEL_ENTRY, .proc = ca_sw_rate_limiter_ipc_debug_handle},
	{ .msg_no = SW_RATE_LIMITER_IPC_DEL_ALL_ENTRY, .proc = ca_sw_rate_limiter_ipc_debug_handle},
};

static ca_status_t ca_sw_rate_limiter_ipc_init(ca_uint32_t dest_VoQ)
{
        ca_status_t ret;
        char rcv_buff[16];
        ca_uint16_t result_size = 16;
        ca_ipc_pkt_t ipc_pkt;
        ca_uint32_t i;
        ca_sw_rate_limiter_ipc_init_t ipc_init_entry;

	ca_printf("%s: dest_VoQ=%d\n", __func__, dest_VoQ);

        ipc_init_entry.dest_VoQ = dest_VoQ;

        ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
        ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
        ipc_pkt.priority = 0;
        ipc_pkt.msg_no = SW_RATE_LIMITER_IPC_INIT_DEFAULT_RL;
        ipc_pkt.msg_data = &ipc_init_entry;               	/* the message data to transmit. */
        ipc_pkt.msg_size = sizeof(ca_sw_rate_limiter_ipc_init_t);    	/* the length of message data. */
        ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
        if (ret != CA_E_OK) {
                ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
                return ret;
        }

        return CA_E_OK;
}

static ca_status_t ca_sw_rate_limiter_ipc_add_entry(ca_sw_rate_limiter_cb_t *rate_limiter_cb)
{
	ca_status_t ret;
	char rcv_buff[16];
	ca_uint16_t result_size = 16;
	ca_ipc_pkt_t ipc_pkt;
	ca_uint32_t i;
	ca_sw_rate_limiter_ipc_entry_t ipc_entry;

	ipc_entry.dest_VoQ = rate_limiter_cb->entry.dest_VoQ;

	ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = SW_RATE_LIMITER_IPC_ADD_ENTRY;
	ipc_pkt.msg_data = &ipc_entry; 				/* the message data to transmit. */
	ipc_pkt.msg_size = sizeof(ca_sw_rate_limiter_ipc_entry_t); 	/* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

static ca_status_t ca_sw_rate_limiter_ipc_del_entry(ca_uint32_t dest_VoQ)
{
        ca_status_t ret;
        char rcv_buff[16];
        ca_uint16_t result_size = 16;
        ca_ipc_pkt_t ipc_pkt;
        ca_uint32_t i;
        ca_sw_rate_limiter_ipc_entry_t ipc_entry;

	memset(&ipc_entry, 0,  sizeof(ca_sw_rate_limiter_ipc_entry_t));
        ipc_entry.dest_VoQ = dest_VoQ;

        ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
        ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
        ipc_pkt.priority = 0;
        ipc_pkt.msg_no = SW_RATE_LIMITER_IPC_DEL_ENTRY;
        ipc_pkt.msg_data = &ipc_entry; /* the message data to transmit. */
        ipc_pkt.msg_size = sizeof(ca_sw_rate_limiter_ipc_entry_t); /* the length of message data. */
        ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
        if (ret != CA_E_OK) {
                ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
                return ret;
        }

        return CA_E_OK;
}

static ca_status_t ca_sw_rate_limiter_ipc_del_all_entry(void)
{
        ca_status_t ret;
        char rcv_buff[16];
        ca_uint16_t result_size = 16;
        ca_ipc_pkt_t ipc_pkt;
        ca_uint32_t i;
        ca_sw_rate_limiter_ipc_entry_t ipc_entry;

	memset(&ipc_entry, 0,  sizeof(ca_sw_rate_limiter_ipc_entry_t));
        ipc_entry.dest_VoQ = 0;

        ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
        ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
        ipc_pkt.priority = 0;
        ipc_pkt.msg_no = SW_RATE_LIMITER_IPC_DEL_ALL_ENTRY;
        ipc_pkt.msg_data = &ipc_entry; /* the message data to transmit. */
        ipc_pkt.msg_size = sizeof(ca_sw_rate_limiter_ipc_entry_t); /* the length of message data. */
        ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
        if (ret != CA_E_OK) {
                ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
                return ret;
        }

        return CA_E_OK;
}
#endif

ca_status_t ca_sw_rate_limiter_init(void)
{
	ca_status_t ret;
	ca_uint32_t	sw_rate_limiter_default_rate;
	ca_uint32_t	sw_rate_limiter_max_rate;

	if (sw_shpaer_init_done)
		return CA_E_OK;

	ret = aal_scfg_read(CFG_ID_SW_RATE_LIMITER_MAX_RATE, 4, &sw_rate_limiter_max_rate);
	if (ret == CA_E_OK) {
		if (sw_rate_limiter_max_rate > 0)
			g_sw_rate_limiter_max_rate = sw_rate_limiter_max_rate;
	}
	else {
		ca_printf("%s: failed to read CFG_ID_SW_RATE_LIMITER_MAX_RATE, ret=%d\n", __func__, ret);
		return CA_E_OK;
	}

	ca_printf("%s: CFG_ID_SW_RATE_LIMITER_MAX_RATE=%d\n", __func__, sw_rate_limiter_max_rate);

	/* the rate of max rate limiter is 0 means rate limiter is disabled */
	if (!g_sw_rate_limiter_max_rate) {
		g_sw_rate_limiter_enable = 0;
		return CA_E_OK;
	}

	ret = aal_scfg_read(CFG_ID_SW_RATE_LIMITER_DEFAULT_RATE, 4, &sw_rate_limiter_default_rate);
	if (ret == CA_E_OK) {
		if (sw_rate_limiter_default_rate > 0)
			g_sw_rate_limiter_default_rate = sw_rate_limiter_default_rate;
	}
	else {
		ca_printf("%s: failed to read CFG_ID_SW_RATE_LIMITER_DEFAULT_RATE, use default value, ret=%d\n", __func__, ret);
	}

	ca_printf("%s: CFG_ID_SW_RATE_LIMITER_DEFAULT_RATE=%d\n", __func__, sw_rate_limiter_default_rate);

	/* the default rate limiter is 0 means rate limiter is disabled */
	if (!sw_rate_limiter_default_rate) {
		g_sw_rate_limiter_enable = 0;
		return CA_E_OK;
	}

        if (CA_OSAL_OK != ca_spin_lock_init(&sw_rate_limiter_lock, SPINLOCK_CMN)) {
                ca_printf("%s: init spinlock fail\n", __func__);
                return CA_E_INIT;
        }

	memset(&g_sw_rate_limiter_voq_list[0], 0, CA_SW_RATE_LIMITER_MAX);

	g_sw_rate_limiter_enable = 1;
	sw_shpaer_init_done = 1;

#ifdef CONFIG_CORTINA_IPC2RCPU
        ret = ca_ipc_msg_handle_register(CA_IPC_SESSION_SW_RATE_LIMITER, sw_rate_limiter_ipc_msg, SW_RATE_LIMITER_IPC_MAX);

        if (ret != CA_E_OK) {
                ca_printf("%s: ca-ipc_msg_handle_register() fail ret %d\n", __func__, ret);
        }

        ca_sw_rate_limiter_ipc_init(CA_SW_RATE_LIMITER_DEFAULT_VOQ);
#endif

	/* add default rate limiter */
	ret = ca_sw_rate_limiter_entry_add(0, sw_rate_limiter_default_rate, 1);
	if (ret != CA_E_OK) {
		printk("%s: add default rate limiter failed!!, ret=%d\n", __func__, ret);
		return CA_E_ERROR;
	}

	return CA_E_OK;
}

static ca_status_t ca_sw_rl_add_l3_cls_rule(ca_uint32_t flow_id, ca_uint32_t *rule_index)
{
	ca_status_t ret;
	ca_classifier_key_t key;
	ca_classifier_key_mask_t key_mask;
	ca_classifier_action_t	action;

	memset(&key, 0, sizeof(ca_classifier_key_t));
	memset(&key_mask, 0, sizeof(ca_classifier_key_mask_t));
	memset(&action, 0, sizeof(ca_classifier_action_t));

	/* setup cls key */
	key.src_port = CA_PORT_ID(CA_PORT_TYPE_L2RP, AAL_LPORT_L3_LAN);

	///TODO: debug the source ip and dest ip should be adjusted
	key.ip.ip_sa.afi = CA_IPV4;
	key.ip.ip_sa.ip_addr.ipv4_addr = 0xc0a80164 + flow_id;
	key.ip.ip_sa.addr_len = 32;

	key.ip.ip_da.afi = CA_IPV4;
        key.ip.ip_da.ip_addr.ipv4_addr = 0xc0a86464 + flow_id;
        key.ip.ip_da.addr_len = 32;

	/* setup cls key mask */
	key_mask.src_port 		= 1;
	key_mask.ip 		= 1;
	key_mask.ip_mask.ip_sa 	= 1;
	key_mask.ip_mask.ip_da 	= 1;

	/* setup cls action */
	action.forward = CA_CLASSIFIER_FORWARD_FE;
	action.dest.port = 0;
	action.options.sw_shaper_id = flow_id;
	action.options.flow_id = flow_id + CA_PILICER_L3_FLOW_ID_BASE;

	action.options.masks.sw_shaper_id = 1;
	action.options.masks.flow_id = 1;

	ret = ca_classifier_rule_add(0, 11, &key, &key_mask, &action, rule_index);
	if (ret != CA_E_OK) {
		ca_printf("%s: classifier_rule_add() failed!!, ret=%d\n", __func__, ret);
		return ret;
	}
	ca_printf("%s: rule_index=%d\n", __func__, *rule_index);
	return CA_E_OK;
}

static ca_uint32_t ca_sw_rl_get_total_configured_rate(void)
{
	ca_uint32_t total_rate = 0;
	ca_sw_rate_limiter_cb_t *p_temp;

	p_temp = p_sw_rate_limiter_list;
	while (p_temp) {
		total_rate += p_temp->entry.rate;
		p_temp = p_temp->p_next;
	}

	ca_printf("%s: total_rate=%d\n", __func__, total_rate);
	return total_rate;
}

static ca_uint32_t ca_sw_rl_get_rate(ca_uint32 configured_rate)
{
	ca_uint32_t	surplus_bw;
	ca_uint32_t	egress_rate;

	surplus_bw = g_sw_rate_limiter_max_rate - ca_sw_rl_get_total_configured_rate();

	ca_printf("%s: surplus_bw=%d\n", __func__, surplus_bw);

	egress_rate = configured_rate + (((ca_uint64_t)surplus_bw * configured_rate) / ca_sw_rl_get_total_configured_rate());
	ca_printf("%s: egress_rate=%d\n", __func__, egress_rate);
	return egress_rate;
}

static ca_uint32_t ca_sw_rl_get_next_avaliable_voq(void)
{
	ca_uint32_t voq;

	for (voq = 0; voq < CA_SW_RATE_LIMITER_MAX; voq++) {
		if (g_sw_rate_limiter_voq_list[voq] == 0) {
			g_sw_rate_limiter_voq_list[voq] = 1;
			break;
		}
	}
	if (voq == CA_SW_RATE_LIMITER_MAX) {
		ca_printf("%s: No more voq available!!\n", __func__);
		return CA_E_FULL;
	}
	return voq;
}

static ca_uint32_t ca_sw_rl_get_flow_id(ca_uint32_t voq)
{
	return voq;
}

static void ca_sw_rl_flow_policer_disable(ca_uint32_t flow_id)
{
	ca_policer_t	policer;

	policer.mode = CA_POLICER_MODE_SRTCM;
	policer.pps = 0;
	policer.cir = 0;
	policer.cbs = 1024;
	policer.pir = 0;
	policer.pbs = 1024;

	ca_flow_policer_set(0, flow_id + CA_PILICER_L3_FLOW_ID_BASE, &policer);
}

static void ca_sw_rl_flow_policer_set(void)
{
	ca_status_t ret;
	ca_policer_t	policer;
	ca_sw_rate_limiter_cb_t *p_temp = NULL;

	/* adjust policer rate for all rate limiter (flow rate  = ca_get_rate(configured_rate)); */
	p_temp = p_sw_rate_limiter_list;
	while (p_temp) {

		policer.mode = CA_POLICER_MODE_SRTCM;
		policer.pps = 0;
		policer.cir = ca_sw_rl_get_rate(p_temp->entry.rate);
		policer.cbs = 1024;
		policer.pir = policer.cir;
		policer.pbs = 1024;

		ca_printf("%s: p_temp->entry.rate=%d, policer.cir=%d\n", __func__, p_temp->entry.rate, policer.cir);

		ret = ca_flow_policer_set(0, p_temp->entry.flow_id + CA_PILICER_L3_FLOW_ID_BASE, &policer);
		ca_printf("%s: ca_flow_policer_set(flow_id=%d), ret=%d\n", __func__, p_temp->entry.flow_id, ret);

		p_temp = p_temp->p_next;
	}
}


ca_status_t ca_sw_rate_limiter_entry_add (
    CA_IN  ca_device_id_t 		device_id,
    CA_IN  ca_uint32_t		configured_rate,
    CA_IN ca_uint8_t		default_f)
{
	int i, has_default;
	ca_status_t ret;
	ca_uint32_t dest_voq;
	ca_uint32_t classifier_index;
	ca_sw_rate_limiter_cb_t *p_temp = NULL;
	ca_sw_rate_limiter_cb_t *p_last_entry = NULL;
	ca_policer_t	policer;

	if (!g_sw_rate_limiter_enable) {
		return CA_E_INACTIVE;
	}

	ca_printf("%s: configured_rate=%d, default_f=%d\n", __func__, configured_rate, default_f);

	ca_spin_lock(sw_rate_limiter_lock);

	/* check if there is enough rate left */
	if ((configured_rate + ca_sw_rl_get_total_configured_rate()) > g_sw_rate_limiter_max_rate) {
		ca_printf("%s: configured_rate exceed max rate %d\n", __func__, g_sw_rate_limiter_max_rate);
		ca_spin_unlock(sw_rate_limiter_lock);
		return CA_E_PARAM;
	}

	/* search whether rate limiter is already occupied */
	i = 0;
	has_default = 0;
	p_temp = p_sw_rate_limiter_list;
	while (p_temp) {
		if (p_temp->entry.f_default_limiter) {
			has_default = 1;
		}
		if (has_default && default_f) {
			ca_printf("%s: default rate limiter already exists!!!\n", __func__);
			ca_spin_unlock(sw_rate_limiter_lock);
			return CA_E_EXISTS;
		}
		p_last_entry = p_temp;          /* record the last one entry */
		p_temp = p_temp->p_next;
		i++;
	}

	/* the shaper id equal to g_max_rate_limiter_supported - 1 is reserved for default shaper */
	if (i > g_max_rate_limiter_supported - 1) {
		ca_printf("%s: software rate limiter entry is full!!!\n", __func__);
		ca_spin_unlock(sw_rate_limiter_lock);
		return CA_E_FULL;
	}

	p_temp = ca_malloc(sizeof(ca_sw_rate_limiter_cb_t));
	if (p_temp == NULL) {
		ca_printf("%s: can not allocate shaper because OOM!!!\n", __func__);
		ca_spin_unlock(sw_rate_limiter_lock);
                return CA_E_MEMORY;
	}

	if (!p_sw_rate_limiter_list) {
		p_sw_rate_limiter_list = p_temp;
	}
	else {
		p_last_entry->p_next = p_temp;
		p_temp->p_next = NULL;
	}

	/* fill ca_sw_rate_limiter_entry_t */
	dest_voq = ca_sw_rl_get_next_avaliable_voq();
	p_temp->entry.sw_rate_limiter_id = p_temp->entry.dest_VoQ = dest_voq;
	p_temp->entry.flow_id = ca_sw_rl_get_flow_id(dest_voq);
	p_temp->entry.f_default_limiter = (default_f) ? TRUE:FALSE;
	p_temp->entry.rate = configured_rate;

	classifier_index = 0;
	ret = ca_sw_rl_add_l3_cls_rule(p_temp->entry.flow_id, &classifier_index);
	if (ret == CA_E_OK) {
		p_temp->entry.classifier_index = classifier_index;
	}

	/* adjust policer rate for all rate limiter (flow rate  = ca_get_rate(configured_rate)); */
	/* re-calcluate the commited rate and set flow policer */
	ca_sw_rl_flow_policer_set();

	ca_spin_unlock(sw_rate_limiter_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
	/* send VoQ add to PE via IPC, if dest VoQ = default VoQ which has been sent to PE in initial */
	if (dest_voq != CA_SW_RATE_LIMITER_DEFAULT_VOQ)
		ca_sw_rate_limiter_ipc_add_entry(p_temp);
#endif

        return CA_E_OK;
}

ca_status_t ca_sw_rate_limiter_entry_delete (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  sw_rate_limiter_id)
{
        int i;
	ca_uint32_t dest_VoQ;
        ca_sw_rate_limiter_cb_t *p_temp, *p_last;

        if (!g_sw_rate_limiter_enable) {
                return CA_E_INACTIVE;
        }

	ca_printf("%s: sw_rate_limiter_id=%d\n", __func__, sw_rate_limiter_id);

	/* can not delete default rate limiter */
	if (sw_rate_limiter_id == CA_SW_RATE_LIMITER_DEFAULT_VOQ) {
		ca_printf("%s: can not delete default rate limiter!!\n", __func__);
		return CA_E_PARAM;
	}

	ca_spin_lock(sw_rate_limiter_lock);

        /* find the sw shaper entry to be deleted */
	p_temp = p_sw_rate_limiter_list;
	while (p_temp) {
		if (p_temp->entry.sw_rate_limiter_id == sw_rate_limiter_id) {
			ca_printf("%s: Got sw rate limiter entry, id=%d\n", __func__, sw_rate_limiter_id);
			break;
		}
		p_last = p_temp;
		p_temp = p_temp->p_next;
        }
        if (!p_temp) {
		ca_spin_unlock(sw_rate_limiter_lock);
                return CA_E_NOT_FOUND;
	}

	dest_VoQ = p_temp->entry.dest_VoQ;

	/* disable the deleted flow policer */
	ca_sw_rl_flow_policer_disable(p_temp->entry.flow_id);

	/* delete classifier rule */
	ca_classifier_rule_delete(0, p_temp->entry.classifier_index);

	/* free voq */
	g_sw_rate_limiter_voq_list[dest_VoQ] = 0;

	if (p_temp == p_sw_rate_limiter_list) {
		p_sw_rate_limiter_list = p_temp->p_next;
	}
	else {
		p_last->p_next = p_temp->p_next;
	}
	ca_free(p_temp);

	/* re-calcluate the commited rate and set flow policer */
	ca_sw_rl_flow_policer_set();

	ca_spin_unlock(sw_rate_limiter_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
	/* send VoQ add to PE via IPC (by now all packets in VoQ should be drained) */
        ca_sw_rate_limiter_ipc_del_entry(dest_VoQ);
#endif

        return CA_E_OK;
}

ca_status_t ca_sw_rate_limiter_delete_all (
    CA_IN  ca_device_id_t device_id)
{
	int i;
        ca_sw_rate_limiter_cb_t *p_temp, *p_next;

	if (!g_sw_rate_limiter_enable) {
		return CA_E_INACTIVE;
	}

	ca_spin_lock(sw_rate_limiter_lock);

	/* delete all of entry except the default rate entry */
	p_temp = p_sw_rate_limiter_list->p_next;
	while (p_temp) {
		p_next = p_temp->p_next;
		ca_free(p_temp);
		p_temp = p_next;
	}

	ca_spin_unlock(sw_rate_limiter_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
        ca_sw_rate_limiter_ipc_del_all_entry();
#endif

	return CA_E_OK;
}

ca_status_t ca_sw_rate_limiter_entry_get (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  sw_rate_limiter_id,
    CA_OUT ca_sw_rate_limiter_entry_t	*p_sw_rate_limiter)
{
        int i;
	ca_status_t ret;
        ca_sw_rate_limiter_cb_t *p_temp;

        if (!g_sw_rate_limiter_enable) {
                return CA_E_INACTIVE;
        }

        if (p_sw_rate_limiter == NULL) {
		ca_printf("%s: p_sw_shaper is NULL!!!\n", __func__);
                return CA_E_PARAM;
        }

	/* can get the default shaper id */
	if (sw_rate_limiter_id >= CA_SW_RATE_LIMITER_MAX - 1) {
		ca_printf("%s: software rate limiter id exceed %d!!!\n", __func__, CA_SW_RATE_LIMITER_MAX - 1);
		return CA_E_PARAM;
	}

	ca_spin_lock(sw_rate_limiter_lock);

	p_temp = p_sw_rate_limiter_list;
        while (p_temp) {
                if (p_temp->entry.sw_rate_limiter_id == sw_rate_limiter_id) {
			break;
                }
                p_temp = p_temp->p_next;
        }

	if (p_temp != NULL) {
		memcpy(p_sw_rate_limiter, &(p_temp->entry), sizeof(ca_sw_rate_limiter_entry_t));
		ret = CA_E_OK;
	}
	else {
		ca_printf("%s: sw_rate_limiter_id=%d not found!!!\n", __func__, sw_rate_limiter_id);
		ret = CA_E_NOT_FOUND;
	}

	ca_spin_unlock(sw_rate_limiter_lock);

        return ret;
}

ca_status_t ca_sw_rate_limiter_entry_iterate (
    CA_IN  ca_device_id_t 	 device_id,
    CA_OUT ca_iterator_t *return_entry)
{
	ca_uint32_t i, start, e_sz, cnt;
	ca_sw_rate_limiter_entry_t *p_entry = return_entry->entry_data;
	ca_int32_t b_sz = return_entry->user_buffer_size;
	ca_sw_rate_limiter_cb_t *p_temp;

        if (!g_sw_rate_limiter_enable) {
                return CA_E_INACTIVE;
        }

	if (return_entry == NULL || return_entry->entry_data == NULL)
		return CA_E_PARAM;

	e_sz = sizeof(ca_sw_rate_limiter_entry_t);
	//ca_printf("%s: return_entry->prev_handle=%p\n", __func__, return_entry->prev_handle);
	if (return_entry->prev_handle == NULL) {
		p_temp = p_sw_rate_limiter_list;
	} else {
		p_temp = return_entry->prev_handle;
	}

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	ca_spin_lock(sw_rate_limiter_lock);

	/* the shaper id equal to g_max_rate_limiter_supported - 1 is reserved for default shaper */
	for (i = 0, cnt = 0; (p_temp != NULL) && (i < g_max_rate_limiter_supported) && (b_sz >= e_sz) && (cnt < return_entry->entry_count); i++) {
                //ca_printf("%s: p_rate_limiter_temp=%p, i=%d, cnt=%d, b_sz=%d, e_sz=%d\n", __func__, p_rate_limiter_temp, i, cnt, b_sz, e_sz);

		if (p_temp == NULL)
			break;
		/* copy data */
		memcpy(p_entry, &(p_temp->entry), sizeof(ca_sw_rate_limiter_entry_t));
		b_sz -= e_sz;
		cnt++;
		p_entry++;

		p_temp = p_temp->p_next;
	}

	return_entry->entry_count = cnt;
	return_entry->prev_handle = p_temp;

	ca_spin_unlock(sw_rate_limiter_lock);

	if (cnt == 0) {
		ca_printf("%s: No data is available!!!\n", __func__);
		return CA_E_UNAVAIL;
	}

	return CA_E_OK;
}

void ca_sw_rate_limiter_table_dump(void)
{
        ca_iterator_t   iterator;
        ca_uint32_t     count;
        ca_status_t     ret;
        int i, index;
	ca_sw_rate_limiter_entry_t sw_rate_limiter_entry;

        /* get first nexthop entry */
	memset(&iterator, 0, sizeof(ca_iterator_t));
        iterator.prev_handle = NULL;
        count = 1;
        iterator.entry_count = count;
        iterator.entry_data = &sw_rate_limiter_entry;
        iterator.user_buffer_size = sizeof(ca_sw_rate_limiter_entry_t);

        index = 0;
        while (1) {
                ret = ca_sw_rate_limiter_entry_iterate(0, &iterator);
                if (ret != CA_E_OK || iterator.entry_count == 0)
                        break;

                ca_printf("================= index=%d  =====================\n", index);

		ca_printf("sw_rate_limiter_id=%d, dest_VoQ=%d, classifier_index=%d\n",
			sw_rate_limiter_entry.sw_rate_limiter_id, sw_rate_limiter_entry.dest_VoQ, sw_rate_limiter_entry.classifier_index);

		ca_printf("flow_id=%d, f_default_limiter=%d, rate=%d\n", sw_rate_limiter_entry.flow_id,
			sw_rate_limiter_entry.f_default_limiter, sw_rate_limiter_entry.rate);

		/* end of list */
		if (iterator.prev_handle == NULL) {
			break;
		}
                index++;
        }
	ca_printf("\n\n");
}

EXPORT_SYMBOL(ca_sw_rate_limiter_entry_add);
EXPORT_SYMBOL(ca_sw_rate_limiter_entry_delete);
EXPORT_SYMBOL(ca_sw_rate_limiter_delete_all);
EXPORT_SYMBOL(ca_sw_rate_limiter_entry_get);
EXPORT_SYMBOL(ca_sw_rate_limiter_entry_iterate);
