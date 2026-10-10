#include <soc/cortina/ca_ipc_pe.h>

#include "osal_cmn.h"
#include "osal_common.h"
#include "aal_common.h"
#include "ca-scfg/scfg.h"
#include "port.h"
#include "aal_port.h"
#include "ca_us_rate_manager.h"

#define CA_US_RATE_MANAGER_IPC_MAX			5
#define CA_US_RATE_MANAGER_IPC_GLB_CONFIG_SET		0
#define CA_US_RATE_MANAGER_IPC_ADD_ENTRY		1
#define CA_US_RATE_MANAGER_IPC_DEL_ENTRY		2
#define CA_US_RATE_MANAGER_IPC_DEL_ALL_ENTRY		3
#define CA_US_RATE_MANAGER_IPC_DUMP			4

typedef struct ca_us_rate_manager_cb {
	struct ca_us_rate_manager_cb 		*p_next;
	ca_uint8_t		     		app_stream_id;
	us_rate_manager_ipc_app_spec_stream_t  	entry;
} ca_us_rate_manager_cb_t;

static ca_uint 			g_us_rate_manager_lock;
static ca_uint8_t 		g_us_rate_manager_init_done = 0;
static ca_uint8_t		g_us_rate_manager_enable = 0;
//static ca_uint32_t		g_us_rate_manager_max_rate = 0;
static ca_uint32_t		g_us_rate_manager_max_supported = CA_US_RATE_MANAGER_MAX;
//static ca_uint32_t		g_us_rate_manager_default_rate = CA_MAX_PE_FWD_RATE;

static us_rate_manager_global_control_t		g_us_rate_manager_global_control;

static ca_us_rate_manager_cb_t	*p_us_rate_manager_list = NULL;

static ca_uint8_t	g_us_rate_manager_voq_list[CA_US_RATE_MANAGER_MAX];

static ca_uint8_t	g_us_rate_manager_wan_port_id;

/* IPC communicate with Taroko */
#ifdef CONFIG_CORTINA_IPC2RCPU
static int ca_us_rate_manager_ipc_debug_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
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

static ca_ipc_msg_handle_t us_rate_manager_ipc_msg[CA_US_RATE_MANAGER_IPC_MAX] = {
	{ .msg_no = CA_US_RATE_MANAGER_IPC_GLB_CONFIG_SET, 	.proc = ca_us_rate_manager_ipc_debug_handle},
	{ .msg_no = CA_US_RATE_MANAGER_IPC_ADD_ENTRY, 		.proc = ca_us_rate_manager_ipc_debug_handle},
	{ .msg_no = CA_US_RATE_MANAGER_IPC_DEL_ENTRY, 		.proc = ca_us_rate_manager_ipc_debug_handle},
	{ .msg_no = CA_US_RATE_MANAGER_IPC_DEL_ALL_ENTRY, 	.proc = ca_us_rate_manager_ipc_debug_handle},
	{ .msg_no = CA_US_RATE_MANAGER_IPC_DUMP, 		.proc = ca_us_rate_manager_ipc_debug_handle},
};

static ca_status_t ca_us_rate_manager_ipc_global_config_set(us_rate_manager_global_control_t *p_config)
{
	ca_status_t ret;
	char rcv_buff[sizeof(us_rate_manager_global_control_t)];
	ca_uint16_t result_size = sizeof(us_rate_manager_global_control_t);
	ca_ipc_pkt_t ipc_pkt;

	ca_printf("%s: Enter...\n", __func__);

	ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_US_RATE_MANAGER_IPC_GLB_CONFIG_SET;
	ipc_pkt.msg_data = p_config;               				/* the message data to transmit. */
	ipc_pkt.msg_size = sizeof(us_rate_manager_global_control_t);    	/* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

static ca_status_t ca_us_rate_manager_ipc_add_entry(us_rate_manager_ipc_app_spec_stream_t *p_stream)
{
	ca_status_t ret;
	char rcv_buff[sizeof(us_rate_manager_ipc_app_spec_stream_t)];
	ca_uint16_t result_size = sizeof(us_rate_manager_ipc_app_spec_stream_t);
	ca_ipc_pkt_t ipc_pkt;

	ca_printf("%s: Enter...\n", __func__);

	ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_US_RATE_MANAGER_IPC_ADD_ENTRY;
	ipc_pkt.msg_data = p_stream; 					/* the message data to transmit. */
	ipc_pkt.msg_size = sizeof(us_rate_manager_ipc_app_spec_stream_t); 	/* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

static ca_status_t ca_us_rate_manager_ipc_del_entry(ca_uint8_t app_stream_id)
{
	ca_status_t ret;
	char rcv_buff[16];
	ca_uint16_t result_size = 16;
	ca_ipc_pkt_t ipc_pkt;

	ca_printf("%s: Enter...\n", __func__);

	//memset(&ipc_entry, 0,  sizeof(ca_us_rate_manager_ipc_entry_t));

	/* the dest_VoQ here should be converted to 0-128 for CPU256 VoQ */
	//ipc_entry.dest_VoQ = dest_VoQ - 256;

	ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_US_RATE_MANAGER_IPC_DEL_ENTRY;
	ipc_pkt.msg_data = &app_stream_id; 				/* the message data to transmit. */
	ipc_pkt.msg_size = sizeof(ca_uint8_t); 	/* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

static ca_status_t ca_us_rate_manager_ipc_del_all_entry(void)
{
	ca_status_t ret;
	char rcv_buff[16];
	ca_uint16_t result_size = 16;
	ca_ipc_pkt_t ipc_pkt;

	ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_US_RATE_MANAGER_IPC_DEL_ALL_ENTRY;
	ipc_pkt.msg_data = NULL; /* the message data to transmit. */
	ipc_pkt.msg_size = 0; /* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

static ca_status_t ca_us_rate_manager_ipc_dump(void)
{
	ca_status_t ret;
	char rcv_buff[16];
	ca_uint16_t result_size = 16;
	ca_ipc_pkt_t ipc_pkt;

	ipc_pkt.session_id = CA_IPC_SESSION_SW_RATE_LIMITER;
	ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE0;
	ipc_pkt.priority = 0;
	ipc_pkt.msg_no = CA_US_RATE_MANAGER_IPC_DUMP;
	ipc_pkt.msg_data = NULL; /* the message data to transmit. */
	ipc_pkt.msg_size = 0; /* the length of message data. */
	ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &result_size);
	if (ret != CA_E_OK) {
		ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}

#endif

ca_status_t ca_us_rate_manager_global_configuration_set(
		CA_IN   ca_device_id_t                          device_id,
		CA_IN   us_rate_manager_global_control_t        *p_config)
{
	int i;
	if(p_config->max_rate_manager_app_spec_streams != 248 && p_config->max_rate_manager_app_spec_streams != 256) {
		printk("%s: %d streams not support\n", __func__, p_config->max_rate_manager_app_spec_streams);
		return CA_E_NOT_SUPPORT;
	}

	printk("%s: max_rate_manager_app_spec_streams=%d, default_stream_bandwidth_percentage=%d, remaining_bandwidth_distribution_scheme=%d\n", __func__,
			p_config->max_rate_manager_app_spec_streams, p_config->default_stream_bandwidth_percentage, p_config->remaining_bandwidth_distribution_scheme);
	printk("%s: default_stream_scheduler=%d, max_egress_rate=%d\n", __func__, p_config->default_stream_scheduler, p_config->max_egress_rate);
	printk("%s: default_stream_voq_weigths[0-7]=", __func__);
	for (i = 0; i < 8; i++) {
		printk("%d-", p_config->default_stream_voq_weigths[i]);
	}
	printk("\n");

	ca_spin_lock(g_us_rate_manager_lock);

	g_us_rate_manager_global_control = *p_config;

	ca_spin_unlock(g_us_rate_manager_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
	ca_us_rate_manager_ipc_global_config_set(p_config);
#endif

	return CA_E_OK;
}

ca_status_t     ca_us_rate_manager_global_configuration_get (
		CA_IN   ca_device_id_t                          device_id,
		CA_IN   us_rate_manager_global_control_t        *p_config)
{
	int i;

	ca_spin_lock(g_us_rate_manager_lock);

	*p_config = g_us_rate_manager_global_control;

	ca_spin_unlock(g_us_rate_manager_lock);

	printk("%s: max_rate_manager_app_spec_streams=%d, default_stream_bandwidth_percentage=%d, remaining_bandwidth_distribution_scheme=%d\n", __func__,
			p_config->max_rate_manager_app_spec_streams, p_config->default_stream_bandwidth_percentage, p_config->remaining_bandwidth_distribution_scheme);
	printk("%s: default_stream_scheduler=%d, max_egress_rate=%d\n", __func__, p_config->default_stream_scheduler, p_config->max_egress_rate);
	printk("%s: default_stream_voq_weigths[0-7]=", __func__);
	for (i = 0; i < 8; i++) {
		printk("%d-", p_config->default_stream_voq_weigths[i]);
	}
	printk("\n");

	return CA_E_OK;
}

ca_status_t ca_us_rate_manager_init(void)
{
	ca_status_t ret;
	//ca_uint32_t     us_rate_manager_default_rate;
	//ca_uint32_t     us_rate_manager_max_rate;

	ca_printf("%s: g_us_rate_manager_init_done=%d\n", __func__, g_us_rate_manager_init_done);

	if (g_us_rate_manager_init_done)
		return CA_E_OK;

	if (CA_OSAL_OK != ca_spin_lock_init(&g_us_rate_manager_lock, SPINLOCK_CMN)) {
		ca_printf("%s: init spinlock fail\n", __func__);
		return CA_E_INIT;
	}

#if defined(CONFIG_RG_G3_WAN_PORT_INDEX)
	g_us_rate_manager_wan_port_id = CONFIG_RG_G3_WAN_PORT_INDEX;
#else
	aal_scfg_read(CFG_ID_WAN_PORT_ID, 1, &g_us_rate_manager_wan_port_id);
#endif

	memset(&g_us_rate_manager_voq_list[0], 0, CA_US_RATE_MANAGER_MAX);

	g_us_rate_manager_enable = 1;
	g_us_rate_manager_init_done = 1;

#ifdef CONFIG_CORTINA_IPC2RCPU
	ret = ca_ipc_msg_handle_register(CA_IPC_SESSION_SW_RATE_LIMITER, us_rate_manager_ipc_msg, CA_US_RATE_MANAGER_IPC_MAX);

	if (ret != CA_E_OK) {
		ca_printf("%s: ca-ipc_msg_handle_register() fail ret %d\n", __func__, ret);
	}
#endif
	return CA_E_OK;
}

ca_status_t ca_us_rate_manager_entry_add (
		CA_IN  ca_device_id_t 		device_id,
		CA_IN  us_rate_manager_app_spec_stream_t	*p_rate_mgr)
{
	int i, has_default, found_entry;
	//ca_uint32_t dest_voq;
	ca_us_rate_manager_cb_t *p_temp = NULL;
	ca_us_rate_manager_cb_t *p_last_entry = NULL;
	ca_us_rate_manager_cb_t *p_prev_entry = NULL;

	if (!g_us_rate_manager_enable) {
		return CA_E_INACTIVE;
	}

	ca_printf("%s: app_stream_id=%d, cir_kbps=%d, pir_kbpsr=%d, strict_prio=%d\n", __func__,
			p_rate_mgr->app_stream_id, p_rate_mgr->cir_kbps, p_rate_mgr->pir_kbps, p_rate_mgr->strict_prio);

	if (p_rate_mgr == NULL) {
		ca_printf("%s: p_rate_mgr == NULL !!\n", __func__);
		return CA_E_PARAM;
	}

	if (p_rate_mgr->app_stream_id > CA_US_RATE_MANAGER_MAX) {
		ca_printf("%s: app_stram_id > %d!!\n", __func__, CA_US_RATE_MANAGER_MAX);
		return CA_E_PARAM;
	}

	ca_spin_lock(g_us_rate_manager_lock);

	///TODO: debug
	/* check if there is enough rate left */

	/* search whether us rate manager is already occupied */
	i = 0;
	has_default = 0;
	found_entry = 0;
	p_temp = p_us_rate_manager_list;
	while (p_temp) {
#if 0
		if (p_temp->entry.f_default_manager) {
			has_default = 1;
		}
		if (has_default && p_rate_mgr->f_default_manager) {
			ca_printf("%s: default us rate manager already exists!!!\n", __func__);
			ca_spin_unlock(g_us_rate_manager_lock);
			return CA_E_EXISTS;
		}
#endif

		if (p_rate_mgr->app_stream_id == p_temp->entry.app_spec_stream.app_stream_id) {
			ca_printf("%s: Stream ID already exists!!!\n", __func__);
			ca_spin_unlock(g_us_rate_manager_lock);
			return CA_E_EXISTS;
		}

		/* the highest priority should be at top */
		if (p_rate_mgr->app_stream_id < p_temp->entry.app_spec_stream.app_stream_id) {
			p_last_entry = p_temp;          /* record the last one entry */
		}
		else {
			if (!found_entry) {
				p_prev_entry = p_last_entry;
				p_last_entry = p_temp;
			}
			found_entry = 1;
		}
		p_temp = p_temp->p_next;
		i++;
	}

	/* the shaper id equal to g_us_rate_manager_max_supported - 1 is reserved for default shaper */
	if (i > g_us_rate_manager_max_supported - 1) {
		ca_printf("%s: software us rate manager entry is full!!!\n", __func__);
		ca_spin_unlock(g_us_rate_manager_lock);
		return CA_E_FULL;
	}

	p_temp = ca_malloc(sizeof(ca_us_rate_manager_cb_t));
	if (p_temp == NULL) {
		ca_printf("%s: can not allocate shaper because OOM!!!\n", __func__);
		ca_spin_unlock(g_us_rate_manager_lock);
		return CA_E_MEMORY;
	}

	if (!p_us_rate_manager_list) {
		p_us_rate_manager_list = p_temp;
	}
	else {
		/* priority higher than the last one */
		if (found_entry) {
			/* only one entry is there */
			if (p_last_entry == p_us_rate_manager_list) {
				p_temp->p_next = p_us_rate_manager_list;
				p_us_rate_manager_list = p_temp;
			}
			else {
				p_temp->p_next = p_last_entry;
				if (p_prev_entry != NULL) {
					p_prev_entry->p_next = p_temp;
				}
			}
		}
		else {
			p_last_entry->p_next = p_temp;
			p_temp->p_next = NULL;
		}
	}

	/* fill ca_us_rate_manager_entry_t */
	p_temp->entry.app_spec_stream.app_stream_id = p_rate_mgr->app_stream_id;
	p_temp->entry.app_spec_stream.cir_kbps = p_rate_mgr->cir_kbps;
	p_temp->entry.app_spec_stream.pir_kbps = p_rate_mgr->pir_kbps;
	p_temp->entry.app_spec_stream.strict_prio = p_rate_mgr->strict_prio;
	p_temp->entry.dest_port = g_us_rate_manager_wan_port_id;

	ca_spin_unlock(g_us_rate_manager_lock);

#ifdef CONFIG_CORTINA_IPC2RCPU
	ca_us_rate_manager_ipc_add_entry(&(p_temp->entry));
#endif

	return CA_E_OK;
}

ca_status_t ca_us_rate_manager_entry_delete (
		CA_IN  ca_device_id_t  device_id,
		CA_IN  ca_uint8_t app_stream_id)
{
	ca_us_rate_manager_cb_t *p_temp, *p_last;

	if (!g_us_rate_manager_enable) {
		return CA_E_INACTIVE;
	}

	ca_printf("%s: app_stream_id=%d\n", __func__, app_stream_id);

	/* can not delete default us rate manager */
	if (app_stream_id >= CA_US_RATE_MANAGER_MAX) {
		ca_printf("%s: can not delete default us rate manager!!\n", __func__);
		return CA_E_PARAM;
	}

	ca_spin_lock(g_us_rate_manager_lock);

	/* find the sw shaper entry to be deleted */
	p_temp = p_us_rate_manager_list;
	while (p_temp) {
		if (p_temp->entry.app_spec_stream.app_stream_id == app_stream_id) {
			ca_printf("%s: Got sw us rate manager entry, id=%d\n", __func__, app_stream_id);
			break;
		}
		p_last = p_temp;
		p_temp = p_temp->p_next;
	}
	if (!p_temp) {
		ca_spin_unlock(g_us_rate_manager_lock);
		return CA_E_NOT_FOUND;
	}

	/* free voq */
	g_us_rate_manager_voq_list[p_temp->entry.app_spec_stream.app_stream_id] = 0;

#ifdef CONFIG_CORTINA_IPC2RCPU
	/* call to PE before free */
	ca_us_rate_manager_ipc_del_entry(p_temp->entry.app_spec_stream.app_stream_id);
	//ca_us_rate_manager_ipc_del_entry(p_temp);
#endif

	if (p_temp == p_us_rate_manager_list) {
		p_us_rate_manager_list = p_temp->p_next;
	}
	else {
		p_last->p_next = p_temp->p_next;
	}
	ca_free(p_temp);

	ca_spin_unlock(g_us_rate_manager_lock);

	return CA_E_OK;
}

ca_status_t ca_us_rate_manager_delete_all (
		CA_IN  ca_device_id_t device_id)
{
	ca_us_rate_manager_cb_t *p_temp, *p_next;

	if (!g_us_rate_manager_enable) {
		return CA_E_INACTIVE;
	}

	if (p_us_rate_manager_list == NULL) {
		return CA_E_OK;
	}

#ifdef CONFIG_CORTINA_IPC2RCPU
	ca_us_rate_manager_ipc_del_all_entry();
#endif

	ca_spin_lock(g_us_rate_manager_lock);

	/* delete all of entry except the default rate entry */
	p_temp = p_us_rate_manager_list->p_next;
	while (p_temp) {
		p_next = p_temp->p_next;
		ca_free(p_temp);
		p_temp = p_next;
	}

	p_us_rate_manager_list = NULL;

	ca_spin_unlock(g_us_rate_manager_lock);

	return CA_E_OK;
}

ca_status_t ca_us_rate_manager_entry_get (
		CA_IN  ca_device_id_t  device_id,
		CA_OUT us_rate_manager_app_spec_stream_t       *p_app_stream)
{
	ca_status_t ret;
	ca_us_rate_manager_cb_t *p_temp;

	if (!g_us_rate_manager_enable) {
		return CA_E_INACTIVE;
	}

	if (p_app_stream == NULL) {
		ca_printf("%s: p_app_stream is NULL!!!\n", __func__);
		return CA_E_PARAM;
	}

	/* can get the default shaper id */
	if (p_app_stream->app_stream_id >= CA_US_RATE_MANAGER_MAX) {
		ca_printf("%s: up stream rate manager app stream id exceed %d!!!\n", __func__, CA_US_RATE_MANAGER_MAX - 1);
		return CA_E_PARAM;
	}

	ca_spin_lock(g_us_rate_manager_lock);

	p_temp = p_us_rate_manager_list;
	while (p_temp) {
		if (p_temp->entry.app_spec_stream.app_stream_id == p_app_stream->app_stream_id) {
			break;
		}
		p_temp = p_temp->p_next;
	}

	if (p_temp != NULL) {
		memcpy(p_app_stream, &(p_temp->entry.app_spec_stream), sizeof(us_rate_manager_app_spec_stream_t));
		ret = CA_E_OK;
	}
	else {
		ca_printf("%s: app_stream_id=%d not found!!!\n", __func__, p_app_stream->app_stream_id);
		ret = CA_E_NOT_FOUND;
	}

	ca_spin_unlock(g_us_rate_manager_lock);

	return ret;
}

ca_status_t ca_us_rate_manager_entry_iterate (
		CA_IN  ca_device_id_t        device_id,
		CA_OUT ca_iterator_t *return_entry)
{
	ca_uint32_t i, e_sz, cnt;
	us_rate_manager_app_spec_stream_t *p_entry = return_entry->entry_data;
	ca_int32_t b_sz = return_entry->user_buffer_size;
	ca_us_rate_manager_cb_t *p_temp;

	if (!g_us_rate_manager_enable) {
		return CA_E_INACTIVE;
	}

	if (return_entry == NULL || return_entry->entry_data == NULL)
		return CA_E_PARAM;

	e_sz = sizeof(us_rate_manager_app_spec_stream_t);
	//ca_printf("%s: return_entry->prev_handle=%p\n", __func__, return_entry->prev_handle);
	if (return_entry->prev_handle == NULL) {
		p_temp = p_us_rate_manager_list;
	} else {
		p_temp = return_entry->prev_handle;

		/* check if prev_handle hit the last entry already */
		if (p_temp->p_next == NULL && return_entry->entry_count > 0){
			return_entry->entry_count = 0;
			ca_printf("%s: No data is available!!!\n", __func__);
			return CA_E_UNAVAIL;
		} else {
			p_temp = p_temp->p_next;
		}
	}
  
	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	ca_spin_lock(g_us_rate_manager_lock);

	/* the shaper id equal to g_us_rate_manager_max_supported - 1 is reserved for default shaper */

	for (i = 0, cnt = 0; (p_temp != NULL) && (i < g_us_rate_manager_max_supported) && (b_sz >= e_sz) && (cnt < return_entry->entry_count); i++) {
		/* copy data */
		memcpy(p_entry, &(p_temp->entry.app_spec_stream), sizeof(us_rate_manager_app_spec_stream_t));
		b_sz -= e_sz;
		cnt++;
		p_entry++;

		if (p_temp->p_next == NULL || cnt == return_entry->entry_count)
			break;
		p_temp = p_temp->p_next;
	}

	return_entry->entry_count = cnt;
	return_entry->prev_handle = p_temp;

	ca_spin_unlock(g_us_rate_manager_lock);

	if (cnt == 0) {
		ca_printf("%s: No data is available!!!\n", __func__);
		return CA_E_UNAVAIL;
	}

	return CA_E_OK;
}

void ca_us_rate_manager_table_dump(void)
{
	ca_iterator_t   iterator;
	ca_uint32_t     count;
	ca_status_t     ret;
	int index;
	us_rate_manager_app_spec_stream_t us_rate_manager_entry;

	/* get first nexthop entry */
	memset(&iterator, 0, sizeof(ca_iterator_t));
	iterator.prev_handle = NULL;
	count = 1;
	iterator.entry_count = count;
	iterator.entry_data = &us_rate_manager_entry;
	iterator.user_buffer_size = sizeof(us_rate_manager_app_spec_stream_t);

	index = 0;
	while (1) {
		ret = ca_us_rate_manager_entry_iterate(0, &iterator);
		if (ret != CA_E_OK || iterator.entry_count == 0)
			break;

		ca_printf("================= index=%d  =====================\n", index);

		ca_printf("app_stream_id=%d, cir_kbps=%d, pir_kbps=%d, strict_prio=%d\n",
				us_rate_manager_entry.app_stream_id, us_rate_manager_entry.cir_kbps, us_rate_manager_entry.pir_kbps, us_rate_manager_entry.strict_prio);

		/* end of list */
		if (iterator.prev_handle == NULL) {
			break;
		}
		index++;
	}
	ca_printf("\n\n");

#ifdef CONFIG_CORTINA_IPC2RCPU
	ca_us_rate_manager_ipc_dump();
#endif
}

EXPORT_SYMBOL(ca_us_rate_manager_global_configuration_set);
EXPORT_SYMBOL(ca_us_rate_manager_global_configuration_get);
EXPORT_SYMBOL(ca_us_rate_manager_entry_add);
EXPORT_SYMBOL(ca_us_rate_manager_entry_delete);
EXPORT_SYMBOL(ca_us_rate_manager_delete_all);
EXPORT_SYMBOL(ca_us_rate_manager_entry_get);
EXPORT_SYMBOL(ca_us_rate_manager_entry_iterate);
