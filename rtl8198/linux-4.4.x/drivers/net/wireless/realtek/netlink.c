#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>

#define NETLINK_WIFI	30

enum {
	WIFI_EVENT_RX_DISTRIBUTE,
	WIFI_EVENT_RSVD
};

#define WIFI_NETLINK_OPTYPE_NONE            0x00
#define WIFI_NETLINK_OPTYPE_REGISTER        0x01
#define WIFI_NETLINK_OPTYPE_SEND_MSG        0x02 /* Send message between kernel/user */

typedef struct pr_wifiNetlink_s {
    unsigned short prOpType;   /* operation type */
    int            ownPid;     /* Process ID */
} pr_wifiNetlink_t;

typedef struct {
	unsigned int msgType;
    unsigned int disableRxDistribute;
	unsigned int reserved;
	unsigned int reserved1;
	unsigned int reserved2;
} wifiEventMsg_t;

static struct sock *wifi_netlink_sock;
int wifi_netlink_pid;

int wifi_event_send_rxDistribute(int disableRxDistribute)
{
	struct sk_buff *skb;
    wifiEventMsg_t msgData;
    struct nlmsghdr *nlh;
    int len;
    void *data;
    int ret;

    len = NLMSG_SPACE(sizeof(msgData));
    skb = alloc_skb(len, GFP_ATOMIC);
    if (!skb)
    {
        return -3;
    }
#if defined(CONFIG_KERNEL_2_6_30)
    nlh = NLMSG_PUT(skb, wifi_netlink_pid, 0, 0, sizeof(msgData));
#else
	nlh = nlmsg_put(skb, wifi_netlink_pid, 0, 0, sizeof(msgData), 0);
#endif
    nlh->nlmsg_flags = 0;
    data = NLMSG_DATA(nlh);

	memset(&msgData, 0, sizeof(msgData));
	msgData.msgType = WIFI_EVENT_RX_DISTRIBUTE;
	msgData.disableRxDistribute = disableRxDistribute;

    memcpy(data, &msgData, sizeof(msgData));
#if defined(CONFIG_KERNEL_2_6_30)
    NETLINK_CB(skb).pid = 0; /* from kernel */
#else
	NETLINK_CB(skb).portid = 0; /* from kernel */
#endif
    NETLINK_CB(skb).dst_group = 0; /* unicast */

    ret = netlink_unicast(wifi_netlink_sock, skb, wifi_netlink_pid, MSG_DONTWAIT);
    if (ret < 0)
    {
        printk("send failed %d\n", ret);
        return -4;
    }
    return 0;

    nlmsg_failure: /* Used by NLMSG_PUT */
    if (skb)
    {
        kfree_skb(skb);
    }
    return -5;
}

static void wifi_netlink_recv(struct sk_buff *skb)
{
    u_int pid;
    void *data;
    struct nlmsghdr *nlh;
	pr_wifiNetlink_t *prId;

    nlh = (struct nlmsghdr *)skb->data;
    pid = NETLINK_CREDS(skb)->pid;
    data = NLMSG_DATA(nlh);
    printk("recv skb from user space pid:%d\n", pid);

	prId = (pr_wifiNetlink_t *) data;
	switch(prId->prOpType)
	{
		case WIFI_NETLINK_OPTYPE_REGISTER:
			wifi_netlink_pid = prId->ownPid;
			printk("wifi_netlink_pid=%d\n", wifi_netlink_pid);
			break;
		default:
			break;
	}
}

int __init wifi_netlink_init(void)
{
    /* Create netlink for communicate between kernel and user protocol */
#if defined(CONFIG_KERNEL_2_6_30)
    wifi_netlink_sock = netlink_kernel_create(&init_net, NETLINK_WIFI, 0, wifi_netlink_recv, NULL, THIS_MODULE);
#else
	struct netlink_kernel_cfg cfg = {
	    .input = wifi_netlink_recv,
	};

	wifi_netlink_sock = netlink_kernel_create(&init_net, NETLINK_WIFI, &cfg);
#endif
}

void __exit wifi_netlink_exit(void)
{
    sock_release(wifi_netlink_sock->sk_socket);
}

module_init(wifi_netlink_init);
module_exit(wifi_netlink_exit);


