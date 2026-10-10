//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <netdb.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <netlink/netlink.h>
//#include <linux/msg.h>
#include <netlink/handlers.h>
#include <netlink/socket.h>
#include <netlink/errno.h>

#include "ca_types.h"
#include "scfg.h"
#include "rx.h"
#include "port.h"
#include "os_core.h"
#include "special_packet.h"
#include "osal_queue.h"

#include "ca_sdk_msg.h"
#include "ca_sdk_linux.h"

/* use for standalone test */
//#define CA_RX_TEST_MODE	1
//#define CA_RX_TEST_DEBUG	1

#define NETLINK_CA_RX	25

#define MY_MSG_TYPE (0x10 + 2)  // + 2 is arbitrary but is the same for kern/usr

/* used for multicast queue */
#define CA_RX_MAIN_MQ_NAME          	"ca_rx_mq"
#define CA_RX_IPC_MAX_ALLOC_SIZE	2048
#define CA_RX_IPC_MAX_MESG_NUM 	 	128

#define MAX_RX_PACKET_QUEUE_SIZE	256*1024	/* 256KB */

static char msg[32]="Cortina Access ca_rx_register";
static char exit_msg[32]="GoodBye";
static char ack_msg[32]="ACK";

static int ca_rx_has_initialized = 0;
static struct nl_sock *nls;
static ca_uint qid; 		/*receive queue id */
static ca_uint8_t ca_ni_rx_register_use_mq = 0;
static ca_uint8_t ca_ne_netlink_rx_ack = 0;

static char tmpbuf[CA_RX_IPC_MAX_ALLOC_SIZE];

typedef struct cpu_rx_cb_list_t{
	ca_pkt_type_t        type;
        ca_rx_cb        	fn;
        ca_uint8_t      	priority;
        void            	*cookie;
        ca_uint32_t     	flags;
        struct cpu_rx_cb_list_t *p_next;
}cpu_rx_cb_list_t;

/* there are two priority list one is high another is low */
cpu_rx_cb_list_t *cpu_rx_cb_high_head = NULL;
cpu_rx_cb_list_t *cpu_rx_cb_low_head = NULL;

static struct sigaction sigint_act;
static struct sigaction sigterm_act;

static ca_status_t ca_netlink_init(void);

static ca_status_t set_special_packet_to_cpu(ca_pkt_type_t pkt_type)
{
	ca_port_id_t source_port;
	ca_port_id_t cpu_port;
	ca_status_t ret;
    ca_special_packet_option_t   options = {0};

	source_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, 2);
	cpu_port = CA_PORT_ID(CA_PORT_TYPE_CPU, CA_PORT_ID_CPU1);
	ret = ca_special_packet_set(0, source_port, pkt_type, 1, cpu_port, 1, &options);

#ifdef CA_RX_TEST_DEBUG
	printf("%s: ca_special_packet_set() ret=%d\n", __func__, ret);
#endif
	return ret;
}

static void ca_rx_call_callback(ca_pkt_t * ca_pkt)
{
	int i;
	cpu_rx_cb_list_t  *p_node = NULL;

	/* search high priority RX register call back routine */
	i = 0;
	for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {

		/* find the registered pkt_type */
		if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == ca_pkt->pkt_type) {

#ifdef CA_RX_TEST_DEBUG
			printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
					i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
#endif

			p_node->fn(0, ca_pkt, p_node->cookie);
		}
		i++;
	}

	/* search low priority RX register call back routine */
	i = 0;
	for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {

		/* find the registered pkt_type */
		if (p_node->type == CA_PKT_TYPE_CONTROL_ALL || p_node->type == ca_pkt->pkt_type) {

#ifdef CA_RX_TEST_DEBUG
			printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
					i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
#endif

			p_node->fn(0, ca_pkt, p_node->cookie);
		}
		i++;
	}
}

static void ca_rx_packet_entry(void)
{
	struct nl_cb  *cb = NULL;
	struct sockaddr_nl nla;
	unsigned char *buf = NULL;
	unsigned char *data;
	int ret, err, size, i;
	struct nlmsghdr *nlh;
	ca_pkt_t *ca_pkt;
	ca_pkt_t *new_ca_pkt, *orig_new_ca_pkt;
	char *ptr;

	while (1) {
		size = nl_recv(nls, &nla, &buf, NULL);
		/* if nl_recv() failed do not queue the packet */
		if (size <= 0) {
			continue;
		}

#ifdef CA_RX_TEST_DEBUG
		printf("%s: qid=%d, nl_recv() OK, size=%d\n", __func__, qid, size);
#endif

		nlh = (struct nlmsghdr *)buf;
		ca_pkt = (ca_pkt_t *)NLMSG_DATA(nlh);

		data = (unsigned char *)ca_pkt;

		data += sizeof(ca_pkt_t);
		ca_pkt->pkt_data = (ca_pkt_block_t *)data;

		data += sizeof(ca_pkt_block_t);
		ca_pkt->pkt_data->data = data;

#ifdef CA_RX_TEST_DEBUG
		printf("%s: buf=%p, ca_pkt->pkt_data->data=%p\n", __func__, buf, ca_pkt->pkt_data->data);
#endif

		if (ca_ni_rx_register_use_mq) {
			size = sizeof(ca_pkt_t) + sizeof(ca_pkt_block_t) + ca_pkt->pkt_data->len;
			ret = ca_queue_put(qid, ca_pkt, size, 1000, 0);
			if (ret == -1) {
				printf("%s: ca_queue_put() failed!!\n", __func__);
			}
		}
		else {
			ca_rx_call_callback(ca_pkt);
		}

		/* The caller of nl_recv() is responsible for freeing the buffer. */
		free(buf);

		/* send "ACK" to linux kernel */
		if (ca_ne_netlink_rx_ack) {
			ret = nl_send_simple(nls, MY_MSG_TYPE, 0, ack_msg, sizeof(ack_msg));
			if (ret < 0) {
				nl_perror(ret, "nl_send_simple(ACK)");
			} else {
				//printf("%s: send ACK, sent %d bytes\n", __func__, ret);
			}
		}

	}
}

static void ca_queue_get_pkt_entry(void)
{
	int ret, i, size;
	ca_uint32 size_copied;
	ca_pkt_t *ca_pkt;
	cpu_rx_cb_list_t  *p_node = NULL;
	char *ptr;
	char *msg_ptr;

	while (1) {
		ret = ca_queue_get(qid, &tmpbuf[0], CA_RX_IPC_MAX_ALLOC_SIZE, &size_copied, 1000);
		if (ret == CA_E_OK) {

			ptr = &tmpbuf[0];
			ca_pkt = (ca_pkt_t *)ptr;
			ca_pkt->pkt_data = ptr + sizeof(ca_pkt_t);
			ca_pkt->pkt_data->data = ptr + sizeof(ca_pkt_t)+sizeof(ca_pkt_block_t);

#ifdef CA_RX_TEST_DEBUG
			printf("%s: package receivde, size_copied=%d\n", __func__, size_copied);
			printf("packet data received[]=");
			for (i = 0; i < ca_pkt->pkt_data->len; i++) {
			if (!(i % 16)) printf("\n");
				printf("%2x ", ca_pkt->pkt_data->data[i]);
			}
			printf("\n");
#endif

			ca_rx_call_callback(ca_pkt);
		}
	}
}

static ca_int32 ca_packet_init(void)
{
	ca_uint64 tid;
	ca_int ret = CA_E_ERROR;
	ca_uint8 mq_name[128+1];
	ca_device_config_tlv_t scfg_tlv;

	memset(&scfg_tlv, 0, sizeof(ca_device_config_tlv_t));
	scfg_tlv.type = CA_CFG_ID_NE_CA_RX_REGISTER_USE_MQ;
	scfg_tlv.len = sizeof(ca_uint8_t);
	if (ca_device_config_tlv_get(0, &scfg_tlv) == CA_E_OK) {
		printf("Got CFG_ID_NE_CA_RX_REGISTER_USE_MQ vlaue=%d\n", scfg_tlv.value[0]);
		ca_ni_rx_register_use_mq = scfg_tlv.value[0];
	}
	else
		printf("%s: Got CFG_ID_NE_CA_RX_REGISTER_USE_MQ failed!!\n", __func__);

	if (ca_ni_rx_register_use_mq) {
		sprintf(mq_name, "%s%d", CA_RX_MAIN_MQ_NAME, getpid());
		ret = ca_queue_create(&qid, mq_name, CA_RX_IPC_MAX_MESG_NUM, CA_RX_IPC_MAX_ALLOC_SIZE, 0);
		if (ret) {
			printf("MC: creating queue %s failed, ret=%d\n", CA_RX_MAIN_MQ_NAME, ret);
			return -1;
		}
		ret = ca_thread_create(&tid, "QUEUE_PKT_GET_THREAD", ca_queue_get_pkt_entry, NULL, MAX_RX_PACKET_QUEUE_SIZE, 15, 0);
		if (ret)
		{
			printf("Create rx packet thread failed!!!\n");
			return CA_E_ERROR;
		}
	}

	memset(&scfg_tlv, 0, sizeof(ca_device_config_tlv_t));
	scfg_tlv.type = CA_CFG_ID_NE_NETLINK_RX_ACK;
	scfg_tlv.len = sizeof(ca_uint8_t);
	if (ca_device_config_tlv_get(0, &scfg_tlv) == CA_E_OK) {
		printf("Got CFG_ID_NE_NETLINK_RX_ACK vlaue=%d\n", scfg_tlv.value[0]);
		ca_ne_netlink_rx_ack = scfg_tlv.value[0];
	}
	else
		printf("%s: Get CA_CFG_ID_NE_NETLINK_RX_ACK failed!!\n", __func__);


	ret = ca_thread_create(&tid, "PKT_RX_THREAD", ca_rx_packet_entry, NULL, MAX_RX_PACKET_QUEUE_SIZE, 15, 0);
	if (ret)
	{
		printf("Create rx packet thread failed!!!\n");
		return CA_E_ERROR;
	}

	return CA_E_OK;
}

//#ifdef CA_RX_TEST_MODE
static void ca_process_controlC_handle(int signum)
{
	int ret;
	ca_uint8 mq_name[128+1];

	printf("%s: signum=%d\n", __func__, signum);

	/* destroy the msg queue which created by ca_rx_register() */
	ca_queue_delete(qid);

	/* should call mq_unlink() to release the resource occuppied */
	/* otherwise when call mq_open() will get "too many open files" error */
	sprintf(mq_name, "/%s%d", CA_RX_MAIN_MQ_NAME, getpid());
	mq_unlink(mq_name);

	ret = nl_send_simple(nls, MY_MSG_TYPE, 0, exit_msg, sizeof(exit_msg));
	if (ret < 0) {
		nl_perror(ret, "nl_send_simple");
		nl_socket_free(nls);
		nl_close(nls);
	} else {
		printf("%s: signum=%d, sent %d bytes\n", __func__, signum, ret);
	}

	if (signum == SIGINT) {
		printf("%s: SIGINT, sigint_act.sa_handler=%p\n", __func__, sigint_act.sa_handler);
		if (sigint_act.sa_handler != NULL)
			sigint_act.sa_handler(signum);
	}

	if (signum == SIGTERM) {
		printf("%s: SIGTERM, sigterm_act.sa_handler=%p\n", __func__, sigterm_act.sa_handler);
		if (sigterm_act.sa_handler != NULL)
			sigterm_act.sa_handler(signum);
	}

	exit(-1);
}
//#endif

/* Cortina API here */

static void ca_rx_register_dump_all()
{
	int i;
	cpu_rx_cb_list_t  *p_node = NULL;

	printf("%s: === dump RX high priority register list ===\n", __func__);
	i = 0;
	for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next) {
		printf("%s: p_node=%p, cpu_rx_cb_high_head=%p\n", __func__, p_node, cpu_rx_cb_high_head);
		printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
				i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
		i++;
	}

        printf("%s: === dump RX low priority register list ===\n", __func__);
        i = 0;
        for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next) {
                printf("%s: p_node=%p, cpu_rx_cb_low_head=%p\n", __func__, p_node, cpu_rx_cb_low_head);
                printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
                                i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
                i++;
        }
}

static ca_uint32_t ca_rx_register_check_empty(void)
{
	ca_uint32_t count = 0;
	cpu_rx_cb_list_t  *p_node = NULL;
	ca_uint8 mq_name[128+1];

	for (p_node = cpu_rx_cb_high_head; p_node;  p_node = p_node->p_next)
		count++;

	for (p_node = cpu_rx_cb_low_head; p_node;  p_node = p_node->p_next)
		count++;

	if (count == 0) {
		printf("%s: No packet type resgistered, delete queue and free socket!!!\n", __func__);
		ca_queue_delete(qid);

		/* should call mq_unlink() to release the resource occuppied */
		/* otherwise when call mq_open() will get "too many open files" error */
		sprintf(mq_name, "/%s%d", CA_RX_MAIN_MQ_NAME, getpid());
		mq_unlink(mq_name);
		nl_send_simple(nls, MY_MSG_TYPE, 0, exit_msg, sizeof(exit_msg));
		nl_socket_free(nls);
		nl_close(nls);
	}

	return count;
}

ca_status_t ca_rx_register(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_pkt_type_t  type,
	CA_IN ca_rx_cb		fn,
	CA_IN ca_uint8_t	priority,
	CA_IN void		*cookie,
	CA_IN ca_uint32_t	flags)
{
	cpu_rx_cb_list_t  *p_head_node = NULL;
	cpu_rx_cb_list_t  *p_node = NULL;
	cpu_rx_cb_list_t  *p_new_node = NULL;
	cpu_rx_cb_list_t  *p_prev_node = NULL;
	ca_status_t        ret = CA_E_OK;
	struct sigaction newact;
	sighandler_t	signal_ret;

#ifndef CA_RX_TEST_MODE
	if (!ca_rx_has_initialized) {
		newact.sa_handler = ca_process_controlC_handle;
		sigemptyset(&newact.sa_mask);
		newact.sa_flags = 0;

		signal_ret = sigaction(SIGINT, NULL, &sigint_act);
		printf("%s: old SIGINT, sigint_act.sa_handler=%p, signal_ret=%d\n", __func__, sigint_act.sa_handler, signal_ret);
		signal_ret = signal(SIGINT, ca_process_controlC_handle);
		printf("%s: new SIGINT, sigint_act.sa_handler=%p, signal_ret=%d\n", __func__, sigint_act.sa_handler, signal_ret);

		signal_ret = sigaction(SIGTERM, NULL, &sigterm_act);
		printf("%s: old SIGTERM, sigint_act.sa_handler=%p, signal_ret=%d\n", __func__, sigterm_act.sa_handler, signal_ret);
		signal_ret = signal(SIGTERM, ca_process_controlC_handle);
		printf("%s: new SIGTERM, sigterm_act.sa_handler=%p, signal_ret=%d\n", __func__, sigterm_act.sa_handler, signal_ret);

		ca_netlink_init();
		ca_packet_init();
		ca_rx_has_initialized = 1;
	}
#endif

	/* check input parameters */
	if (fn == NULL) {
		printf("%s: call back function is NULL!!\n", __func__);
		return CA_E_PARAM;
	}

	if (priority) {
		if (cpu_rx_cb_high_head != NULL){
			for (p_node = cpu_rx_cb_high_head; p_node; p_node = p_node->p_next) {
				if (p_node->type == type && p_node->fn == fn && p_node->priority == priority)
#ifdef CA_RX_TEST_DEBUG
					printf("%s: type=%d already exists!!\n", __func__, type);
#endif
					return CA_E_EXISTS;
			}
		}
	}
	else {
		if (cpu_rx_cb_low_head != NULL){
			for (p_node = cpu_rx_cb_low_head; p_node; p_node = p_node->p_next) {
				if (p_node->type == type && p_node->fn == fn && p_node->priority == priority)
#ifdef CA_RX_TEST_DEBUG
                                        printf("%s: type=%d already exists!!\n", __func__, type);
#endif
					return CA_E_EXISTS;
			}
		}
	}

	/* pull the type of packet to CPU */
	/* the special packet should be set in user application */
	//set_special_packet_to_cpu(type);

	p_new_node = malloc(sizeof(cpu_rx_cb_list_t));
	if (p_new_node == NULL){
		printf("%s: Out of memory!!\n", __func__);
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

	p_new_node->type	= type;
	p_new_node->fn		= fn;
	p_new_node->priority	= priority;
	p_new_node->cookie	= cookie;
	p_new_node->flags	= flags;

#ifdef CA_RX_TEST_DEBUG
	printf("%s: p_new_node=%p, cpu_rx_cb_low_head=%p, cpu_rx_cb_high_head=%p\n",
		__func__, p_new_node, cpu_rx_cb_low_head, cpu_rx_cb_high_head);
	ca_rx_register_dump_all();
#endif

	/* call to Cortina API kernel mode */
	ca_rx_register_pkt(device_id, getpid(), priority, type);

	return ret;
}

ca_status_t ca_rx_unregister(
		CA_IN ca_device_id_t       device_id,
		CA_IN ca_pkt_type_t	type,
		CA_IN ca_rx_cb          fn,
		CA_IN ca_uint8_t        priority)
{
	cpu_rx_cb_list_t  *p_node = NULL;
	cpu_rx_cb_list_t  *p_new_node = NULL;
	cpu_rx_cb_list_t  *p_prev_node = NULL;
	ca_status_t        ret = CA_E_OK;

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
					free(p_node);

					///TODO: for test
					ca_rx_register_dump_all();

					/* call to Cortina API kernel part */
					ca_rx_unregister_pkt(device_id, getpid(), priority, type);

					/* check whether there is no packet type registered */
					ca_rx_register_check_empty();

					return CA_E_OK;
				}
				p_prev_node = p_node;
			}
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
					free(p_node);

					///TODO: for test
					ca_rx_register_dump_all();

					/* call to Cortina API kernel part */
					ca_rx_unregister_pkt(device_id, getpid(), priority, type);

					/* check whether there is no packet type registered */
					ca_rx_register_check_empty();

					return CA_E_OK;
				}
				p_prev_node = p_node;
			}
			return CA_E_NOT_FOUND;
		}
	}
	return CA_E_NOT_FOUND;
}

#ifdef CA_RX_TEST_MODE
static ca_int32_t default_ca_rx_cb(
        CA_IN   ca_device_id_t device_id,
        CA_OUT  ca_pkt_t    *pkt,
        CA_IN   void        *cookie)
{
	printf("%s: pkt=%p, cookie=%p\n", __func__, pkt, cookie);
	return CA_RX_HANDLED;
}

static void call_ca_rx_register(void)
{
	ca_uint32_t cookie = 88;

        /* set special packet */
        set_special_packet_to_cpu(CA_PKT_TYPE_DOT1X);

	ca_rx_register(0, (ca_rx_cb)default_ca_rx_cb, 0, &cookie, 0);

	ca_rx_register_dump_all();
}
#endif

//static char tmpbuf[CA_RX_IPC_MAX_ALLOC_SIZE];

static ca_status_t ca_netlink_init(void)
{
	int ret;
        struct nl_cb  *cb = NULL;
        struct nlmsghdr *nlh;

	cb = nl_cb_alloc(NL_CB_CUSTOM);
	if (cb == NULL) {
		printf("allocate nl_cb failed!!!\n");
		return CA_E_ERROR;
	}

	nls = nl_socket_alloc();
	if (!nls) {
		printf("nl_socket_alloc() failed!!!n");
		return CA_E_ERROR;
	}

	/* for libnl 3.2.25, set the local port id before call nl_connect() */
	nl_socket_set_local_port(nls, getpid());

	ret = nl_connect(nls, NETLINK_CA_RX);
	if (ret < 0) {
		nl_perror(ret, "nl_connect");
		nl_socket_free(nls);
		return CA_E_ERROR;
	}

	ret = nl_send_simple(nls, MY_MSG_TYPE, 0, msg, sizeof(msg));
	if (ret < 0) {
		nl_perror(ret, "nl_send_simple");
		nl_close(nls);
		nl_socket_free(nls);
		return CA_E_ERROR;
	} else {
		printf("Netlink sent %d bytes\n", ret);
		return CA_E_OK;
	}
}

#ifdef CA_RX_TEST_MODE 
int main(int argc, char *argv[])
{
	//struct nl_sock *nls;
	struct nl_cb  *cb = NULL;
	struct sockaddr_nl nla;
        unsigned char *buf = NULL;
	unsigned char *data;
	int ret, err, size, i, size_copied;
	struct nlmsghdr *nlh;
	ca_pkt_t *ca_pkt;
	cpu_rx_cb_list_t  *p_node = NULL;

	signal(SIGINT, ca_process_controlC_handle);
	signal(SIGTERM, ca_process_controlC_handle);
	signal(SIGKILL, ca_process_controlC_handle);

	/* set special packet */
	//set_special_packet_to_cpu(CA_PKT_TYPE_DOT1X);
	call_ca_rx_register();

#if 0
	cb = nl_cb_alloc(NL_CB_CUSTOM);
	if (cb == NULL)	{
		printf("allocate nl_cb failed!!!\n");
		return EXIT_FAILURE;
	}

	nls = nl_socket_alloc();
	if (!nls) {
		printf("nl_socket_alloc() failed!!!n");
		return EXIT_FAILURE;
	}

	ret = nl_connect(nls, NETLINK_CA_RX);
	if (ret < 0) {
		nl_perror(ret, "nl_connect");
		nl_socket_free(nls);
		return EXIT_FAILURE;
	}

	ret = nl_send_simple(nls, MY_MSG_TYPE, 0, msg, sizeof(msg));
	if (ret < 0) {
		nl_perror(ret, "nl_send_simple");
		nl_close(nls);
		nl_socket_free(nls);
		return EXIT_FAILURE;
	} else {
		printf("sent %d bytes\n", ret);
	}
#endif
	ca_netlink_init();

	ca_packet_init();

#if 0
	while (1) {
		ret = ca_queue_get(qid, &tmpbuf[0], CA_RX_IPC_MAX_ALLOC_SIZE, &size_copied, 1000);
		ca_pkt = (ca_pkt_t *)&tmpbuf[0];
		if (ret == CA_E_OK) {
			printf("%s: package receivde, size_copied=%d\n", __func__, size_copied);

			///TODO: for test
			printf("packet data received[]=");
			for (i = 0; i < ca_pkt->pkt_data->len; i++) {
				if (!(i % 16)) printf("\n");
				printf("%2x ", ca_pkt->pkt_data->data[i]);
			}
			printf("\n");

			/* search RX register call back routine */
        		i = 0;
        		for (p_node = cpu_rx_cb_head; p_node;  p_node = p_node->p_next) {
                		printf("[%d] call back func=%p, priority=%d, cookie=%p, flags=0x%x\n",
                        	i, p_node->fn, p_node->priority, p_node->cookie, p_node->flags);
				p_node->fn(0, ca_pkt, p_node->cookie);
                		i++;
       			}

	
		//	ret = mc_recv_frame(ca_pkt);
		}
	}
#endif

 	while (1) {
		sleep(10);
	}

	nl_close(nls);
	nl_socket_free(nls);

	return EXIT_SUCCESS;
}
#endif

typedef struct{
    ca_device_id_t device_id;
    ca_uint8  rsvd_f1[2];
    ca_uint32_t count;
    ca_pkt_t *pkt;
}__attribute__((packed)) ca_rx_polling_msg_wrap_t;

ca_status_t ca_rx(
    ca_device_id_t  device_id,
    ca_uint32_t * count,
    ca_pkt_t * pkt)
{
        int i;
        ca_int8 buf[CA_SDK_MSG_MAX_LEN];
        ca_sdk_msg_t *pMsg = (ca_sdk_msg_t *)buf;
        ca_status_t result = CA_E_ERROR;
        ca_rx_polling_msg_wrap_t * _wrap_cfg = (ca_rx_polling_msg_wrap_t *)(pMsg->data);

        if (NULL == count || NULL == pkt){
                return CA_E_PARAM;
        }

        ///TODO: debug
        printf("%s: count=%d, pkt=%p\n", __func__, *count, pkt);
        memset(buf,0x00,sizeof(buf));

        pMsg->len = sizeof(ca_rx_polling_msg_wrap_t);
        pMsg->sdk_id = CA_SDK_RX_POLL;
        _wrap_cfg->device_id = device_id;
        _wrap_cfg->count = *count;
        _wrap_cfg->pkt = pkt;

        printf("%s: _wrap_cfg->pkt=%p, &(_wrap_cfg->pkt)=%p\n", __func__, _wrap_cfg->pkt, &(_wrap_cfg->pkt));

        result =  ca_sdk_ioctl(device_id,pMsg);

        if(result != CA_E_OK){
                return result;
        }

        *count= _wrap_cfg->count;

#if 0
        for(i = 0; i < *count; i++, pkt++) {
                pkt->pkt_data->len = pkt->pkt_len;
        }
#endif
        return CA_E_OK;
}

