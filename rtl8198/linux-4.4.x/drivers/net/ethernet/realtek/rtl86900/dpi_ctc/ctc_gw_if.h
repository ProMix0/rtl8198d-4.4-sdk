#ifndef __MAXNET_GW_IF_H__
#define __MAXNET_GW_IF_H__

#if __KERNEL__
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 0)
#include <net/netfilter/nf_nat.h>
#else
#include <net/netfilter/nf_nat_rule.h>
#endif
#endif

#define MAXNET_FDS_ERR          (-1)
#define MAXNET_FDS_TRANS        (3)

/* 64 Bytes */
typedef struct dpi_appinfo_
{
    unsigned char       valid_mac;
    unsigned char       unused;
    unsigned char       user_mac[6];
    
    /* app id */
    unsigned int        related_app_id;
    unsigned int        cache_app_id;
    unsigned int        dev_id;
    unsigned int        fds_pkt_id;

    /* pad */
    unsigned char       pad[40];
} dpi_appinfo_t;

typedef struct dpi_gwinfo_
{
#if __KERNEL__
    unsigned int                layer7_id;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 0)
    struct nf_nat_range         nat_range;
#elif LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
    struct nf_nat_ipv4_range    nat_range;
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 21)
    struct nf_nat_range         nat_range;
#else
    struct ip_nat_range         nat_range;
#endif
#endif
} dpi_gwinfo_t;

typedef dpi_gwinfo_t         CtSgwGwInfo;

typedef int (*ctSgw_appGetInfoProc)(void *app_ctx, void *app_info);

int ctSgw_appRegisterGetInfo(ctSgw_appGetInfoProc funcs);
void ctSgw_appUnRegisterGetInfo(void);
int ctSgw_appGetInfo(void *app_ctx, void *app_info);

typedef int (*ctSgw_appPreProc)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo,
        void *p_app_ctx, CtSgwGwInfo *gwinfo, void *data);

typedef struct CtSgwPreProcInfo_
{
    ctSgw_appPreProc    hook;
    int                 priority;
    void                *priv_data;
} CtSgwPreProcInfo;

int ctSgw_appRegisterPreProc(CtSgwPreProcInfo *info);
void ctSgw_appUnRegisterPreProc(CtSgwPreProcInfo *info);

typedef int (*ctSgw_appForwardProc)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo,
        void *p_app_ctx, CtSgwGwInfo *gwinfoi, void *data);

typedef struct CtSgwForwardProcInfo_
{
    ctSgw_appForwardProc    hook;
    int                     priority;
    void                    *priv_data;
} CtSgwForwardProcInfo;

int ctSgw_appRegisterForwardProc(CtSgwForwardProcInfo *info);
void ctSgw_appUnRegisterForwardProc(CtSgwForwardProcInfo *info);
#if 1
typedef struct CtSgwDPIStatistic_
{
	//create/destroy context;
	unsigned long create_context;
	unsigned long create_ctx_err;
	unsigned long destroy_context;
	//pre-route
	unsigned long prerte_cnt;
	unsigned long prerte_noctx;
	unsigned long prerte_dnat;
	unsigned long prerte_indir;
	unsigned long prerte_outdir;
	unsigned long prerte_conti;
	unsigned long prerte_trap;
	unsigned long prerte_drop;
	unsigned long prerte_scan;
	unsigned long prerte_new;
	unsigned long prerte_tcp;
	unsigned long prerte_udp;
	unsigned long prerte_otherpro;
	unsigned long prerte_errret;
	//forward
	unsigned long forward_cnt;
	unsigned long forward_noctx;
	unsigned long forward_indir;
	unsigned long forward_outdir;
	unsigned long forward_conti;
	unsigned long forward_fin;
	unsigned long forward_drop;
	unsigned long forward_scan;
	unsigned long forward_new;
	unsigned long forward_tcp;
	unsigned long forward_udp;
	unsigned long forward_otherpro;
	unsigned long forward_errret;
	unsigned long forward_qos;
}CtSgwDPIStatistic;
#endif
#endif  // #ifndef __MAXNET_GW_IF_H__


