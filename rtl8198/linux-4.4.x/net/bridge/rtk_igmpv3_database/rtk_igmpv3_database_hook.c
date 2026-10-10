#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/inet.h>
#ifdef CONFIG_PROC_FS
#include <linux/seq_file.h>
#endif
#include "../br_private.h"
#include "igmpsnooping/rtl865x_igmpsnooping_glue.h"
#include "igmpsnooping/rtl_glue.h"
#include "igmpsnooping/rtl865x_igmpsnooping.h"
#include <net/rtl/rtk_igmpv3_database_hook.h>

/* lock */
static DEFINE_SPINLOCK(rtk_igmpv3);

/* extern */
extern struct proc_dir_entry proc_root;

/* global */
static struct proc_dir_entry *proc_rtk_igmpv3 = NULL;
static RTK_IGMPV3_PARAM_T rtk_igmpv3_param;
static RTK_IGMPV3_DB_T rtk_igmpv3_db[RTK_MAX_BRIDGE_NUM];
/* debug */
static void rtk_igmpv3_database_hook_printf( int serverity, const char *fmt, ... )
{
	unsigned char log[256] = {0};
	unsigned int len = 0;
  	va_list arg;

	va_start(arg,fmt);
    len += vsnprintf( log, sizeof( log ), fmt, arg);
	va_end(arg);

	if(serverity < rtk_igmpv3_param.log_level)
		printk(KERN_INFO "%s",log);

	return;
}

/* HOOK start */
int rtk_igmpv3_database_hook_enter(unsigned short proto,struct net_bridge	*br,unsigned int port_num,unsigned char *data, unsigned short vid)
{
	int i = 0;
	unsigned int fwdPortMask = 0;

#ifndef CONFIG_IPV6
	if(proto == htons(ETH_P_IPV6))
		return RTK_IGMPV3_SUCCESS;
#endif

	if(!br || !data){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_WARNING,"%s: param is not correct\n",__func__);
		if(br)
			rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:bridge name=%s,port_num=%d,vid=%d\n",__func__,br->dev->name,port_num,vid);
		if(data){
			rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:dump 0x30 data\n",__func__);
			for(i = 0;i < 0x30;i++){
				if((i % 0x10) == 0)
					rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"\n");
				rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"0x%02x ",*(data+i));
			}
		}
		dump_stack();
		return -RTK_IGMPV3_EEXEC;
	}

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid && rtk_igmpv3_db[i].br == br)
			break;
	}

	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_WARNING,"%s:rtk_igmpv3_db tbl miss,bridge name=%s\n",__func__,br->dev->name);
		return -RTK_IGMPV3_ETBLMISS;
	}

	spin_lock_bh(&rtk_igmpv3);
	if(rtl_igmpMldProcess(rtk_igmpv3_db[i].snooping_moduleIndex, data, port_num, &fwdPortMask) == FAILED){
		spin_unlock_bh(&rtk_igmpv3);
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:rtl_igmpMldProcess fail\n",__func__);
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:bridge name=%s,port_num=%d,vid=%d\n",__func__,br->dev->name,port_num,vid);
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:dump 0x30 data\n",__func__);
		for(i = 0;i < 0x30;i++){
			if((i % 0x10) == 0)
				rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"\n");
			rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"0x%02x ",*(data+i));		
		}
		dump_stack();
		
		return -RTK_IGMPV3_EEXEC;
	}

	spin_unlock_bh(&rtk_igmpv3);
	return RTK_IGMPV3_SUCCESS;
}

int rtk_igmpv3_database_hook_get_fwdPortMask(struct net_bridge *br,unsigned int *sip,unsigned int *gip,unsigned short proto,unsigned int *fwdPortMask)
{
	int ret = FAILED,i = 0;
	unsigned long flags = 0;
	struct iphdr *iph=NULL;
	struct rtl_multicastDataInfo multicastDataInfo;
	struct rtl_multicastFwdInfo multicastFwdInfo;

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid && rtk_igmpv3_db[i].br == br)
			break;
	}

	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_WARNING,"%s:rtk_igmpv3_db tbl miss,bridge name=%s\n",__func__,br->dev->name);
		return -RTK_IGMPV3_ETBLMISS;
	}

	switch(proto){
		case htons(ETH_P_IP):
			multicastDataInfo.ipVersion = IP_VERSION4;
			memcpy(&multicastDataInfo.sourceIp[0],sip,sizeof(multicastDataInfo.sourceIp[0]));
			memcpy(&multicastDataInfo.groupAddr[0],gip,sizeof(multicastDataInfo.groupAddr[0]));
		break;
#if IS_ENABLED(CONFIG_IPV6)
		case htons(ETH_P_IPV6):
			multicastDataInfo.ipVersion = IP_VERSION6;
			memcpy(multicastDataInfo.sourceIp,sip,sizeof(multicastDataInfo.sourceIp));
			memcpy(multicastDataInfo.groupAddr,gip,sizeof(multicastDataInfo.groupAddr));
		break;
#endif
		default:
			printk(KERN_ERR "ip proto=%d not support\n",htons(proto));
			return -1;
	}

	spin_lock_bh(&rtk_igmpv3);
	ret= rtl_getMulticastDataFwdInfo(rtk_igmpv3_db[i].snooping_moduleIndex, &multicastDataInfo, &multicastFwdInfo);
	if(ret == FAILED){
		spin_unlock_bh(&rtk_igmpv3);
		if(multicastFwdInfo.reservedMCast != TRUE){
			rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:rtl_getMulticastDataFwdInfo fail\n",__func__);
#if IS_ENABLED(CONFIG_IPV6)
			if(proto == htons(ETH_P_IPV6))
				rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:snooping_moduleIndex=%d,bridge name=%s,SIP=%pI6,GIP=%pI6,proto=0x%x\n",__func__,rtk_igmpv3_db[i].snooping_moduleIndex,br->dev->name,sip,gip,proto);
			else
#endif
				rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:snooping_moduleIndex=%d,bridge name=%s,SIP=%pI4,GIP=%pI4,proto=0x%x\n",__func__,rtk_igmpv3_db[i].snooping_moduleIndex,br->dev->name,sip,gip,proto);
		}
		
		return -RTK_IGMPV3_EEXEC;
	}

	*fwdPortMask = multicastFwdInfo.fwdPortMask;
	spin_unlock_bh(&rtk_igmpv3);
	
	return RTK_IGMPV3_SUCCESS;
	
}

int rtk_igmpv3_database_hook_freegGroupEntry(struct net_bridge *br,unsigned short proto,unsigned int *gip)
{	
	int i = 0;
	unsigned int ipVersion = IP_VERSION4;
	unsigned long flags = 0;

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid && rtk_igmpv3_db[i].br == br)
			break;
	}

	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_WARNING,"%s:rtk_igmpv3_db tbl miss,bridge name=%s\n",__func__,br->dev->name);
		return -RTK_IGMPV3_ETBLMISS;
	}

	switch(proto){
		case htons(ETH_P_IP):
			ipVersion = IP_VERSION4;
		break;
#if IS_ENABLED(CONFIG_IPV6)
		case htons(ETH_P_IPV6):
			ipVersion = IP_VERSION6;
		break;
#endif
		default:
			return -1;
	}
	spin_lock_bh(&rtk_igmpv3);
	rtk_igmpv3_igmpsnooping_hook_freeGroupEntry(rtk_igmpv3_db[i].snooping_moduleIndex,ipVersion,gip);
	spin_unlock_bh(&rtk_igmpv3);
	return RTK_IGMPV3_SUCCESS;
}

int rtk_igmpv3_database_hook_freePortEntry(struct net_bridge *br,unsigned short port_no)
{
	int i = 0;
	unsigned long flags = 0;
	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid && rtk_igmpv3_db[i].br == br)
			break;
	}

	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_WARNING,"%s:rtk_igmpv3_db tbl miss,bridge name=%s\n",__func__,br->dev->name);
		return -RTK_IGMPV3_ETBLMISS;
	}
	spin_lock_bh(&rtk_igmpv3);
	rtk_igmpv3_igmpsnooping_hook_FreePortEntry(rtk_igmpv3_db[i].snooping_moduleIndex,port_no);
	spin_unlock_bh(&rtk_igmpv3);
	return RTK_IGMPV3_SUCCESS;
}

/* add/del bridge call */
int  rtk_igmpv3_database_hook_registerModule(struct net_bridge *br)
{
	int retVal = FAILED,i = 0;
	struct rtl_mCastTimerParameters mCastTimerParameters;

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid == 0)
			break;
	}
	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"rtk_igmpv3_db tbl full,bridge name=%s\n",br->dev->name);
		return -RTK_IGMPV3_ETBLFULL;
	}
	memset(&rtk_igmpv3_db[i],0,sizeof(rtk_igmpv3_db[i]));

	/* update bridge param to igmpv3 snooping module */
	/* per bridge to global */
	if(i == 0){
		memset(&mCastTimerParameters,0,sizeof(mCastTimerParameters));
		mCastTimerParameters.groupMemberAgingTime = br->multicast_membership_interval/HZ;
		mCastTimerParameters.lastMemberAgingTime = br->multicast_last_member_interval/HZ;
		rtl_setMulticastParameters(	mCastTimerParameters);	
	}
	
	retVal=rtl_registerIgmpSnoopingModule(&rtk_igmpv3_db[i].snooping_moduleIndex);
	if(retVal == FAILED){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"register igmpv3_database_hook snooping module fail,bridge name=%s\n",br->dev->name);
		return -RTK_IGMPV3_EEXEC;
	}

	/* enable fastleave */
	rtl_configMulticastSnoopingFastLeave(1,0);
	
	rtk_igmpv3_db[i].valid = 1;
	rtk_igmpv3_db[i].br = br;
	
	return RTK_IGMPV3_SUCCESS;
}

int  rtk_igmpv3_database_hook_unregisterModule(struct net_bridge *br)
{
	int retVal = FAILED,i = 0;;

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid && rtk_igmpv3_db[i].br == br)
			break;
	}
	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"rtk_igmpv3_db tbl miss,bridge name=%s\n",br->dev->name);
		return -RTK_IGMPV3_ETBLMISS;
	}

	if(rtl_unregisterIgmpSnoopingModule(rtk_igmpv3_db[i].snooping_moduleIndex) != SUCCESS){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"unregister igmpv3_database_hook snooping module fail,bridge name=%s\n",br->dev->name);
		return -RTK_IGMPV3_EEXEC;
	}

	return RTK_IGMPV3_SUCCESS;
}

int rtk_igmpv3_database_hook_groupEntryState_ops(struct net_bridge *br,unsigned short proto,unsigned int *gip,
		int ops,enum RTK_IGMPV3_GIP_STATE *state, enum RTK_IGMPV3_GIP_STATE_TOGGLE *toggle)
{
	int i = 0;
	unsigned int ipVersion = IP_VERSION4;

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid && rtk_igmpv3_db[i].br == br)
			break;
	}

	if(i >= RTK_MAX_BRIDGE_NUM){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_WARNING,"%s:rtk_igmpv3_db tbl miss,bridge name=%s\n",__func__,br->dev->name);
		return -RTK_IGMPV3_ETBLMISS;
	}

	switch(proto){
		case htons(ETH_P_IP):
			ipVersion = IP_VERSION4;
		break;
#if IS_ENABLED(CONFIG_IPV6)
		case htons(ETH_P_IPV6):
			ipVersion = IP_VERSION6;
		break;
#endif
		default:
			return -1;
	}

	spin_lock_bh(&rtk_igmpv3);
	if(rtk_igmpv3_igmpsnooping_hook_groupEntryState_ops(rtk_igmpv3_db[i].snooping_moduleIndex,ipVersion,gip,ops,state,toggle) != 0){
		spin_unlock_bh(&rtk_igmpv3);
		if(ipVersion == IP_VERSION4)
			rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:set gip=%pI4 toggle fail,ops=%d,ipVersion=%d, br_name=%s\n",__func__,gip,ops,ipVersion,br->dev->name);
		else
			rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"%s:set gip=%pI6 toggle fail,ops=%d,ipVersion=%d, br_name=%s\n",__func__,gip,ops,ipVersion,br->dev->name);
		
		return -RTK_IGMPV3_EEXEC;
	}
	spin_unlock_bh(&rtk_igmpv3);

	return RTK_IGMPV3_SUCCESS;
}

/* HOOK end */

/* proc start */
static int rtk_igmpv3_param_show_help(struct seq_file *m, void *v)
{
	seq_printf(m, "\necho cmd param value > /proc/rtk_igmpv3/param\n");
	return 0;
}

int rtk_igmpv3_param_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_igmpv3_param_show_help, (void *) file);
}

ssize_t rtk_igmpv3_param_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char tmp[32];
	ssize_t len;
	char *token, *cp = tmp;
	unsigned long value;
	enum RTK_IGMPV3_PROC_CMD cmd;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		return count;
	tmp[len] = '\0';


	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if(!strncmp(token,"set",strlen("set")))
			cmd = RTK_IGMPV3_SET;
		else if(!strncmp(token,"get",strlen("get")))
			cmd = RTK_IGMPV3_GET;
		else{
			printk(KERN_ERR "first token=%s not support\n",token);
			return count;
		}
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if(!strncmp(token,"log_level",strlen("log_level"))){
			if(cmd == RTK_IGMPV3_SET){
				token = strsep(&cp, " \n");
				if (token != NULL)
					rtk_igmpv3_param.log_level = simple_strtoul(token,NULL,0);
				else
					printk(KERN_ERR "echo set log_level value > /proc/rtk_igmpv3/param\n");
			}else
				printk(KERN_INFO "log_level=%d\n",rtk_igmpv3_param.log_level);
		}
	}else{
		printk(KERN_ERR "second token=%s not support\n",token);
		return count;
	}
	return count;
}

static const struct file_operations fops_rtk_igmpv3_param __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= rtk_igmpv3_param_proc_open,
	.read		= seq_read,
	.write		= rtk_igmpv3_param_proc_write,
};

static int rtk_igmpv3_database_show(struct seq_file *m, void *v)
{
	igmp_show(m,v);
	return 0;
}

int rtk_igmpv3_database_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_igmpv3_database_show, (void *) file);
}

ssize_t rtk_igmpv3_database_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	return igmp_write(file,buf,count,pos);
}

static const struct file_operations fops_rtk_igmpv3_database __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= rtk_igmpv3_database_proc_open,
	.read		= seq_read,
	.write		= rtk_igmpv3_database_proc_write,
};

static int rtk_igmpv3_setGipState_show(struct seq_file *m, void *v)
{
	return 0;
}

int rtk_igmpv3_setGipState_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_igmpv3_setGipState_show, (void *) file);
}

ssize_t rtk_igmpv3_setGipState_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	int i =0;
	unsigned short proto;
	int af;
	char tmp[64];
	ssize_t len;
	char *token, *cp = tmp;
	unsigned long value;
	unsigned int gip[4] = {0};
	enum RTK_IGMPV3_GIP_STATE state;
	enum RTK_IGMPV3_GIP_STATE_TOGGLE toggle;

	len = min(count, (unsigned long)(sizeof(tmp) - 1));
	if (copy_from_user(tmp, buf, len))
		return count;
	tmp[len] = '\0';

	/* parse 1st token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if(!strncmp(token,"i4",strlen("i4")))
			af = AF_INET;
#if IS_ENABLED(CONFIG_IPV6)
		else if(!strncmp(token,"i6",strlen("i6")))
			af = AF_INET6;
#endif
		else{
			printk(KERN_ERR "first token=%s not support\n",token);
			return count;
		}
	}

	/* parse 2nd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if(af == AF_INET){
			if(in4_pton(token, strlen(token), gip, '\0', NULL) == 0){
				printk(KERN_ERR "second token=%s not support\n",token);
				return count;
			}
		}
#if IS_ENABLED(CONFIG_IPV6)
		else if(af == AF_INET6){
			if(in6_pton(token, strlen(token), gip, '\0', NULL) == 0){
				printk(KERN_ERR "second token=%s not support\n",token);
				return count;
			}
		}
#endif
	}

	/* parse 3rd token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if(!strncmp(token,"inV2",strlen("inV2")))
			state = RTK_IGMPV3_GIP_STATE_V2;
		else if(!strncmp(token,"inV3",strlen("inV3")))
			state = RTK_IGMPV3_GIP_STATE_V3;
		else if(!strncmp(token,"init",strlen("init")))
			state = RTK_IGMPV3_GIP_STATE_INIT;
		else{
			printk(KERN_ERR "third token=%s not support\n",token);
			return count;
		}
	}

	/* parse 4th token */
	token = strsep(&cp, " \n");
	if (token != NULL) {
		if(!strncmp(token,"toV2",strlen("toV2")))
			toggle = RTK_IGMPV3_GIP_STATE_TOGGLE_TOV2;
		else if(!strncmp(token,"toV3",strlen("toV3")))
			toggle = RTK_IGMPV3_GIP_STATE_TOGGLE_TOV3;
		else if(!strncmp(token,"disabled",strlen("disabled")))
			toggle = RTK_IGMPV3_GIP_STATE_TOGGLE_DISABLED;
		else{
			printk(KERN_ERR "fourth token=%s not support\n",token);
			return count;
		}
	}

	switch(af){
		case AF_INET:
			proto = htons(ETH_P_IP);
		break;
#if IS_ENABLED(CONFIG_IPV6)
		case AF_INET6:
			proto = htons(ETH_P_IPV6);
		break;
#endif
		default:
			return -1;
	}

	for(i = 0;i < RTK_MAX_BRIDGE_NUM;i++){
		if(rtk_igmpv3_db[i].valid){
			if(af==AF_INET)
				printk(KERN_INFO "br name= %s,set gip=%pI4 state=%d toggle=%d\n",rtk_igmpv3_db[i].br->dev->name,gip,state,toggle);
#if IS_ENABLED(CONFIG_IPV6)
			else
				printk(KERN_INFO "br name= %s,set gip=%pI6 state=%d toggle=%d\n",rtk_igmpv3_db[i].br->dev->name,gip,state,toggle);
#endif
			if(rtk_igmpv3_database_hook_groupEntryState_ops(rtk_igmpv3_db[i].br,proto,gip,RTK_IGMPV3_GIP_STATE_SET,&state,&toggle) != RTK_IGMPV3_SUCCESS)
				printk(KERN_ERR "br name= %sset gip=%pI6 state=%d toggle=%d fail\n",rtk_igmpv3_db[i].br->dev->name,gip,state,toggle);
		}
	}
		
	return count;
}

static const struct file_operations fops_rtk_igmpv3_setGipState __read_mostly = {
	.owner		= THIS_MODULE,
	.open		= rtk_igmpv3_setGipState_proc_open,
	.read		= seq_read,
	.write		= rtk_igmpv3_setGipState_proc_write,
};

/* proc end */

/* init/exit */
int rtk_igmpv3_database_hook_init(void)
{
	int retVal = FAILED;
	struct rtl_mCastSnoopingGlobalConfig mCastSnoopingGlobalConfig;
	/* param init */
	memset(&rtk_igmpv3_param,0,sizeof(rtk_igmpv3_param));
	memset(rtk_igmpv3_db,0,sizeof(rtk_igmpv3_db));
	rtk_igmpv3_param.log_level = RTK_IGMPV3_LOG_ERR;
	
	/* init */
	memset(&mCastSnoopingGlobalConfig, 0, sizeof(struct rtl_mCastSnoopingGlobalConfig));
	mCastSnoopingGlobalConfig.maxGroupNum=256;
	mCastSnoopingGlobalConfig.maxSourceNum=300;
	mCastSnoopingGlobalConfig.hashTableSize=64;

	mCastSnoopingGlobalConfig.groupMemberAgingTime=600;
	mCastSnoopingGlobalConfig.lastMemberAgingTime=2;
	mCastSnoopingGlobalConfig.querierPresentInterval=260;

	mCastSnoopingGlobalConfig.dvmrpRouterAgingTime=120;
	mCastSnoopingGlobalConfig.mospfRouterAgingTime=120;
	mCastSnoopingGlobalConfig.pimRouterAgingTime=120;

	retVal=rtl_initMulticastSnooping(mCastSnoopingGlobalConfig);
	if(retVal == FAILED){
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"init igmpv3_database_hook fail\n");
		return -RTK_IGMPV3_EEXEC;
	}

	/* proc create */
	proc_rtk_igmpv3 = proc_mkdir("rtk_igmpv3", &proc_root);
	
	if (proc_create("param", S_IRUGO | S_IWUGO, proc_rtk_igmpv3, &fops_rtk_igmpv3_param) == NULL) {
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"Fail to create proc entry rtk_igmpv3/param.\n");
		return -RTK_IGMPV3_EEXEC;
	}

	if (proc_create("database", S_IRUGO | S_IWUGO, proc_rtk_igmpv3, &fops_rtk_igmpv3_database) == NULL) {
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"Fail to create proc entry rtk_igmpv3/database.\n");
		return -RTK_IGMPV3_EEXEC;
	}

	if (proc_create("setGipState", S_IRUGO | S_IWUGO, proc_rtk_igmpv3, &fops_rtk_igmpv3_setGipState) == NULL) {
		rtk_igmpv3_database_hook_printf(RTK_IGMPV3_LOG_ERR,"Fail to create proc entry rtk_igmpv3/setGipState.\n");
		return -RTK_IGMPV3_EEXEC;
	}
	
	return RTK_IGMPV3_SUCCESS;
}

void  rtk_igmpv3_database_hook_exit(void)
{
	rtl_exitMulticastSnooping();
	memset(rtk_igmpv3_db,0,sizeof(rtk_igmpv3_db));
	memset(&rtk_igmpv3_param,0,sizeof(rtk_igmpv3_param));

	/* proc delete */
	remove_proc_entry("database", proc_rtk_igmpv3);
	remove_proc_entry("param", proc_rtk_igmpv3);
	remove_proc_entry("setGipState", proc_rtk_igmpv3);
	proc_remove(proc_rtk_igmpv3);
}


EXPORT_SYMBOL(rtk_igmpv3_database_hook_enter);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_get_fwdPortMask);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_freegGroupEntry);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_freePortEntry);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_groupEntryState_ops);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_registerModule);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_unregisterModule);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_init);
EXPORT_SYMBOL(rtk_igmpv3_database_hook_exit);


