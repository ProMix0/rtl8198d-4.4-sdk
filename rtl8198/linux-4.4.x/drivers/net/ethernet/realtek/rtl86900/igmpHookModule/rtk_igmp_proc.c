#include <br_private.h>
#include <linux/proc_fs.h>
// #include <linux/seq_file.h>
// #include <linux/inet.h>
#include <fs/proc/internal.h>

#include <rtk_igmp_proc.h>
#include <rtk_igmp_struct.h>
#include <rtk_igmp_hook.h>
#include <rtk_igmp_debug.h>
#include <rtk_igmp_snooping.h>


struct proc_dir_entry *rtk_igmp_proc_dir=NULL;
struct proc_dir_entry *rtk_igmp_dump_proc_dir=NULL;
struct proc_dir_entry *rtk_igmp_ctrl_proc_dir=NULL;

int rtk_igmp6_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp6_proc_read(struct seq_file *s, void *v);
int rtk_igmp_proc_read(struct seq_file *s, void *v);
int rtk_igmp_mrouteMapping_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_mrouteMapping_read(struct seq_file *s, void *v);
int rtk_igmp_debug_read(struct seq_file *s, void *v);
int rtk_igmp_debug_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_br_proc_read(struct seq_file *s, void *v);
int rtk_igmp_br_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_whiteList_proc_read(struct seq_file *s, void *v);
int rtk_igmp_whiteList_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_ignoreGroup_proc_read(struct seq_file *s, void *v);
int rtk_igmp_ignoreGroup_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_igmphook_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_igmphook_proc_read(struct seq_file *s, void *v);
int rtk_igmp_igmphwctrl_proc_read(struct seq_file *s, void *v);
int rtk_igmp_igmphwctrl_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_groupWeight_proc_read(struct seq_file *s, void *v);
int rtk_igmp_groupWeight_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_devStatisic_read(struct seq_file *s, void *v);
int rtk_igmp_devStatisic_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_nextGroupStatisic_read(struct seq_file *s, void *v);
int rtk_igmp_nextGroupStatisic_write(struct file *file, const char *buffer, unsigned long count, void *data);
int rtk_igmp_unknownFlood_proc_read(struct seq_file *s, void *v);
int rtk_igmp_unknownFlood_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);



rtk_igmp_proc_t igmpProc[]=
{
	//dump
	{
		.name="devConfig" ,
		.get = rtk_igmp_dump_allDevConfig ,
		.set = NULL ,
		.dir = PROC_DIR_IGMP_DUMP,
	},

	{
		.name="igmpInfo" ,
		.get = rtk_igmp_allDump ,
		.set = NULL ,
		.dir = PROC_DIR_IGMP_DUMP,
	},
	{
		.name="igmpDebug" ,
		.get = rtk_igmp_debug_read,
		.set = rtk_igmp_debug_write ,
		.dir = PROC_DIR_IGMP_DUMP,
	},
	{
		.name="devStatisic" ,
		.get = rtk_igmp_devStatisic_read,
		.set = rtk_igmp_devStatisic_write ,
		.dir = PROC_DIR_IGMP_DUMP,
	},
	{
		.name="nextGroupStatisic" ,
		.get = rtk_igmp_nextGroupStatisic_read,
		.set = rtk_igmp_nextGroupStatisic_write ,
		.dir = PROC_DIR_IGMP_DUMP,
	},


	//control
	{
		.name="igmpConfig" ,
		.get = rtk_igmp_proc_read ,
		.set = rtk_igmp_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},
	{
		.name="igmp6Config" ,
		.get = rtk_igmp6_proc_read ,
		.set = rtk_igmp6_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},

	{
		.name="brConfig" ,
		.get = rtk_igmp_br_proc_read ,
		.set = rtk_igmp_br_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},
	{
		.name="whiteList" ,
		.get = rtk_igmp_whiteList_proc_read ,
		.set = rtk_igmp_whiteList_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},
	{
		.name="ignoreGroup" ,
		.get = rtk_igmp_ignoreGroup_proc_read ,
		.set = rtk_igmp_ignoreGroup_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},	
	{
		.name="groupWeight" ,
		.get = rtk_igmp_groupWeight_proc_read ,
		.set = rtk_igmp_groupWeight_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},		
	{
		.name="igmphook" ,
		.get = rtk_igmp_igmphook_proc_read ,
		.set = rtk_igmp_igmphook_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},
	{
		.name="hwControl" ,
		.get = rtk_igmp_igmphwctrl_proc_read ,
		.set = rtk_igmp_igmphwctrl_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},
	{
		.name="unknownFlood" ,
		.get = rtk_igmp_unknownFlood_proc_read ,
		.set = rtk_igmp_unknownFlood_proc_write ,
		.dir = PROC_DIR_IGMP_CTRL,
	},


#if 0
	{
		.name="mrouteMapping" ,
		.get = rtk_igmp_mrouteMapping_read ,
		.set = rtk_igmp_mrouteMapping_write,
		.dir = PROC_DIR_IGMP_CTRL,
	},
#endif

};



static void *rtk_igmp_single_start(struct seq_file *p, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *rtk_igmp_single_next(struct seq_file *p, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void rtk_igmp_single_stop(struct seq_file *p, void *v)
{
}

int rtk_igmp_seq_open(struct file *file, const struct seq_operations *op)
{
	struct seq_file *p = file->private_data;

	if (!p) {
			p = kmalloc(sizeof(*p), GFP_ATOMIC);
			if (!p){
				return -ENOMEM;
			}
			file->private_data = p;
	}
	memset(p, 0, sizeof(*p));
	mutex_init(&p->lock);
	p->op = op;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 79)
	p->file = file;
#else
#ifdef CONFIG_USER_NS
	p->user_ns = file->f_cred->user_ns;
#endif
#endif

	/*
	 * Wrappers around seq_open(e.g. swaps_open) need to be
	 * aware of this. If they set f_version themselves, they
	 * should call seq_open first and then set f_version.
	 */
	file->f_version = 0;

	/*
	 * seq_files support lseek() and pread().  They do not implement
	 * write() at all, but we clear FMODE_PWRITE here for historical
	 * reasons.
	 *
	 * If a client of seq_files a) implements file.write() and b) wishes to
	 * support pwrite() then that client will need to implement its own
	 * file.open() which calls seq_open() and then sets FMODE_PWRITE.
	 */
	file->f_mode &= ~FMODE_PWRITE;
	return 0;
}


int rtk_igmp_single_open(struct file *file, int (*show)(struct seq_file *, void *),void *data)
{
	struct seq_operations *op = kmalloc(sizeof(*op), GFP_ATOMIC);
	int res = -ENOMEM;

	if (op) {
		op->start = rtk_igmp_single_start;
		op->next = rtk_igmp_single_next;
		op->stop = rtk_igmp_single_stop;
		op->show = show;
		res = rtk_igmp_seq_open(file, op);
		if (!res)
			((struct seq_file *)file->private_data)->private = data;
		else
			kfree(op);
	}
	return res;
}


static int rtk_igmp_nullDebugGet(struct seq_file *s, void *v)
{
       return 0;
}

static int rtk_igmp_nullDebugSingleOpen(struct inode *inode, struct file *file)
{
       return(single_open(file, rtk_igmp_nullDebugGet, NULL));
}

static int rtk_igmp_commonDebugSingleOpen(struct inode *inode, struct file *file)
{
    int i, ret = -1;

	igmp_spin_lock_bh(igmpSysdb.lock_igmp);
	//========================= Critical Section Start =========================//
    for( i=0; i< (sizeof(igmpProc)/sizeof(rtk_igmp_proc_t)) ;i++)
    {
		//printk("common_single_open inode_id=%u i_ino=%u\n",igmpProc[i].inode_id,(unsigned int)inode->i_ino);
        if(igmpProc[i].inode_id==(unsigned int)inode->i_ino)
        {
			ret = rtk_igmp_single_open(file, igmpProc[i].get, NULL);
			break;
        }
    }
	//========================= Critical Section End =========================//
	igmp_spin_unlock_bh(igmpSysdb.lock_igmp);

    return ret;
}

static ssize_t rtk_igmp_commonDebugSingleWrite(struct file * file, const char __user * userbuf,
				size_t count, loff_t * off)
{
    int i, ret = -1;
	char procBuffer[count];
	char *pBuffer = NULL;

	/* write data to the buffer */
	memset(procBuffer, 0, sizeof(procBuffer));
	if ( copy_from_user(procBuffer, userbuf, count) ) {
		return -EFAULT;
	}
	procBuffer[count-1] = '\0';
	pBuffer = procBuffer;

	igmp_spin_lock_bh(igmpSysdb.lock_igmp);
	//========================= Critical Section Start =========================//
    for( i=0; i< (sizeof(igmpProc)/sizeof(rtk_igmp_proc_t)) ;i++)
    {

		//printk("common_single_write inode_id=%u i_ino=%u\n",igmpProc[i].inode_id,(unsigned int)file->f_dentry->d_inode->i_ino);
        if(igmpProc[i].inode_id==(unsigned int)file->f_inode->i_ino)
        {
			if(igmpProc[i].unlockBefortWrite)
				igmp_spin_unlock_bh(igmpSysdb.lock_igmp);
			ret = igmpProc[i].set(file,pBuffer,count,off);
			break;
        }
    }
	//========================= Critical Section End =========================//
	if((i!=(sizeof(igmpProc)/sizeof(rtk_igmp_proc_t))) && !igmpProc[i].unlockBefortWrite)
		igmp_spin_unlock_bh(igmpSysdb.lock_igmp);

    return ret;
}


extern int in4_pton(const char *src, int srclen, u8 *dst, int delim, const char **end);
extern int in6_pton(const char *src, int srclen, u8 *dst, int delim, const char **end);
extern char * strstr(const char *,const char *);

enum rtk_igmp_parseDataType
{
   MAC=0,
   V4IP,
   V6IP,
   STRING_EN_DIS, //warning : only use for uint32* parameterGetAddr
   UNSIGNED_LONG, //uint32
   SINGED_LONG,	  //int32
   STRING,		  //warning : should sure string non-overflow
};

void _rtk_igmp_str2mac(unsigned char *mac_string,rtk_mac_t *pMacEntry)
{
	int i,j=0,k=0;
	memset(pMacEntry,0,sizeof(rtk_mac_t));
	for(i=0;i<strlen(mac_string);i++)
	{
		if(mac_string[i]==':')
		{
			j=0;
			continue;
		}
		else if((mac_string[i]>='A')&&(mac_string[i]<='F'))
			pMacEntry->octet[k]+=(mac_string[i]-'A'+10);
		else if((mac_string[i]>='a')&&(mac_string[i]<='f'))
			pMacEntry->octet[k]+=(mac_string[i]-'a'+10);
		else if((mac_string[i]>='0')&&(mac_string[i]<='9'))
			pMacEntry->octet[k]+=(mac_string[i]-'0');
		else
			IGMP_CTRL("str2mac MAC string parsing error!");
		if(j==0) pMacEntry->octet[k]<<=4;
		if(j==1) k++;
		j++;
	}
}


int32 _rtk_igmp_parseGetValue(char* stringBuff,void *parameterGetAddr,char* parameterName,enum rtk_igmp_parseDataType dataType)
{
	char* strPtr;
	uint32 stringSize=0;
	uint32 netv4ip;

	strPtr = stringBuff;

	while(1)
	{
		strPtr=strstr(strPtr,parameterName);
		if(strPtr==NULL) //string not find
		{
			//printk("stringBuff %s \n%s not find\n",stringBuff,parameterName);
			return FAILED;
		}
		//DEBUG("%s type=%d posDif=%d  sizeof:%d   [%d][%d]\n", parameterName,dataType, (int)(strPtr-stringBuff),(int)strlen(parameterName),((strPtr-stringBuff)!=0  && ((*(strPtr-1)!=' ') ||(*(strPtr-1)!=',') )  ),( (*(strPtr+strlen(parameterName)) != ' ') || (*(strPtr+strlen(parameterName)) != ',')));
		if(((strPtr-stringBuff)!=0  && ((*(strPtr-1)!=' ') && (*(strPtr-1)!=',') )  ) ||
			( (*(strPtr+strlen(parameterName)) != ' ') && (*(strPtr+strlen(parameterName)) != ',')))
		{
			//not this string maybe a sub-string goto find next
			strPtr+=strlen(parameterName);
			while((*strPtr==' ') || (*strPtr==',')) //ingore " " and ","
				strPtr++;
		}
		if( (*(strPtr+strlen(parameterName)) == ' ') || (*(strPtr+strlen(parameterName)) == ','))
			break;
	}
	strPtr+=strlen(parameterName);
	while((*strPtr==' ') || (*strPtr==',')) //ingore " " and ","
		strPtr++;


	if(*strPtr<0x20 ||  *strPtr==0x7f)//out of ascii char range
		return FAILED;

//	printk("%s type=%d posDif=%d  sizeof:%d\n",parameterName,dataType,strPtr-stringBuff,strlen(parameterName));

	switch(dataType)
	{
		case MAC:
			_rtk_igmp_str2mac(strPtr,(rtk_mac_t*)(parameterGetAddr));
			break;
		case V4IP:
			//*((uint32 *)parameterGetAddr) = (uint32)in_aton(strPtr);
			in4_pton(strPtr,-1,(uint8*)&netv4ip,-1,NULL);
			*(uint32*)parameterGetAddr= ntohl(netv4ip);
			break;
		case V6IP:
			in6_pton(strPtr,-1,(uint8*)parameterGetAddr,-1,NULL);
			break;
		case UNSIGNED_LONG:
			*((uint32*)parameterGetAddr)= simple_strtoul(strPtr,NULL,0);
			break;
		case SINGED_LONG:
			*((int32*)parameterGetAddr)= simple_strtol(strPtr,NULL,0);
			break;
		case STRING_EN_DIS:
			if(memcmp(strPtr,"enable",sizeof("enable")-1)==0){
				*((uint32*)parameterGetAddr)=1;
			}else if(memcmp(strPtr,"disable",sizeof("disable")-1)==0){
				*((uint32*)parameterGetAddr)=0;
			}else
				return FAILED;
			break;
		case STRING:
			while(1){
				if(*(strPtr+stringSize)<=0x20 ||  *(strPtr+stringSize)==0x7f)//out of ascii char range (include space)
					break;
				stringSize++;
			}
			memcpy(parameterGetAddr,strPtr,stringSize);
			break;
		default:
			return FAILED;

	}

	return SUCCESS;
}


int rtk_igmp_igmphook_proc_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("echo IGMPHOOK enable > proc/igmp/ctrl/igmphook\n");
	PROC_PRINTF("echo IGMPHOOK disable > proc/igmp/ctrl/igmphook\n");
	PROC_PRINTF("status:%s",igmpSysdb.disHook==0?"enable\n":"disable\n");
	return SUCCESS;

}
int rtk_igmp_igmphook_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;
	uint32 value;

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"IGMPHOOK",STRING_EN_DIS) ==SUCCESS)
		{
			if(value)//enable ==1 disable==0
				igmpSysdb.disHook=0;
			else
				igmpSysdb.disHook=1;
		}
	}
	rtk_igmp_igmphook_proc_read(NULL,NULL);

	return count;

}

int rtk_igmp_igmphwctrl_proc_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("echo HWCTRL enable > proc/igmp/ctrl/hwControl\n");
	PROC_PRINTF("echo HWCTRL disable > proc/igmp/ctrl/hwControl\n");
	PROC_PRINTF("status:%s",igmpSysdb.hwctrl==0?"enable":"disable\n");
	return SUCCESS;

}
int rtk_igmp_igmphwctrl_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;
	uint32 value;

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"HWCTRL",STRING_EN_DIS) ==SUCCESS)
		{
			if(value)//enable ==1 disable==0
			{
				igmpSysdb.hwctrl=0;
				rtk_igmp_hwCbReg();
			}
			else
			{
				igmpSysdb.hwctrl=1;
				rtk_igmp_hwCbUnReg();
			}
		}
	}
	rtk_igmp_igmphwctrl_proc_read(NULL,NULL);

	return count;

}


int _rtk_igmp_proc_pasring_string_to_integer(const char *buff,unsigned long len)
{
	char tmpbuf[256];
	int ret;

	memset(tmpbuf,0,sizeof(tmpbuf));
	if (buff)
	{
		/* copy data to the buffer */
		strncpy(tmpbuf, buff, len);

		tmpbuf[len-1] = '\0';
	}

	ret=simple_strtol(tmpbuf, NULL, 0);

	return ret;
}

int rtk_igmp_unknownFlood_proc_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Enable unknown multicast data flooding:\n");
	PROC_PRINTF("echo 1 > proc/igmp/ctrl/UnkownFlood\n");
	PROC_PRINTF("Disable unknown multicast data flooding:\n");
	PROC_PRINTF("echo 0 > proc/igmp/ctrl/UnkownFlood\n");
	PROC_PRINTF("status:%s",igmpSysdb.unknownFlood==0?"disable":"enable\n");
	return SUCCESS;

}

int rtk_igmp_unknownFlood_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	igmpSysdb.unknownFlood = _rtk_igmp_proc_pasring_string_to_integer(buffer,count);;
	rtk_igmp_unknownFlood_proc_read(NULL, NULL);

	return count;

}


int rtk_igmp_whiteList_proc_read(struct seq_file *s, void *v)
{

	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support ADD Patten: [DEVIFID/DEVIFNAME] [SMAC] [SMACMSK(SMAC option)] \n");
	PROC_PRINTF("					 [GIP4_START/GIP4_END] [GIP6_START/GIP6_END]\n");
	PROC_PRINTF("					 [SIP4_START/SIP4_END] [SIP6_START/SIP6_END]\n");
	PROC_PRINTF("					 [MCSIP4_START/MCSIP4_END] [MCSIP6_START/MCSIP6_END]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DEVIFNAME eth0.2 GIP4_START 224.1.2.3 GIP4_END 224.1.2.3 SIP4_START 192.168.1.7  SIP4_END 192.168.1.7 > proc/igmp/ctrl/whiteList\n");
	PROC_PRINTF("Support DEL Patten: [DELINDEX] [DELBY_PATTEN]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DELINDEX 0 > proc/igmp/ctrl/whiteList\n");
	PROC_PRINTF("echo DELBY_PATTEN 1 DEVIFNAME eth0.2 GIP4_START 224.2.3.4 GIP4_END 224.2.3.4 > proc/igmp/ctrl/whiteList\n");
	rtk_igmp_dump_devConfig(FAIL,NULL,NULL);
	return SUCCESS;
}

int rtk_igmp_whiteList_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;
	rt_igmpHook_whiteList_t patten;
	int index;
	uint32 v4ip=0;
	uint32 v6ip[4]={0};
	uint8  macaddr[6]={0};

	int32 devifid=FAIL;
	int32 whiteListIdx;
	char ifname[32]={0};

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&whiteListIdx,"DELINDEX",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_whiteListDel(NULL,0,whiteListIdx);
			rtk_igmp_dump_devConfig(devifid,NULL,NULL);
			return count;
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
		{
			if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
			{
				printk("DEVIFNAME parser error\n");
				goto DEV_ERROR;
			}
			else
			{
				devifid = rtk_igmp_devNameToDevIfidx(ifname);
			}
			if(devifid == FAIL)
			{
				printk("DEVIFNAME devifid FAIL\n");
				goto DEV_ERROR;
			}
		}

		memset(&patten,0,sizeof(patten));


		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4_START",V4IP) ==SUCCESS)
		{
			patten.gipChk=1;
			patten.wlGroupIpStart[0]=v4ip;
			patten.wlGroupIpEnd[0]=v4ip;
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4_END",V4IP) ==SUCCESS)
			{
				patten.wlGroupIpEnd[0]=v4ip;
			}
		}


		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"SIP4_START",V4IP) ==SUCCESS)
		{
			patten.sipChk=1;
			patten.wlSipStart[0]=v4ip;
			patten.wlSipEnd[0]=v4ip;
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"SIP4_END",V4IP) ==SUCCESS)
			{
				patten.wlSipEnd[0]=v4ip;
			}
		}


		if(_rtk_igmp_parseGetValue(tmpBuf,v6ip,"SIP6_START",V6IP) ==SUCCESS)
		{
			patten.sipChk=1;
			memcpy(patten.wlSipStart,v6ip,sizeof(patten.wlSipStart));
			memcpy(patten.wlSipEnd,v6ip,sizeof(patten.wlSipEnd));
			patten.isIpv6=1;
			if(_rtk_igmp_parseGetValue(tmpBuf,v6ip,"SIP6_END",V6IP) ==SUCCESS)
			{
				memcpy(patten.wlSipEnd,v6ip,sizeof(patten.wlSipEnd));
			}
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,macaddr,"SMAC",MAC) ==SUCCESS)
		{
			patten.smacChk=1;
			memcpy(patten.smac,macaddr,sizeof(patten.smac));
			memset(patten.smacMask,0xff,sizeof(patten.smacMask));
			if(_rtk_igmp_parseGetValue(tmpBuf,macaddr,"SMACMSK",MAC) ==SUCCESS)
			{
				memcpy(patten.smacMask,macaddr,sizeof(patten.smacMask));
			}
		}



		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"MCSIP4_START",V4IP) ==SUCCESS)
		{
			patten.mcSipChk=1;
			patten.wlMcSip[0]=v4ip;
			patten.wlMcSipEnd[0]=v4ip;
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"MCSIP4_END",V4IP) ==SUCCESS)
			{
				patten.wlMcSipEnd[0]=v4ip;
			}
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,v6ip,"MCSIP6_START",V6IP) ==SUCCESS)
		{
			patten.mcSipChk=1;
			memcpy(patten.wlMcSip,v6ip,sizeof(patten.wlMcSip));
			memcpy(patten.wlMcSipEnd,v6ip,sizeof(patten.wlMcSipEnd));
			patten.isIpv6=1;
			if(_rtk_igmp_parseGetValue(tmpBuf,v6ip,"MCSIP6_END",V6IP) ==SUCCESS)
			{
				memcpy(patten.wlMcSipEnd,v6ip,sizeof(patten.wlMcSipEnd));
			}
			
		}


		if(_rtk_igmp_parseGetValue(tmpBuf,&whiteListIdx,"DELBY_PATTEN",UNSIGNED_LONG) ==SUCCESS)
		{
			int ret;
			ret=rtk_igmp_whiteListDel(&patten,devifid , 0);
			if(ret ==FAIL)
				printk("Error can't find the entry \n");
			else
			{
				rtk_igmp_dump_devConfig(devifid,NULL,NULL);
			}
			return count;
		}

		if(rtk_igmp_whiteListAdd( &patten,devifid , &index)!=SUCCESS)
			printk("Error\n");
		else
			printk("whiteList Add Success at %d\n",index);


	}
	rtk_igmp_dump_devConfig(devifid,NULL,NULL);

DEV_ERROR:

	return count;

}



int rtk_igmp_ignoreGroup_proc_read(struct seq_file *s, void *v)
{
	rtk_igmp_groupToPsDump(s,v);
	PROC_PRINTF("=================================================\n");
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support ADD Patten:[GIP4/GIP6] [GIPMSK4/GIPMSK6(GIP option)] [GIP4_START/GIP6_START] [GIP4_END/GIP6_END]  \n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo  GIP4 224.2.3.4 > proc/igmp/ctrl/ignoreGroup\n");
	PROC_PRINTF("Support DEL Patten: [DELINDEX] [DELBY_PATTEN]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DELINDEX 0 > proc/igmp/ctrl/ignoreGroup\n");
	PROC_PRINTF("echo DELBY_PATTEN 1 GIP4 224.2.3.4 > proc/igmp/ctrl/ignoreGroup\n");
	return SUCCESS;
}

int rtk_igmp_ignoreGroup_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;
	rt_igmpHook_ignoreGroup_t patten;
	int index;
	uint32 v4ip=0;
	uint32 v6ip[4]={0};

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&index,"DELINDEX",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_groupToPsTblDel(NULL,index);
			return count;
		}

		memset(&patten,0,sizeof(patten));

		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4",V4IP) ==SUCCESS)
		{
			patten.ignGroupIpStart[0]=v4ip;
			patten.ignGroupIpEnd[0]=patten.ignGroupIpStart[0];
		}
		
		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4_START",V4IP) ==SUCCESS)
		{
			patten.ignGroupIpStart[0]=v4ip;
			patten.ignGroupIpEnd[0]=patten.ignGroupIpStart[0];
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4_END",V4IP) ==SUCCESS)
			{
				patten.ignGroupIpEnd[0]=v4ip;
			}

		}


		if(_rtk_igmp_parseGetValue(tmpBuf,v6ip,"GIP6",V6IP) ==SUCCESS)
		{
			memcpy(patten.ignGroupIpStart,v6ip,sizeof(patten.ignGroupIpStart));
			memcpy(patten.ignGroupIpEnd,v6ip,sizeof(patten.ignGroupIpEnd));
			patten.isIpv6=1;
		}
		if(_rtk_igmp_parseGetValue(tmpBuf,&v6ip,"GIP6_START",V6IP) ==SUCCESS)
		{
			memcpy(patten.ignGroupIpStart,v6ip,sizeof(patten.ignGroupIpStart));
			memcpy(patten.ignGroupIpEnd,v6ip,sizeof(patten.ignGroupIpEnd));
			patten.isIpv6=1;
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP6_END",V4IP) ==SUCCESS)
			{
				memcpy(patten.ignGroupIpEnd,v6ip,sizeof(patten.ignGroupIpEnd));
			}
		}


		if(_rtk_igmp_parseGetValue(tmpBuf,&index,"DELBY_PATTEN",UNSIGNED_LONG) ==SUCCESS)
		{
			int ret;
			ret=rtk_igmp_groupToPsTblDel(&patten,-1);
			if(ret ==FAIL)
				printk("Error can't find the entry \n");
			else
			{
				//rtk_igmp_dump_devConfig(devifid,NULL,NULL);
			}
			return count;
		}


		if(rtk_igmp_groupToPsTblAdd( &patten, &index)!=SUCCESS)
			printk("Error\n");
		else
			printk("groupToPs Add Success at %d\n",index);


	}
	//rtk_igmp_dump_devConfig(devifid,NULL,NULL);

	return count;

}



int rtk_igmp_groupWeight_proc_read(struct seq_file *s, void *v)
{
	rtk_igmp_groupWeightDump(s,v);
	PROC_PRINTF("=================================================\n");
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support ADD Patten:[GIP4/GIP6] [GIPMSK4/GIPMSK6(GIP option)] [GIP4_START/GIP6_START] [GIP4_END/GIP6_END][WEIGHT]  \n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo  DEVIFNAME eth0.2.0 GIP4 224.2.3.4 WEIGHT 10 > proc/igmp/ctrl/groupWeight\n");
	PROC_PRINTF("Support DEL Patten: [DELINDEX] [DELBY_PATTEN]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DELINDEX 0 > proc/igmp/ctrl/ignoreGroup\n");
	PROC_PRINTF("echo DELBY_PATTEN 1 GIP4 224.2.3.4 > proc/igmp/ctrl/groupWeight\n");
	return SUCCESS;
}

int rtk_igmp_groupWeight_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;
	rt_igmpHook_groupWeight_t patten;
	int index;
	uint32 v4ip=0;
	int32 weight=FAIL;
	uint32 v6ip[4]={0};
	rt_igmpHook_devInfo_t devInfo={{0}};
	int32 devifid=FAIL;
	char ifname[32]={0};

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&index,"DELINDEX",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_groupWeightTblDel(devInfo,NULL,index);
			return count;
		}

		memset(&patten,0,sizeof(patten));

		if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
		{
			if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
			{
				printk("DEVIFNAME parser error\n");
				return count;
			}
			else
			{
				memcpy(devInfo.devIfname,ifname,sizeof(devInfo.devIfname));
			}
		
		}
		else
			devInfo.devIfidx=devifid;



		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4",V4IP) ==SUCCESS)
		{
			patten.weightGroupIpStart[0]=v4ip;
			patten.weightGroupIpEnd[0]=patten.weightGroupIpStart[0];
		}
		
		if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4_START",V4IP) ==SUCCESS)
		{
			patten.weightGroupIpStart[0]=v4ip;
			patten.weightGroupIpEnd[0]=patten.weightGroupIpStart[0];
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP4_END",V4IP) ==SUCCESS)
			{
				patten.weightGroupIpEnd[0]=v4ip;
			}

		}


		if(_rtk_igmp_parseGetValue(tmpBuf,v6ip,"GIP6",V6IP) ==SUCCESS)
		{
			memcpy(patten.weightGroupIpStart,v6ip,sizeof(patten.weightGroupIpStart));
			memcpy(patten.weightGroupIpEnd,v6ip,sizeof(patten.weightGroupIpEnd));
			patten.isIpv6=1;
		}
		if(_rtk_igmp_parseGetValue(tmpBuf,&v6ip,"GIP6_START",V6IP) ==SUCCESS)
		{
			memcpy(patten.weightGroupIpStart,v6ip,sizeof(patten.weightGroupIpStart));
			memcpy(patten.weightGroupIpEnd,v6ip,sizeof(patten.weightGroupIpEnd));
			patten.isIpv6=1;
			if(_rtk_igmp_parseGetValue(tmpBuf,&v4ip,"GIP6_END",V4IP) ==SUCCESS)
			{
				memcpy(patten.weightGroupIpEnd,v6ip,sizeof(patten.weightGroupIpEnd));
			}
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&weight,"WEIGHT",SINGED_LONG) ==SUCCESS)
		{
			patten.weight=weight;
		}




		if(_rtk_igmp_parseGetValue(tmpBuf,&index,"DELBY_PATTEN",UNSIGNED_LONG) ==SUCCESS)
		{
			int ret;
			ret=rtk_igmp_groupWeightTblDel(devInfo,&patten,-1);
			if(ret ==FAIL)
				printk("Error can't find the entry \n");
			else
			{
				//rtk_igmp_dump_devConfig(devifid,NULL,NULL);
			}
			return count;
		}


		if(weight==FAIL || rtk_igmp_groupWeightTblAdd( devInfo,&patten, &index)!=SUCCESS)
			printk("Error\n");
		else
			printk("groupWeight Add Success at %d\n",index);


	}


	return count;

}





int rtk_igmp_br_proc_read(struct seq_file *s, void *v)
{

	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support Patten: [DEVIFID/DEVIFNAME] [BR_MAX_GROUP_SIZE] [BR_MAX_IP4_GROUP_SIZE] [BR_MAX_IP6_GROUP_SIZE] [BR_MAX_CLIENT_SIZE] [BR_MAX_IP4_CLIENT_SIZE]\n");
	PROC_PRINTF("                [BR_MAX_IP6_CLIENT_SIZE] [BR_PER_IP4_CLIENT_MAX_JOIN_GROUP] [BR_PER_IP6_CLIENT_MAX_JOIN_GROUP]\n");
	PROC_PRINTF("                [DROP_IGMPV1] [DROP_IGMPV2] [DROP_IGMPV3] [DROP_MLDV1] [DROP_MLDV2] [SNOOPING_DISABLE] [RATE_LIMIT] [FAST_LEAVE]\n");
	PROC_PRINTF("                [BR_FLUSH_CONF] [BR_FLUSH_MDB] [MAX_IP4_WEIGHT] [MAX_IP6_WEIGHT] [DROP_IP4_OVER_WEIGHT] [DROP_IP6_OVER_WEIGHT]\n");
	PROC_PRINTF("Example:\n");

	PROC_PRINTF("echo DEVIFNAME br0 BR_MAX_GROUP_SIZE 1 > proc/igmp/ctrl/brConfig\n");

	return SUCCESS;
}

int rtk_igmp_br_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;

	uint32 value;
	int32 devifid=FAIL;
	char ifname[32]={0};

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
		{
			if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
			{
				printk("DEVIFNAME parser error\n");
				goto DEV_ERROR;
			}
			else
			{
				devifid = rtk_igmp_devNameToBrDevIfidx(ifname);
			}
			if(devifid == FAIL)
			{
				printk("DEVIFNAME devifid FAIL\n");
				goto DEV_ERROR;
			}
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_MAX_GROUP_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_GROUP_SIZE,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_MAX_IP4_GROUP_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_IP4_GROUP_SIZE,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_MAX_IP6_GROUP_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_IP6_GROUP_SIZE,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_MAX_CLIENT_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_CLIENT_SIZE,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_MAX_IP4_CLIENT_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_IP4_CLIENT_SIZE,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_MAX_IP6_CLIENT_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_IP6_CLIENT_SIZE,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_PER_IP4_CLIENT_MAX_JOIN_GROUP",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_PER_IP4_CLIENT_MAX_JOIN_GROUP,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_PER_IP6_CLIENT_MAX_JOIN_GROUP",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_PER_IP6_CLIENT_MAX_JOIN_GROUP,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IGMPV1",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_IGMPV1,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IGMPV2",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_IGMPV2,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IGMPV3",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_IGMPV3,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_MLDV1",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_MLDV1,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_MLDV2",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_MLDV2,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"SNOOPING_DISABLE",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_SNOOPING_DISABLE,value);
		}
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_FLUSH_CONF",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_FLUSH_CONF,value);
		}
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"BR_FLUSH_MDB",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_FLUSH_MDB,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"RATE_LIMIT",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_RATE_LIMIT,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"FAST_LEAVE",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_FAST_LEAVE,value);
		}

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MAX_IP4_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_IP4_WEIGHT,value);
		}		

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MAX_IP6_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_MAX_IP6_WEIGHT,value);
		}	

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IP4_OVER_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_IP4_OVER_WEIGHT,value);
		}	
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IP6_OVER_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
		{
			rtk_igmp_BrDevConfig_set(devifid,RT_BR_DROP_IP6_OVER_WEIGHT,value);
		}	

		
	}
	rtk_igmp_dump_devConfig(devifid,NULL,NULL);
DEV_ERROR:

	return count;

}


int rtk_igmp6_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;

	uint32 value;
	int32 devifid=FAIL;
	char ifname[32]={0};

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
		{
			if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
			{
				printk("DEVIFNAME parser error\n");
				goto DEV_ERROR;
			}
			else
			{
				devifid = rtk_igmp_devNameToDevIfidx(ifname);
			}
			if(devifid == FAIL)
			{
				printk("DEVIFNAME devifid FAIL\n");
				goto DEV_ERROR;
			}
		}


		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"SNOOPING_DISABLE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_SNOOPING_DISABLE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_MLDV1",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_DROP_MLDV1 ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_MLDV2",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_DROP_MLDV2 ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"IGNORE_MLDV1",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_IGNORE_MLDV1 ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"IGNORE_MLDV2",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_IGNORE_MLDV2 ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"REPORT_DROP",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_REPORT_DROP ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"FAST_LEAVE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_FAST_LEAVE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"CARE_SOURCE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_CARE_SOURCE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MAX_GROUP_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_MAX_GROUP_SIZE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MC_MEMBER_AGING_INTERVAL",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_MC_MEMBER_AGING_INTERVAL ,value);
		
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MC_LAST_MEMBER_AGING_INTERVAL",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_MC_LAST_MEMBER_AGING_INTERVAL ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "RATE_LIMIT", UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1,RT_RATE_LIMIT, value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "FLUSH_CONF",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1, RT_FLUSH_CONF, value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "MAX_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1, RT_MAX_WEIGHT, value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "DROP_OVER_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 1, RT_DROP_OVER_WEIGHT, value);		

	}
	rtk_igmp_dump_devConfig(devifid,NULL,NULL);

DEV_ERROR:
	return count;
}

int rtk_igmp6_proc_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support Patten: [DEVIFID/DEVIFNAME] [SNOOPING_DISABLE] [DROP_MLDV1] [DROP_MLDV2] [IGNORE_MLDV1] [IGNORE_MLDV2]\n");
	PROC_PRINTF("                [REPORT_DROP] [FAST_LEAVE] [CARE_SOURCE] [DISABLE_CTAG_AUTO_LEARNING] [MAX_GROUP_SIZE] [MC_MEMBER_AGING_INTERVAL] [RATE_LIMIT] [FLUSH_CONF]\n");
	PROC_PRINTF("                [MAX_WEIGHT] [DROP_OVER_WEIGHT] [MC_LAST_MEMBER_AGING_INTERVAL]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DEVIFNAME eth0.2 SNOOPING_DISABLE 1 > proc/igmp/ctrl/igmp6Config\n");

	return SUCCESS;
}

int rtk_igmp_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;

	uint32 value;
	int32 devifid=FAIL;
	char ifname[32]={0};

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
		{
			if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
			{
				printk("DEVIFNAME parser error\n");
				goto DEV_ERROR;
			}
			else
			{
				devifid = rtk_igmp_devNameToDevIfidx(ifname);
			}
			if(devifid == FAIL)
			{
				printk("DEVIFNAME devifid FAIL\n");
				goto DEV_ERROR;
			}
		}


		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"SNOOPING_DISABLE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_SNOOPING_DISABLE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IGMPV1",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_DROP_IGMPV1 ,value);
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IGMPV2",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_DROP_IGMPV2 ,value);
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DROP_IGMPV3",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_DROP_IGMPV3 ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"IGNORE_IGMPV1",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_IGNORE_IGMPV1 ,value);
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"IGNORE_IGMPV2",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_IGNORE_IGMPV2 ,value);
		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"IGNORE_IGMPV3",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_IGNORE_IGMPV3 ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"REPORT_DROP",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_REPORT_DROP ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"FAST_LEAVE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_FAST_LEAVE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"CARE_SOURCE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_CARE_SOURCE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MAX_GROUP_SIZE",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_MAX_GROUP_SIZE ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MC_MEMBER_AGING_INTERVAL",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_MC_MEMBER_AGING_INTERVAL ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"MC_LAST_MEMBER_AGING_INTERVAL",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0,RT_MC_LAST_MEMBER_AGING_INTERVAL ,value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "RATE_LIMIT",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0, RT_RATE_LIMIT, value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "FLUSH_CONF",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0, RT_FLUSH_CONF, value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "MAX_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0, RT_MAX_WEIGHT, value);

		if(_rtk_igmp_parseGetValue(tmpBuf,&value, "DROP_OVER_WEIGHT",UNSIGNED_LONG) ==SUCCESS)
			rtk_igmp_devConfig_set(devifid, 0, RT_DROP_OVER_WEIGHT, value);		

	}
	rtk_igmp_dump_devConfig(devifid,NULL,NULL);

DEV_ERROR:
	return count;

}

int rtk_igmp_nextGroupStatisic_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support Patten: [DEVIFID/DEVIFNAME]  [ISIP6]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DEVIFNAME eth0.2 ISIP6 1 > proc/igmp/dump/nextGroupStatisic\n");

	return SUCCESS;

}


int rtk_igmp_nextGroupStatisic_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	struct seq_file *s=NULL;
	unsigned char tmpBuf[256] = {0};
	uint32 isipv6=0;
	int32 getIdx=0;
	int32 devifid=FAIL;
	char ifname[32]={0};
	rt_igmpHook_devInfo_t devInfo={{0}};
	rt_igmpHook_groupStatistic_t statisticInfo;
	int len = (count >= 255) ? 255 : count;

	if(!buffer)
		return count;
	/* copy data to the buffer */
	strncpy(tmpBuf+1, buffer, len);
	
	tmpBuf[len] = '\0';
	tmpBuf[0] = ' ';


	if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
	{
		if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
		{
			printk("DEVIFNAME parser error\n");
			return count;
		}
		else
		{
			memcpy(devInfo.devIfname,ifname,sizeof(devInfo.devIfname));
		}

	}
	else
		devInfo.devIfidx=devifid;


	_rtk_igmp_parseGetValue(tmpBuf,&isipv6,"ISIP6",SINGED_LONG);
		

	while(1)
	{
		int i=0;
		if(rtk_igmp_nextValidGroupStatistic_get(devInfo,&getIdx,isipv6,&statisticInfo)==SUCCESS &&  getIdx>=0)
		{
			if(isipv6)
			{
				PROC_PRINTF("[%d]Group=%pI6 SrcAddr=%pI6 vlan=%d curClient=%pI6(ExistTimesSec:%lu)\n",i,statisticInfo.groupAddr,statisticInfo.sourceAddr,statisticInfo.vlanId,statisticInfo.curClient,statisticInfo.curClientExistTimesSec);
			}
			else
			{
				PROC_PRINTF("[%d]Group=%pI4h SrcAddr=%pI4h vlan=%d curClient=%pI4h(ExistTimesSec:%lu)\n",i,statisticInfo.groupAddr,statisticInfo.sourceAddr,statisticInfo.vlanId,statisticInfo.curClient,statisticInfo.curClientExistTimesSec);

			}
			getIdx++;
			i++;
		}
		else
			break;
	}
	

	return count;

}

int rtk_igmp_devStatisic_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support Patten: [DEVIFID/DEVIFNAME][ISIP6] \n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DEVIFNAME eth0.2 ISIP6 1 > proc/igmp/dump/devStatistic\n");

	return SUCCESS;
}


int rtk_igmp_devStatisic_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	struct seq_file *s=NULL;
	unsigned char tmpBuf[256] = {0};
	uint32 isipv6=0;
	int32 devifid=FAIL;
	char ifname[32]={0};
	rt_igmpHook_devInfo_t devInfo={{0}};
	rt_igmpHook_devStatistic_t statisticInfo;
	int len = (count >= 255) ? 255 : count;

	if(!buffer)
		return count;
	/* copy data to the buffer */
	strncpy(tmpBuf+1, buffer, len);
	
	tmpBuf[len] = '\0';
	tmpBuf[0] = ' ';

	if(_rtk_igmp_parseGetValue(tmpBuf,&devifid,"DEVIFID",UNSIGNED_LONG) ==FAILED)
	{
		if(_rtk_igmp_parseGetValue(tmpBuf,ifname,"DEVIFNAME",STRING) ==FAILED)
		{
			printk("DEVIFNAME parser error\n");
			return count;
		}
		else
		{
			memcpy(devInfo.devIfname,ifname,sizeof(devInfo.devIfname));
		}

	}
	else
		devInfo.devIfidx=devifid;


	if(_rtk_igmp_parseGetValue(tmpBuf,&isipv6,"ISIP6",SINGED_LONG) !=SUCCESS)
		

	rtk_igmp_devStatistic_get(devInfo,isipv6,&statisticInfo);
	
	PROC_PRINTF("activeGroupCnt=%d bandwidthExceedCnt=%d curWeight=%d joinCnt=%d\n",statisticInfo.activeGroupCnt,statisticInfo.bandwidthExceedCnt,statisticInfo.curWeight,statisticInfo.joinCnt);


	return count;

}





int rtk_igmp_proc_read(struct seq_file *s, void *v)
{

	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("Support Patten: [DEVIFID/DEVIFNAME] [SNOOPING_DISABLE] [DROP_IGMPV1] [DROP_IGMPV2] [DROP_IGMPV3] [IGNORE_IGMPV1] [IGNORE_IGMPV2] [IGNORE_IGMPV3]\n");
	PROC_PRINTF("                [REPORT_DROP] [FAST_LEAVE] [CARE_SOURCE] [DISABLE_CTAG_AUTO_LEARNING] [MAX_GROUP_SIZE] [MC_MEMBER_AGING_INTERVAL] [RATE_LIMIT] [FLUSH_CONF]\n");
	PROC_PRINTF("                [MAX_WEIGHT] [DROP_OVER_WEIGHT] [MC_LAST_MEMBER_AGING_INTERVAL]\n");
	PROC_PRINTF("Example:\n");
	PROC_PRINTF("echo DEVIFNAME eth0.2 SNOOPING_DISABLE 1 > proc/igmp/ctrl/igmpConfig\n");


	return SUCCESS;
}

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
extern atomic_t wqAllocCnt;
extern atomic_t wqFreeCnt;
#endif

int rtk_igmp_debug_read(struct seq_file *s, void *v)
{
	PROC_PRINTF("DEBUG_LEVEL:%03x\n",igmpSysdb.igmp_debug_level);
	PROC_PRINTF("[WARNING]:          :0x001   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_WARNING?"ON":"");
	PROC_PRINTF("[HOOK_POINT]:       :0x002   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_HOOK_POINT?"ON":"");
	PROC_PRINTF("[PARSER]:           :0x004   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_PARSER?"ON":"");	
	PROC_PRINTF("[IGMPCTRL]:         :0x008   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_IGMPCTRL?"ON":"");
	PROC_PRINTF("[FLOWDATA]:         :0x010   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_FLOWDATA?"ON":"");
	PROC_PRINTF("[HWCB]:             :0x020   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_HWCB?"ON":"");
	PROC_PRINTF("[DUMP]:             :0x040   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_DUMP?"ON":"");
	PROC_PRINTF("[RATE_LIMIT_DROP]:  :0x080   %s \n",igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_RATE_LIMIT_DROP?"ON":"");
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	PROC_PRINTF("wqAllocCnt =%d  wqFreeCnt=%d \n",atomic_read(&wqAllocCnt),atomic_read(&wqFreeCnt));
#endif
	return SUCCESS;
}

int rtk_igmp_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{

	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf, buffer, len);
		igmpSysdb.igmp_debug_level=simple_strtoul(tmpBuf, NULL, 16);
		rtk_igmp_debug_read(NULL,NULL);
		return count;
	}
	return -EFAULT;

}


int rtk_igmp_mrouteMapping_read(struct seq_file *s, void *v)
{

	PROC_PRINTF("Usage:\n");
	PROC_PRINTF("echo UPIFIDX [UP_IF_IDX] DOWNIFIDX [DOWN_IF_IDX] > proc/igmp/ctrl/mrouteMapping\n");
	PROC_PRINTF("echo UPIFNAME [UP_IF_NAME] DOWNIFNAME [DOWN_IF_NAME] > proc/igmp/ctrl/mrouteMapping\n");

	PROC_PRINTF("\n\n");
	rtk_igmp_allDump(s,v);

	return SUCCESS;
}

int rtk_igmp_mrouteMapping_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned char tmpBuf[256] = {0};
	int len = (count >= 255) ? 255 : count;
	int32 upifidx=FAIL,downifidx=FAIL;
	uint32 value;
	char upIfName[16]={0};
	char downIfName[16]={0};
	int ifNameGet=FAIL;

	if (buffer)
	{
		/* copy data to the buffer */
		strncpy(tmpBuf+1, buffer, len);

		tmpBuf[len] = '\0';
		tmpBuf[0] = ' ';

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"DOWNIFIDX",UNSIGNED_LONG) ==SUCCESS)
			upifidx = value;
		else
			upifidx=FAIL;

		if(_rtk_igmp_parseGetValue(tmpBuf,&value,"UPIFIDX",UNSIGNED_LONG) ==SUCCESS)
			downifidx = value;
		else
			downifidx = FAIL;

		if((_rtk_igmp_parseGetValue(tmpBuf,upIfName,"UPIFNAME",STRING) ==SUCCESS) && (_rtk_igmp_parseGetValue(tmpBuf,downIfName,"DOWNIFNAME",STRING) ==SUCCESS))
			ifNameGet=SUCCESS;
	}

	if(upifidx!=FAIL && downifidx!=FAIL)
	{
		rtk_igmp_wanIdxBindToBrIdx(upifidx,downifidx);
		rtk_igmp_allDump(NULL,NULL);
	}
	else if(ifNameGet==SUCCESS)
	{
		rtk_igmp_wanIfNameBindToBrName(upIfName,downIfName);
		rtk_igmp_allDump(NULL,NULL);
	}
	else
		printk("parameter error\n");

	return count;
}


int rtk_igmp_proc_init(void)
{
	int i = 0;
	struct proc_dir_entry *p=NULL;

	/* create proc directories  */
	if(rtk_igmp_proc_dir==NULL){
		rtk_igmp_proc_dir = proc_mkdir("igmp", NULL);
	}
	if(rtk_igmp_dump_proc_dir==NULL){
		rtk_igmp_dump_proc_dir = proc_mkdir("dump", rtk_igmp_proc_dir);
	}
	if(rtk_igmp_ctrl_proc_dir==NULL){
		rtk_igmp_ctrl_proc_dir = proc_mkdir("ctrl", rtk_igmp_proc_dir);
	}

	for( i=0; i< (sizeof(igmpProc)/sizeof(rtk_igmp_proc_t)) ;i++)
	{
		struct proc_dir_entry *tmpDir=NULL;

		if(igmpProc[i].get==NULL)
			igmpProc[i].proc_fops.open=rtk_igmp_nullDebugSingleOpen;
		else
			igmpProc[i].proc_fops.open=rtk_igmp_commonDebugSingleOpen;

		if(igmpProc[i].set==NULL)
			igmpProc[i].proc_fops.write=NULL;
		else
			igmpProc[i].proc_fops.write=rtk_igmp_commonDebugSingleWrite;

		igmpProc[i].proc_fops.read=seq_read;
		igmpProc[i].proc_fops.llseek=seq_lseek;
		igmpProc[i].proc_fops.release=single_release;

		switch(igmpProc[i].dir)
		{
			case PROC_DIR_IGMP_DUMP:
				tmpDir=rtk_igmp_dump_proc_dir;
				break;
			case PROC_DIR_IGMP_CTRL:
				tmpDir=rtk_igmp_ctrl_proc_dir;
				break;
			default:
				break;
		}


		p = proc_create_data(igmpProc[i].name, S_IRUGO, tmpDir, &(igmpProc[i].proc_fops),NULL);
		if(!p){
			printk("create proc %s failed!\n",igmpProc[i].name);
		}
		igmpProc[i].inode_id = p->low_ino;
	}

	return SUCCESS;

}



void rtk_igmp_proc_exit(void)
{
	int i = 0;

	for( i=0; i< (sizeof(igmpProc)/sizeof(rtk_igmp_proc_t)) ;i++)
	{
		struct proc_dir_entry *tmpDir=NULL;

		switch(igmpProc[i].dir)
		{
			case PROC_DIR_IGMP_DUMP:
				tmpDir=rtk_igmp_dump_proc_dir;
				break;
			case PROC_DIR_IGMP_CTRL:
				tmpDir=rtk_igmp_ctrl_proc_dir;
				break;
			default:
				break;
		}

		remove_proc_entry(igmpProc[i].name, tmpDir);
	}

	proc_remove(rtk_igmp_dump_proc_dir);
	proc_remove(rtk_igmp_ctrl_proc_dir);
	proc_remove(rtk_igmp_proc_dir);
}


