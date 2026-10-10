#include <soc/cortina/ca_ipc_pe.h>

#include "osal_cmn.h"
#include "osal_common.h"
#include "aal_common.h"
#include "ca-scfg/scfg.h"
#include "port.h"
#include "sw_shaper.h"

#define SW_SHAPER_IPC_MAX			4
#define SW_SHAPER_IPC_INIT_DEFAULT_SHAPER	0
#define SW_SHAPER_IPC_ADD_ENTRY			1
#define SW_SHAPER_IPC_DEL_ENTRY			2
#define SW_SHAPER_IPC_DEL_ALL_ENTRY		3

typedef struct ca_sw_shaper_tx_frame_list {
        struct ca_sw_shaper_tx_frame_list *p_next;
        ca_uint8_t              *p_data;
        ca_uint32_t             len;
} ca_sw_shaper_tx_frame_list_t;

typedef struct ca_sw_shaper_cb {
    struct ca_sw_shaper_cb      *p_next;
    ca_sw_shaper_entry_t        sw_shaper;
    ca_uint32_t                 token_per_msec;
    ca_uint32_t                 prev_cycle;
    ca_int32_t                  current_token_count;
    ca_uint32_t                 burst_token_per_msec;
    ca_sw_shaper_tx_frame_list_t        *p_tx_frame_lis;
} ca_sw_shaper_cb_t;

//static ca_sw_shaper_entry_t *sw_shaper_list[CA_SW_SHAPER_MAX];
static ca_uint sw_shaper_lock;
static ca_uint8_t sw_shpaer_init_done = 0;
static ca_uint8_t		g_sw_shaper_enable = 0;

static ca_sw_shaper_cb_t	*p_sw_shaper_list = NULL;
//static ca_sw_shaper_cb_t	default_sw_shaper;
static ca_uint32_t		g_default_shaper_token_count_per_msec = TOKEN_COUNT_FOR_MAX_RATE / SEC_TO_MSEC(1);
static ca_uint32_t		g_orig_default_shaper_token_count_per_msec = TOKEN_COUNT_FOR_MAX_RATE / SEC_TO_MSEC(1);
static ca_uint32_t		g_max_shaper_supported = CA_SW_SHAPER_MAX;

/* IPC communicate with Taroko */
#ifdef CONFIG_CORTINA_IPC2RCPU
static int ca_sw_shaper_ipc_debug_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
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

static ca_ipc_msg_handle_t sw_shaper_ipc_msg[SW_SHAPER_IPC_MAX] = {
	{ .msg_no = SW_SHAPER_IPC_INIT_DEFAULT_SHAPER, .proc = ca_sw_shaper_ipc_debug_handle},
	{ .msg_no = SW_SHAPER_IPC_ADD_ENTRY, .proc = ca_sw_shaper_ipc_debug_handle},
	{ .msg_no = SW_SHAPER_IPC_DEL_ENTRY, .proc = ca_sw_shaper_ipc_debug_handle},
	{ .msg_no = SW_SHAPER_IPC_DEL_ALL_ENTRY, .proc = ca_sw_shaper_ipc_debug_handle},
};

static ca_status_t ca_sw_shaper_ipc_init(ca_uint32_t max_shaper_supported, ca_uint32_t default_shaper_token_count_per_msec)
{
        ca_status_t ret;
        char rcv_buff[16];
        ca_uint16_t result_size = 16;
        ca_ipc_pkt_t ipc_pkt;
        ca_uint32_t i;
        ca_sw_shaper_ipc_init_t ipc_init_entry;

	printk("%s: max_shaper_supported=%d, default_shaper_token_count_per_msec=%d\n", __func__,
			max_shaper_supported, default_shaper_token_count_per_msec);

        ipc_init_entry.default_shaper_token_count_per_msec = default_shaper_token_count_per_msec;
	ipc_init_entry.max_shaper_supported = max_shaper_supported;

        ipc_pkt.session_id = CA_IPC_SESSION_PKT_SHAPER;
        ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
        ipc_pkt.priority = 0;
        ipc_pkt.msg_no = SW_SHAPER_IPC_INIT_DEFAULT_SHAPER;
        ipc_pkt.msg_data = &ipc_init_entry;               	/* the message data to transmit. */
        ipc_pkt.msg_size = sizeof(ca_sw_shaper_ipc_init_t);    	/* the length of message data. */
        ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
        if (ret != CA_E_OK) {
                ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
                return ret;
        }

	g_orig_default_shaper_token_count_per_msec = g_default_shaper_token_count_per_msec = default_shaper_token_count_per_msec;
	g_max_shaper_supported = max_shaper_supported;

        return CA_E_OK;
}

static ca_status_t ca_sw_shaper_ipc_add_entry(ca_sw_shaper_cb_t *shaper_cb)
{
	ca_status_t ret;
	char rcv_buff[16];
	ca_uint16_t result_size = 16;
	ca_ipc_pkt_t ipc_pkt;
	ca_uint32_t i;
	ca_sw_shaper_ipc_entry_t ipc_entry;

	ipc_entry.sw_shaper_id = shaper_cb->sw_shaper.sw_shaper_id;
	ipc_entry.token_per_msec = shaper_cb->token_per_msec;
	ipc_entry.default_shaper_token_count_per_msec = g_default_shaper_token_count_per_msec;
	ipc_entry.burst_token_per_msec = shaper_cb->burst_token_per_msec;

	ipc_pkt.session_id = CA_IPC_SESSION_PKT_SHAPER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = SW_SHAPER_IPC_ADD_ENTRY;
	ipc_pkt.msg_data = &ipc_entry; 				/* the message data to transmit. */
	ipc_pkt.msg_size = sizeof(ca_sw_shaper_ipc_entry_t); 	/* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

static ca_status_t ca_sw_shaper_ipc_del_entry(ca_uint16_t sw_shaper_id)
{
        ca_status_t ret;
        char rcv_buff[16];
        ca_uint16_t result_size = 16;
        ca_ipc_pkt_t ipc_pkt;
        ca_uint32_t i;
        ca_sw_shaper_ipc_entry_t ipc_entry;

	memset(&ipc_entry, 0,  sizeof(ca_sw_shaper_ipc_entry_t));
        ipc_entry.sw_shaper_id = sw_shaper_id;

        ipc_pkt.session_id = CA_IPC_SESSION_PKT_SHAPER;
        ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
        ipc_pkt.priority = 0;
        ipc_pkt.msg_no = SW_SHAPER_IPC_DEL_ENTRY;
        ipc_pkt.msg_data = &ipc_entry; /* the message data to transmit. */
        ipc_pkt.msg_size = sizeof(ca_sw_shaper_ipc_entry_t); /* the length of message data. */
        ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
        if (ret != CA_E_OK) {
                ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
                return ret;
        }

        return CA_E_OK;
}

static ca_status_t ca_sw_shaper_ipc_del_all_entry(void)
{
        ca_status_t ret;
        char rcv_buff[16];
        ca_uint16_t result_size = 16;
        ca_ipc_pkt_t ipc_pkt;
        ca_uint32_t i;
        ca_sw_shaper_ipc_entry_t ipc_entry;

	memset(&ipc_entry, 0,  sizeof(ca_sw_shaper_ipc_entry_t));
        ipc_entry.sw_shaper_id = 0;

        ipc_pkt.session_id = CA_IPC_SESSION_PKT_SHAPER;
        ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
        ipc_pkt.priority = 0;
        ipc_pkt.msg_no = SW_SHAPER_IPC_DEL_ALL_ENTRY;
        ipc_pkt.msg_data = &ipc_entry; /* the message data to transmit. */
        ipc_pkt.msg_size = sizeof(ca_sw_shaper_ipc_entry_t); /* the length of message data. */
        ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
        if (ret != CA_E_OK) {
                ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
                return ret;
        }

        return CA_E_OK;
}
#endif

ca_status_t ca_sw_shaper_init(void)
{
	ca_status_t ret;
	ca_uint8_t	sw_shaper_enable;
	ca_uint32_t	max_sw_shapers_supported;
	ca_uint32_t	sw_shaper_max_rate;

	if (sw_shpaer_init_done)
		return CA_E_OK;

	ret = aal_scfg_read(CFG_ID_SW_SHAPER_ENABLE, 1, &sw_shaper_enable);
	if (ret == CA_E_OK) {
		g_sw_shaper_enable = sw_shaper_enable;
	}
	else {
		printk("%s: failed to read CFG_ID_SW_SHAPER_ENABLE, ret=%d\n", __func__, ret);
		return CA_E_OK;
	}

	if (!sw_shaper_enable)
		return CA_E_OK;

	ret = aal_scfg_read(CFG_ID_MAX_SW_SHAPERS_SUPPORTED, 4, &max_sw_shapers_supported);
	if (ret == CA_E_OK) {
		g_max_shaper_supported = max_sw_shapers_supported;
	}
	else {
		printk("%s: failed to read CFG_ID_MAX_SW_SHAPERS_SUPPORTED, use default value, ret=%d\n", __func__, ret);
	}

	ret = aal_scfg_read(CFG_ID_SW_SHAPER_MAX_RATE, 4, &sw_shaper_max_rate);
	if (ret == CA_E_OK) {
		/* the unit of sh_shaper_max_rate is 1Kbps */
                g_default_shaper_token_count_per_msec = RATE_TO_TOKEN_COUNT((ca_uint_t)sw_shaper_max_rate * 1024) / SEC_TO_MSEC(1);
	}
	else {
		printk("%s: failed to read CFG_ID_SW_SHAPER_MAX_RATE, use default value, ret=%d\n", __func__, ret);
	}
	printk("%s: MAX_SW_SHAPERS_SUPPORTED=%d, SW_SHAPER_MAX_RATE=%d\n", __func__, max_sw_shapers_supported, sw_shaper_max_rate);
	printk("%s: g_max_shaper_supported=%d, g_default_shaper_token_count_per_msec=%d\n", __func__, g_max_shaper_supported, g_default_shaper_token_count_per_msec);

        if (CA_OSAL_OK != ca_spin_lock_init(&sw_shaper_lock, SPINLOCK_CMN)) {
                ca_printf("%s: init spinlock fail\n", __func__);
                return CA_E_INIT;
        }
        //memset(p_sw_shaper_list, 0, sizeof(sw_shaper_list));

#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_ipc_msg_handle_register(CA_IPC_SESSION_PKT_SHAPER, sw_shaper_ipc_msg, SW_SHAPER_IPC_MAX);

	if (ret != CA_E_OK) {
		ca_printf("%s: ca-ipc_msg_handle_register() fail ret %d\n", __func__, ret);
	}

	///TODO: debug, need get the max shaper supported and max rate support for default shaper from startup config
	ca_sw_shaper_ipc_init(g_max_shaper_supported, g_default_shaper_token_count_per_msec);
#endif

	sw_shpaer_init_done = 1;
	return CA_E_OK;
}

static ca_uint32_t ca_get_sum_tokens_per_msec_for_all_shapers(void)
{
	ca_sw_shaper_cb_t *p_shaper_temp = NULL;
	ca_uint32_t total_token_per_msec = 0;

	p_shaper_temp = p_sw_shaper_list;
	while (p_shaper_temp) {
		total_token_per_msec += p_shaper_temp->token_per_msec;
		p_shaper_temp = p_shaper_temp->p_next;
	}
	return total_token_per_msec;
}

static ca_status_t ca_sw_shaper_validate_shaper(ca_sw_shaper_entry_t *p_sw_shaper)
{
	/* the shaper id equal to g_max_shaper_supported - 1 ir reserved for default shaper */
	if (p_sw_shaper->sw_shaper_id >= g_max_shaper_supported - 1) {
		ca_printf("%s: sw_shaper_id=%d is larger than %d!!\n", __func__, p_sw_shaper->sw_shaper_id, g_max_shaper_supported - 1);
		return CA_E_PARAM;
	}
	if (p_sw_shaper->prio > 7) {
		ca_printf("%s: prio=%d is larger than 7!!\n", __func__, p_sw_shaper->prio);
		return CA_E_PARAM;
	}
	if (!CA_PORT_VALID_TEST(p_sw_shaper->internal_port, CA_PORT_TYPE_CPU, CA_PORT_ID_CPU_MQ_MIN, CA_PORT_ID_CPU_MQ_MAX)) {
		ca_printf("%s: port id 0x%x is not correct!!\n", __func__, p_sw_shaper->internal_port);
		return CA_E_PARAM;
	}

	return CA_E_OK;
}

ca_status_t ca_sw_shaper_entry_add (
    CA_IN  ca_device_id_t 		device_id,
    CA_OUT ca_sw_shaper_entry_t	*p_sw_shaper)
{
	int i;
	ca_status_t ret;
	ca_sw_shaper_entry_t *p_entry;
	ca_sw_shaper_cb_t *p_shaper_temp = NULL, *p_temp = NULL;

	if (!g_sw_shaper_enable) {
		return CA_E_INACTIVE;
	}

	if (p_sw_shaper == NULL) {
		ca_printf("%s: p_sw_shaper is NULL!!\n", __func__);
		return CA_E_PARAM;
	}

	ret = ca_sw_shaper_validate_shaper(p_sw_shaper);
	if (ret != CA_E_OK) {
		return ret;
	}

#if 0
	/* check if sw shaper list has been initialized */
	if (CA_E_OK != ca_sw_shaper_list_init())
		return CA_E_INIT;
#endif

	ca_printf("%s: sw_shaper_id=%d, internal_port=0x%x, prio=%d, rate=%d\n",
			__func__, p_sw_shaper->sw_shaper_id, p_sw_shaper->internal_port, p_sw_shaper->prio, p_sw_shaper->shaper.rate);

	ca_spin_lock(sw_shaper_lock);

	/* search whether shaper is already occupied */
	i = 0;
	p_shaper_temp = p_sw_shaper_list;
	p_temp = NULL;
	while (p_shaper_temp) {
		if (p_shaper_temp->sw_shaper.sw_shaper_id == p_sw_shaper->sw_shaper_id) {
			ca_printf("%s: sw_shaper_id=%d already exists!!!\n", __func__, p_sw_shaper->sw_shaper_id);
			ca_spin_unlock(sw_shaper_lock);
			return CA_E_EXISTS;
		}
		p_temp = p_shaper_temp;
		p_shaper_temp = p_shaper_temp->p_next;
		i++;
	}

	/* the shaper id equal to g_max_shaper_supported - 1 is reserved for default shaper */
	if (i > g_max_shaper_supported - 1) {
		ca_printf("%s: software shaper entry is full!!!\n", __func__);
		ca_spin_unlock(sw_shaper_lock);
		return CA_E_FULL;
	}

	p_shaper_temp = ca_malloc(sizeof(ca_sw_shaper_cb_t));
	if (p_shaper_temp == NULL) {
		ca_printf("%s: can not allocate shaper because OOM!!!\n", __func__);
		ca_spin_unlock(sw_shaper_lock);
                return CA_E_MEMORY;
	}

        memcpy(&(p_shaper_temp->sw_shaper), p_sw_shaper, sizeof(ca_sw_shaper_entry_t));
	if (!p_sw_shaper_list) {
		p_sw_shaper_list = p_shaper_temp;
	}
	else {
		p_temp->p_next = p_shaper_temp;		/* p_temp point to the last entry of list */
	}

	/* the unit of rate is 1Kbps */
	p_shaper_temp->token_per_msec = RATE_TO_TOKEN_COUNT(p_sw_shaper->shaper.rate * 1024) / SEC_TO_MSEC(1);
	p_shaper_temp->burst_token_per_msec = (p_sw_shaper->shaper.burst_size) / SEC_TO_MSEC(1);

	g_default_shaper_token_count_per_msec = g_orig_default_shaper_token_count_per_msec - ca_get_sum_tokens_per_msec_for_all_shapers();

	ca_spin_unlock(sw_shaper_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
	ca_sw_shaper_ipc_add_entry(p_shaper_temp);
#endif

        return CA_E_OK;
}

ca_status_t ca_sw_shaper_entry_delete (
    CA_IN  ca_device_id_t  device_id,
    CA_OUT ca_uint16_t  sw_shaper_id)
{
        int i;
        ca_sw_shaper_cb_t *p_shaper_temp;

        if (!g_sw_shaper_enable) {
                return CA_E_INACTIVE;
        }

	ca_printf("%s: sw_shaper_id=%d\n", __func__, sw_shaper_id);

#if 0
	/* check if sw shaper list has been initialized */
	if (CA_E_OK != ca_sw_shaper_list_init())
		return CA_E_INIT;
#endif

	ca_spin_lock(sw_shaper_lock);

        /* find the sw shaper entry to be deleted*/
	p_shaper_temp = p_sw_shaper_list;
	while (p_shaper_temp) {
		if (p_shaper_temp->sw_shaper.sw_shaper_id == sw_shaper_id) {
			ca_printf("%s: Got sw shaper entry\n", __func__);
			break;
		}
		p_shaper_temp = p_shaper_temp->p_next;
        }
        if (!p_shaper_temp) {
		ca_spin_unlock(sw_shaper_lock);
                return CA_E_NOT_FOUND;
	}

	ca_free(p_shaper_temp);

	g_default_shaper_token_count_per_msec = g_orig_default_shaper_token_count_per_msec - ca_get_sum_tokens_per_msec_for_all_shapers();

	ca_spin_unlock(sw_shaper_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
        ca_sw_shaper_ipc_del_entry(sw_shaper_id);
#endif

        return CA_E_OK;
}

ca_status_t ca_sw_shaper_delete_all (
    CA_IN  ca_device_id_t device_id)
{
	int i;
        ca_sw_shaper_cb_t *p_shaper_temp, *p_next;

	if (!g_sw_shaper_enable) {
		return CA_E_INACTIVE;
	}

	ca_spin_lock(sw_shaper_lock);
	/* find the sw shaper entry to be deleted*/
	p_shaper_temp = p_sw_shaper_list;
	while (p_shaper_temp) {
		p_next = p_shaper_temp->p_next;
		ca_free(p_shaper_temp);
		p_shaper_temp = p_next;
	}

	g_default_shaper_token_count_per_msec = g_orig_default_shaper_token_count_per_msec;

	ca_spin_unlock(sw_shaper_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
        ca_sw_shaper_ipc_del_all_entry();
#endif

	return CA_E_OK;
}

ca_status_t ca_sw_shaper_entry_get (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint16_t	sw_shaper_id,
    CA_OUT ca_sw_shaper_entry_t	*p_sw_shaper)
{
        int i;
	ca_status_t ret;
        ca_sw_shaper_cb_t *p_shaper_temp;

        if (!g_sw_shaper_enable) {
                return CA_E_INACTIVE;
        }

        if (p_sw_shaper == NULL) {
		ca_printf("%s: p_sw_shaper is NULL!!!\n", __func__);
                return CA_E_PARAM;
        }

	/* can get the default shaper id */
	/* the shaper id equal to g_max_shaper_supported - 1 is reserved for default shaper */
	if (sw_shaper_id >= g_max_shaper_supported - 1) {
		ca_printf("%s: sw_shaper_id exceed %d!!!\n", __func__, g_max_shaper_supported);
		return CA_E_PARAM;
	}

#if 0
	/* check if sw shaper list has been initialized */
	if (CA_E_OK != ca_sw_shaper_list_init())
		return CA_E_INIT;
#endif

	ca_spin_lock(sw_shaper_lock);

	p_shaper_temp = p_sw_shaper_list;
        while (p_shaper_temp) {
                if (p_shaper_temp->sw_shaper.sw_shaper_id == sw_shaper_id) {
			break;
                }
                p_shaper_temp = p_shaper_temp->p_next;
        }

	if (p_shaper_temp != NULL) {
		memcpy(p_sw_shaper, &(p_shaper_temp->sw_shaper), sizeof(ca_sw_shaper_entry_t));
		ret = CA_E_OK;
	}
	else {
		ca_printf("%s: sw_shaper_id=%d not found!!!\n", __func__, sw_shaper_id);
		ret = CA_E_NOT_FOUND;
	}

	ca_spin_unlock(sw_shaper_lock);

        return ret;
}

ca_status_t ca_sw_shaper_entry_iterate (
    CA_IN  ca_device_id_t 	 device_id,
    CA_OUT ca_iterator_t *return_entry)
{
	ca_uint32_t i, start, e_sz, cnt;
	ca_sw_shaper_entry_t *p_entry = return_entry->entry_data;
	ca_int32_t b_sz = return_entry->user_buffer_size;
	ca_sw_shaper_cb_t *p_shaper_temp;

        if (!g_sw_shaper_enable) {
                return CA_E_INACTIVE;
        }

	if (return_entry == NULL || return_entry->entry_data == NULL)
		return CA_E_PARAM;

	e_sz = sizeof(ca_sw_shaper_entry_t);
	//ca_printf("%s: return_entry->prev_handle=%p\n", __func__, return_entry->prev_handle);
	if (return_entry->prev_handle == NULL) {
		p_shaper_temp = p_sw_shaper_list;
	} else {
		p_shaper_temp = return_entry->prev_handle;
	}

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	ca_spin_lock(sw_shaper_lock);

	/* the shaper id equal to g_max_shaper_supported - 1 is reserved for default shaper */
	for (i = 0, cnt = 0; (p_shaper_temp != NULL) && (i < g_max_shaper_supported) && (b_sz >= e_sz) && (cnt < return_entry->entry_count); i++) {
                //ca_printf("%s: p_shaper_temp=%p, i=%d, cnt=%d, b_sz=%d, e_sz=%d\n", __func__, p_shaper_temp, i, cnt, b_sz, e_sz);

		if (p_shaper_temp == NULL)
			break;
		/* copy data */
		memcpy(p_entry, &(p_shaper_temp->sw_shaper), sizeof(ca_sw_shaper_entry_t));
		b_sz -= e_sz;
		cnt++;
		p_entry++;

		p_shaper_temp = p_shaper_temp->p_next;
	}

	return_entry->entry_count = cnt;
	return_entry->prev_handle = p_shaper_temp;

	ca_spin_unlock(sw_shaper_lock);

	if (cnt == 0) {
		ca_printf("%s: No data is available!!!\n", __func__);
		return CA_E_UNAVAIL;
	}

	return CA_E_OK;
}

void ca_sw_shaper_table_dump(void)
{
        ca_iterator_t   iterator;
        ca_uint32_t     count;
        ca_status_t     ret;
        int i, index;
	ca_sw_shaper_entry_t sw_shaper_entry;

        /* get first nexthop entry */
	memset(&iterator, 0, sizeof(ca_iterator_t));
        iterator.prev_handle = NULL;
        count = 1;
        iterator.entry_count = count;
        iterator.entry_data = &sw_shaper_entry;
        iterator.user_buffer_size = sizeof(ca_sw_shaper_entry_t);

        index = 0;
        while (1) {
                ret = ca_sw_shaper_entry_iterate(0, &iterator);
                if (ret != CA_E_OK || iterator.entry_count == 0)
                        break;

                ca_printf("================= index=%d  =====================\n", index);

		ca_printf("sw_shaper_id=%d, internal_port=0x%x, prio=%d\n",
				sw_shaper_entry.sw_shaper_id, sw_shaper_entry.internal_port, sw_shaper_entry.prio);

		ca_printf("[Shaper]\n");
		ca_printf("	enable=%d, pps=%d, rate=%d(Kbits), burst_size=%d\n", sw_shaper_entry.shaper.enable, sw_shaper_entry.shaper.pps, sw_shaper_entry.shaper.rate,
				sw_shaper_entry.shaper.burst_size);

		/* end of list */
		if (iterator.prev_handle == NULL) {
			break;
		}
                index++;
        }
	ca_printf("\n\n");
}

EXPORT_SYMBOL(ca_sw_shaper_entry_add);
EXPORT_SYMBOL(ca_sw_shaper_entry_delete);
EXPORT_SYMBOL(ca_sw_shaper_delete_all);
EXPORT_SYMBOL(ca_sw_shaper_entry_get);
EXPORT_SYMBOL(ca_sw_shaper_entry_iterate);
