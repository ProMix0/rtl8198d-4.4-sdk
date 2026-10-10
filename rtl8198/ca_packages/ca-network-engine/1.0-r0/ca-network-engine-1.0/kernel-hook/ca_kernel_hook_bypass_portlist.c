#include <linux/spinlock.h>
#include <soc/cortina/ca_kernel_hook.h>

/* TCP BYPASS PORT */
#define FTP_CTRL_PORT		(21)
#define H323_PORT		(1720)
#define T120_PORT		(1503)
#define PPTP_PORT		(1723)
#define UPNP_PORT		(5000)
#define RTSP_PORT		(554)
#define MSN_PORT		(1863)

/* UDP BYPASS PORT */
#define TFTP_PORT		(69)
#define DNS_PORT		(53)
#define NTP_PORT		(123)
#ifndef RAS_PORT
#define RAS_PORT		(1719)
#endif
#define BOOTP67_PORT		(67)
#define BOOTP68_PORT		(68)
#define ISAKMP_PORT		(500)
#define UPNP_DISC_PORT		(1900)
#define L2TP_CTRL_PORT		(1701)
#ifndef SIP_PORT
#define SIP_PORT		(5060)
#endif
#define MAX_PORT_LIST_SIZE      (256)

DEFINE_SPINLOCK(ca_kh_bypass_portlist_lock);
enum {
	BYPASS_LIST_TYPE_TCP,
	BYPASS_LIST_TYPE_UDP,
	BYPASS_LIST_TYPE_MAX,
};

static ca_uint16_t bypass_tcp_port_list[MAX_PORT_LIST_SIZE + 1] = {
	FTP_CTRL_PORT,
	H323_PORT,
	T120_PORT,
	PPTP_PORT,
	UPNP_PORT,
	RTSP_PORT,
	MSN_PORT,
	0
};

static ca_uint16_t bypass_udp_port_list[MAX_PORT_LIST_SIZE + 1] = {
	DNS_PORT,
	NTP_PORT,
	TFTP_PORT,
	RAS_PORT,
	BOOTP67_PORT,
	BOOTP68_PORT,
	ISAKMP_PORT,
	UPNP_DISC_PORT,
#ifndef CONFIG_CS75XX_HW_ACCEL_IPSEC_CTRL
	L2TP_CTRL_PORT,
#endif
	SIP_PORT,
	0
};

ca_uint16_t* ca_forward_port_list_get(ca_int16_t list_type) {
	switch (list_type) {
		case BYPASS_LIST_TYPE_TCP:
			return bypass_tcp_port_list;
		case BYPASS_LIST_TYPE_UDP:
			return bypass_udp_port_list;
		default:
			return NULL;
		}
}

int ca_forward_port_list_add(ca_int16_t list_type, ca_uint16_t port)
{
	int i;
	ca_uint16_t *port_ptr = ca_forward_port_list_get(list_type);
	int ret = -1;
	spin_lock_bh(&ca_kh_bypass_portlist_lock);
	for(i = 0; i < MAX_PORT_LIST_SIZE; port_ptr++, i++) {
		if (*port_ptr == port) {
			break;
		} else if (*port_ptr == 0) {
			*port_ptr = port;
			*(port_ptr+1) = 0;
			printk("%s add port:%d at index(0~%d):%d\n",
				__func__, port, MAX_PORT_LIST_SIZE - 1, i);
			ret = 0;
			break;
		}
	}
	spin_unlock_bh(&ca_kh_bypass_portlist_lock);
	return ret;
}

int ca_forward_port_list_del(ca_int16_t list_type, ca_uint16_t port)
{
	int i;
	ca_uint16_t *port_ptr = ca_forward_port_list_get(list_type);
	ca_uint16_t *del_port_ptr = NULL;
	int ret = -1;

	spin_lock_bh(&ca_kh_bypass_portlist_lock);
	for(i = 0; i < MAX_PORT_LIST_SIZE; port_ptr++, i++) {
		if(*port_ptr == 0) {
			if(del_port_ptr == NULL) {
				//Debug Message
				printk("%s cannot del port:%d the current size(1~%d):%d\n",
					__func__, port, MAX_PORT_LIST_SIZE, i);
			} else {
				*del_port_ptr = *(port_ptr - 1);
				*(port_ptr - 1) = 0;
				ret = 0;
			}
			break;
		} else if(*port_ptr == port) {
			del_port_ptr = port_ptr;
		}
	}
	spin_unlock_bh(&ca_kh_bypass_portlist_lock);

	return ret;
}

void ca_forward_port_list_dump(ca_int16_t list_type)
{
	int i;
	ca_uint16_t *port_ptr = ca_forward_port_list_get(list_type);

	if(port_ptr == NULL) return;

	switch(list_type) {
		case BYPASS_LIST_TYPE_TCP:
			printk("Dump bypass_tcp_port_list: \n");
			break;
		case BYPASS_LIST_TYPE_UDP:
			printk("Dump bypass_udp_port_list: \n");
			break;
		default:
			break;
	}

	spin_lock_bh(&ca_kh_bypass_portlist_lock);
	for(i=0; i < MAX_PORT_LIST_SIZE; port_ptr++, i++) {
		if(*port_ptr == 0)
			break;
		printk("\t%d" , *port_ptr);
		if(i % 10 == 9)
			printk("\n");
	}
	printk("\n\ttotal list size:%d \n", i);
	spin_unlock_bh(&ca_kh_bypass_portlist_lock);
}

int ca_bypass_portlist_exist(ca_int16_t list_type, ca_uint16_t port)
{
	ca_uint16_t *port_ptr = ca_forward_port_list_get(list_type);
	int i, rtn = CA_FALSE;

	spin_lock_bh(&ca_kh_bypass_portlist_lock);
	for(i=0; i < MAX_PORT_LIST_SIZE; port_ptr++, i++) {
		if(*port_ptr == 0) {
			rtn = CA_FALSE;
			break;
		}
		if(*port_ptr == port) {
			if (ca_kh_debug & CA_KH_DEBUG_BYPASS_PORTLIST) {
				printk("%s:port=%d exists in the list\n", __func__, *port_ptr);
			}
			rtn = CA_TRUE;
			break;
		}
	}
	spin_unlock_bh(&ca_kh_bypass_portlist_lock);
	return rtn;
}

ca_boolean_t ca_l4_port_bypass_is_needed(ca_uint8_t ip_proto, ca_uint16_t src_port, ca_uint16_t dst_port)
{
	ca_int16_t i, list_type;
	ca_boolean_t rtn;
	ca_uint16_t *port_ptr;

	if(ip_proto == IPPROTO_TCP)
		list_type = BYPASS_LIST_TYPE_TCP;
	else if(ip_proto == IPPROTO_UDP)
		list_type = BYPASS_LIST_TYPE_UDP;
	else {
		rtn = CA_FALSE;
		goto error;
	}

	if (ca_kh_debug & CA_KH_DEBUG_BYPASS_PORTLIST) {
		ca_forward_port_list_dump(list_type);
		printk("src_port=%d dst_port=%d\n", src_port, dst_port);
	}

	port_ptr = ca_forward_port_list_get(list_type);
	spin_lock_bh(&ca_kh_bypass_portlist_lock);
	for(i=0; i < MAX_PORT_LIST_SIZE; port_ptr++, i++) {
		//printk("%s:port=%d\n", __func__, *port_ptr);
		if(*port_ptr == 0) {
			rtn = CA_FALSE;
			break;
		}
		if(*port_ptr == src_port || *port_ptr == dst_port) {
			rtn = CA_TRUE;
			break;
		}
	}
	spin_unlock_bh(&ca_kh_bypass_portlist_lock);

error:
	return rtn;
}

ca_status_t ca_kh_bypass_tcp_portlist_add(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag)
{
	return ca_forward_port_list_add(BYPASS_LIST_TYPE_TCP, port);
}

ca_status_t ca_kh_bypass_tcp_portlist_delete(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag)
{
	return ca_forward_port_list_del(BYPASS_LIST_TYPE_TCP, port);
}

void ca_kh_bypass_tcp_portlist_dump(void)
{
	ca_forward_port_list_dump(BYPASS_LIST_TYPE_TCP);
}

int ca_kh_bypass_tcp_portlist_exist(ca_uint16_t port)
{
	return ca_bypass_portlist_exist(BYPASS_LIST_TYPE_TCP, port);
}

ca_status_t ca_kh_bypass_tcp_portlist_init(void)
{
	printk("%s: Init OK\n", __func__);
	return CA_E_OK;
}

/* UDP Portlist API */
ca_status_t ca_kh_bypass_udp_portlist_add(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag)
{
	return ca_forward_port_list_add(BYPASS_LIST_TYPE_UDP, port);
}

ca_status_t ca_kh_bypass_udp_portlist_delete(ca_device_id_t device_id, ca_uint16_t port, ca_uint8_t flag)
{
	return ca_forward_port_list_del(BYPASS_LIST_TYPE_UDP, port);
}

void ca_kh_bypass_udp_portlist_dump(void)
{
	ca_forward_port_list_dump(BYPASS_LIST_TYPE_UDP);
}

int ca_kh_bypass_udp_portlist_exist(ca_uint16_t port)
{
	return ca_bypass_portlist_exist(BYPASS_LIST_TYPE_UDP, port);
}

ca_status_t ca_kh_bypass_udp_portlist_init(void)
{
	printk("%s: Init OK\n", __func__);
	return CA_E_OK;
}
