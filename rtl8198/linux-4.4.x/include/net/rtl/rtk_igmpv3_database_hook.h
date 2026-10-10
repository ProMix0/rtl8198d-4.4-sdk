#ifndef __IGMPV3MLDV2_DATABASE_HOOK_H__
#define __IGMPV3MLDV2_DATABASE_HOOK_H__

#include "rtk_igmpv3_database_common.h"

typedef struct rtk_igmpv3_database_hook_db
{
	unsigned char	valid;
	unsigned int snooping_moduleIndex;
	struct net_bridge	*br;
}RTK_IGMPV3_DB_T,*RTK_IGMPV3_DB_Tp;

enum RTK_IGMPV3_LOG_LEVEL
{
	RTK_IGMPV3_LOG_ERR = 0,
	RTK_IGMPV3_LOG_WARNING,
	RTK_IGMPV3_LOG_INFO,
	RTK_IGMPV3_LOG_DEBUG,
	RTK_IGMPV3_LOG_DUMPPKT,
};

typedef struct rtk_igmpv3_database_hook_Param
{
	enum RTK_IGMPV3_LOG_LEVEL log_level;
}RTK_IGMPV3_PARAM_T,*RTK_IGMPV3_PARAM_Tp;


int rtk_igmpv3_database_hook_enter(unsigned short proto,struct net_bridge *br,unsigned int port_num,unsigned char *skb_data, unsigned short vid);
int rtk_igmpv3_database_hook_get_fwdPortMask(struct net_bridge *br,unsigned int *sip,unsigned int *gip,unsigned short proto,unsigned int *fwdPortMask);
int rtk_igmpv3_database_hook_freegGroupEntry(struct net_bridge *br,unsigned short proto,unsigned int *gip);
int rtk_igmpv3_database_hook_freePortEntry(struct net_bridge *br,unsigned short port_no);
int  rtk_igmpv3_database_hook_registerModule(struct net_bridge *br);
int  rtk_igmpv3_database_hook_unregisterModule(struct net_bridge *br);
int rtk_igmpv3_database_hook_groupEntryState_ops(struct net_bridge *br,unsigned short proto,unsigned int *gip,
		int ops,enum RTK_IGMPV3_GIP_STATE *state, enum RTK_IGMPV3_GIP_STATE_TOGGLE *toggle);
int rtk_igmpv3_database_hook_init(void);
void  rtk_igmpv3_database_hook_exit(void);


#define RTK_IGMPV3_SUCCESS			(0)
/* errno */
#define RTK_IGMPV3_EEXEC			(1)
#define RTK_IGMPV3_ETBLFULL			(2)
#define RTK_IGMPV3_ETBLMISS			(3)
#define RTK_IGMPV3_EENTRYMISS		(4)

enum RTK_IGMPV3_PROC_CMD
{
	RTK_IGMPV3_SET = 0,
	RTK_IGMPV3_GET,
};

#define RTK_MAX_BRIDGE_NUM	8

#endif
