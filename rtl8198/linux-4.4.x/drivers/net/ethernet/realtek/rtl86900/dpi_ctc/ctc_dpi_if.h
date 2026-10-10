/**********************************************************************
 * maxnet_dpi_if.h
 *
 * Wang Dongquan <wdq347@163.com>
 * Description: 
 ***********************************************************************/
#ifndef _MAXNET_DPI_IF_H_
#define _MAXNET_DPI_IF_H_

#define MAXNET_GROUPID(id) ((((id) & 0xFF000000) >> 24) & 0x007F)
#define MAXNET_APPID(id)   ((id) & 0x0000FFFF)

#define MAXNET_DPI_ERR     (-1)
#define MAXNET_DPI_CON     (0)
#define MAXNET_DPI_FIN     (1)
#define MAXNET_DPI_DNY     (2)  /* drop this packet */
#define MAXNET_DPI_PQOS    (4)

enum {
	DPI_LOG_LEVEL_OFF = 0,
	DPI_LOG_LEVEL_ERROR,
	DPI_LOG_LEVEL_WARNING,
	DPI_LOG_LEVEL_INFO,
	DPI_LOG_LEVEL_DEBUG,
};
#define NETFILTER_MAX_HOOKS            8
#define MAX_NETDEV	32

typedef struct {
	unsigned int log_level;
	unsigned int skb_skip_fc;
	//others
} ctc_dpi_control_t;

extern ctc_dpi_control_t g_ctc_dpi_ctrl;
extern int g_fwdByPS;//bit position set into skb mark2.

/* Macro */
#define DPI_LOG(level, fmt, arg...) \
    do { if (g_ctc_dpi_ctrl.log_level >= level) { printk(fmt, ##arg); } } while (0);

typedef struct tupleinfo_ {
    unsigned char direct;
    unsigned char proto;
    unsigned char new_conntrack;
    unsigned int sipv4;
    unsigned int dipv4;
    unsigned int sipv6[4];
    unsigned int dipv6[4];
    unsigned short sport;
    unsigned short dport;
    unsigned int in_iif;
    unsigned int out_iif;
} tupleinfo_t;


typedef tupleinfo_t       CtSgwTupleInfo;
typedef  int  (*ctSgw_appCtxCreate)(void** p_app_ctx, unsigned char flag);
typedef  void (*ctSgw_appCtxDestroy)(void** p_app_ctx);
typedef  int (*ctSgw_appProcAppId)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo, void *p_app_ctx, unsigned int *layer7_id);

typedef struct ctSgw_dpiFuncs_ {
    ctSgw_appCtxCreate  ctSgw_appCtxCreateHook;
    ctSgw_appCtxDestroy ctSgw_appCtxDestroyHook;
    ctSgw_appProcAppId  ctSgw_appProcAppIdHook;
} ctSgw_dpiFuncs;
 
int ctSgw_appRegisterFunc(ctSgw_dpiFuncs  *funcs);
void ctSgw_appUnRegisterFunc(void);


#endif /* _MAXNET_DPI_IF_H_ */
