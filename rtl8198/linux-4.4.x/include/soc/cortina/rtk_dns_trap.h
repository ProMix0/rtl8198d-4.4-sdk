#ifndef _RTK_DNS_TRAP_H
#define _RTK_DNS_TRAP_H

//#define DBG_DNS_TRAP
//#define SUPPORT_TRAP_ALL

#if defined(DBG_DNS_TRAP)
	#define DBGP_DNS_TRAP(format, arg...) 	  \
		do {printk(format , ## arg);}while(0)
#else
	#define DBGP_DNS_TRAP(format, arg...)
#endif

typedef struct _header {
	unsigned short int	id;
	unsigned short		u;

	short int	qdcount;
	short int	ancount;
	short int	nscount;
	short int	arcount;
} dnsheader_t;

int rtk_dns_trap_enter(struct sk_buff **skb);
int is_dns_packet(struct sk_buff *skb);
extern int dns_trap_enable;

int rtk_dns_trap_init(void);
void rtk_dns_trap_exit(void);


#endif	/* _RTK_DNS_TRAP_H */
