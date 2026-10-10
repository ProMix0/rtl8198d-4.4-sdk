#include <linux/ptool.h>
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#define uint32 unsigned int

#if defined(CONFIG_RTL9607C_SERIES)
#define uint32 unsigned int	
#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
#define SIGNAL_NAME_MAX_LEN 20
int la_gpio_signal_cpu_mask[LA_GPIO_MAX]={0};
int la_gpio_signal_gpio_id[LA_GPIO_MAX]={0};
char la_gpio_signal_name[LA_GPIO_MAX][SIGNAL_NAME_MAX_LEN]={0};

#define REG32(reg) 	(*((volatile unsigned int *)(reg)))
#define SET_GPIO_NAME(gpio_name)  do { set_gpio_name(gpio_name,#gpio_name); } while(0)


void set_gpio_name(int idx, char *gpio_name)
{
	memcpy(&la_gpio_signal_name[idx],gpio_name,SIGNAL_NAME_MAX_LEN);
}

int la_gpio_init(void)
{
	int i;
	for(i=0;i<LA_GPIO_MAX;i++)
	{
		la_gpio_signal_cpu_mask[i]=0xf;
		la_gpio_signal_gpio_id[i]=0;		
	}

	SET_GPIO_NAME(LA_GPIO_SOFTIRQ);
	SET_GPIO_NAME(LA_GPIO_HW_ISR);
	SET_GPIO_NAME(LA_GPIO_SPINLOCK);
	SET_GPIO_NAME(LA_GPIO_NIC_RX);
	SET_GPIO_NAME(LA_GPIO_NIC_TX);
	SET_GPIO_NAME(LA_GPIO_WIFI_RX_DSR);
	SET_GPIO_NAME(LA_GPIO_WIFI_TX);
	SET_GPIO_NAME(LA_GPIO_AUX0);
	SET_GPIO_NAME(LA_GPIO_AUX1);
	SET_GPIO_NAME(LA_GPIO_AUX2);
	SET_GPIO_NAME(LA_GPIO_AUX3);
	SET_GPIO_NAME(LA_GPIO_AUX4);
	SET_GPIO_NAME(LA_GPIO_AUX5);
	SET_GPIO_NAME(LA_GPIO_AUX6);
	SET_GPIO_NAME(LA_GPIO_AUX7);	

	return 0;
}

void la_gpio_high(int cpu_mask, int gpio_id)
{
	int cpu_id=smp_processor_id();
	if(cpu_mask&(1<<cpu_id))
	{		
		if(gpio_id<32)
		{
			REG32(0xb800330c)|=(1<<gpio_id);
		}
		else
		{
			REG32(0xb8003328)|=(1<<(gpio_id-32));
		}
	}
}

void la_gpio_low(int cpu_mask, int gpio_id)
{
	int cpu_id=smp_processor_id();
	if(cpu_mask&(1<<cpu_id))
	{		
		if(gpio_id<32)
		{
			REG32(0xb800330c)&=~(1<<gpio_id);
		}
		else
		{
			REG32(0xb8003328)&=~(1<<(gpio_id-32));
		}
	}
}
#endif

#ifdef CONFIG_RTK_PTOOL_CPU_PERF

uint32 perf_count=0;

char funcs[MAX_HASH_IDX][FUNC_LEN]={{0}};
int lines[MAX_HASH_IDX]={0};
int lines_exit[MAX_HASH_IDX]={0};


unsigned long long cp3_count0[MAX_HASH_IDX]={0};
unsigned long long cp3_count1[MAX_HASH_IDX]={0};

uint32 cp3_cnt0_temp[MAX_HASH_IDX]={0};
uint32 cp3_cnt1_temp[MAX_HASH_IDX]={0};
uint32 perf_cp0ctrl=0;


int profile_stopped=1;

int profile_stop(void)
{
	perf_cp0ctrl=0;

	__asm__ __volatile__ \
	("	;\
		la		$8, perf_cp0ctrl ;\
		lw		$8, 0($8)		;\
		mtc0	$8, $25,0;\
	");

	profile_stopped=1;	
	return 0;
}
	

int profile_start(void)
{
	uint32 perf_tcid=0;
	uint32 perf_mt_en=1;
	uint32 perf_vpeid=0;
	uint32 perf_pctd=0;
	uint32 perf_event=0;
	uint32 perf_ie=0,perf_u=0,perf_s=0,perf_k=1,perf_exl=0;
	
	memset(funcs,0,MAX_HASH_IDX*FUNC_LEN);
	memset(lines,0,sizeof(int)*MAX_HASH_IDX);
	memset(lines_exit,0,sizeof(int)*MAX_HASH_IDX);
	memset(cp3_count0,0,sizeof(unsigned long long)*MAX_HASH_IDX);
	memset(cp3_count1,0,sizeof(unsigned long long)*MAX_HASH_IDX);


	//write cp0ctrl command
	perf_cp0ctrl=(perf_tcid<<22)|(perf_mt_en<<20)|(perf_vpeid<<16)|(perf_pctd<<15)|(perf_event<<5)|(perf_ie<<4)|(perf_u<<3)|(perf_s<<2)|(perf_k<<1)|perf_exl;
	
	printk("CP0 $25(0)=0x%x",perf_cp0ctrl);
	
	__asm__ __volatile__ \
	("	;\
		la		$8, perf_cp0ctrl ;\
		lw		$8, 0($8)		;\
		mtc0	$8, $25,0;\
	");

	//reset counter
	__asm__ __volatile__ \
	("	;\
		mtc0	$0, $25,1;\
	");

	profile_stopped=0;

	return 0;

}


inline int inline_strlen(char *buf)
{
	int i=0;
	while(1)
	{
		if(buf[i]==0) return i;
		i++;
	}
}

inline void inline_memcpy(u8 *dest, u8 *src, int len)
{
	int i;
	for(i=0;i<len;i++)
		dest[i]=src[i];
}


inline int inline_memcmp(u8 *str1, u8 *str2, int len)
{
	int i;
	int not_eq=0;
	for(i=0;i<len;i++)
		if(str1[i]!=str2[i])
		{
			if(str1[i]>str2[i]) not_eq=1;
			else	not_eq=-1;
			break;
		}
	return not_eq;
}

int profile(const char *func)
{
	int idx, func_len, j;
	
	if(likely(profile_stopped==1)) return MAX_HASH_IDX-1;
	
	func_len=inline_strlen((char *)func);
	idx=((
			 ((unsigned int)(func[func_len-1]-'_'+1)|
				 (unsigned int)((func[func_len-2]-'_'+1)<<8)|
				 (unsigned int)((func[func_len-3]-'_'+1)<<16) )
					 *
					 0x9e370001
					 *
			  ((unsigned int)(func[0]-'_'+1)|
				 (unsigned int)((func[1]-'_'+1)<<8)|
				 (unsigned int)((func[2]-'_'+1)<<16) )					 
				  )>>(32-MAX_HASH_IDX_BIT+MAX_HASH_WAY_BIT))<<MAX_HASH_WAY_BIT;

	for(j=0;j<(1<<MAX_HASH_WAY_BIT);j++)
	{
		if(lines[idx+j]==0)
		{
			++lines[idx+j];
			inline_memcpy(funcs[idx+j],(char *)func,FUNC_LEN-1);
			goto out;
		}
		else
		{			
			if(inline_memcmp((u8*)func,funcs[idx+j],(func_len<(FUNC_LEN-1))?func_len:(FUNC_LEN-1))==0)
			{
				++lines[idx+j];
				goto out;
			}			
		}
	}

	//full
	return MAX_HASH_IDX-1;

out:
	idx+=j;

	__asm__ __volatile__ ("mfc0 %0, $25,1" : "=r" (perf_count));



//	printk("idx2=%d\n",idx);

	cp3_cnt0_temp[idx]=perf_count;

//	printk("idx3=%d\n",idx);	
	
	return idx;
}
	

void profile_end(int idx)
{
	if(likely(profile_stopped==1)) return;	


	// fetch counter
	__asm__ __volatile__ ("mfc0 %0, $25,1" : "=r" (perf_count));

	++lines_exit[idx];

	if(likely(cp3_cnt0_temp[idx]<=perf_count))
		cp3_count0[idx]+=(unsigned long long)(perf_count-cp3_cnt0_temp[idx]);
	else
		cp3_count0[idx]+=(unsigned long long)(perf_count+0x100000000LL-cp3_cnt0_temp[idx]);

}


int profile_dump(void)
{
	int i,j=0;	
	for(i=0;i<MAX_HASH_IDX;i++)
	{
		if(lines[i]!=0)
		{
			unsigned long long tmp=cp3_count0[i];
			++j;
			
			do_div(tmp,lines_exit[i]);
			printk("[%5d],%10d",i,lines[i]);
			printk(",%10d",lines_exit[i]);
			printk(",%15llu",cp3_count0[i]);
			printk(",%10llu",tmp);
			printk(", %s",&funcs[i][0]);
			printk("\n");
		}
	}
	printk("Total %d functions\n",j);
	return 0;

}

#endif


// Proc files

#define MAX_PROC_SIZE 40
char proc_data[MAX_PROC_SIZE];

struct proc_dir_entry *ptool_dir=NULL;
struct proc_dir_entry *proc_entry=NULL;

#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
int ptool_proc_la_gpio_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	int i;
	printk("%5s %20s %8s %5s\n","idx","name","cpu_mask","gpio");
	for(i=0;i<LA_GPIO_MAX;i++)
	{
		printk("%5d %20s %8x %5d\n",i,la_gpio_signal_name[i],la_gpio_signal_cpu_mask[i],la_gpio_signal_gpio_id[i]);	
	}
	printk("\nUser guide:\n");
	printk("use \"echo $idx $cpu_mask $gpio  >/proc/ptool/la_gpio_set\" to config each gpio signal.\n");
	return 0;
}


int ptool_proc_la_gpio_set(struct file *file,const char *buf,int count,void *data )
{
	int idx,cpumask,gpio_pin;
	
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	sscanf(proc_data,"%d %x %d",&idx,&cpumask,&gpio_pin);	
	if(idx<LA_GPIO_MAX)
	{
		printk("idx=%d cpu_mask=0x%x gpio_pin=%d\n",idx,cpumask,gpio_pin);
		la_gpio_signal_cpu_mask[idx]=cpumask;
		la_gpio_signal_gpio_id[idx]=gpio_pin;

		if(la_gpio_signal_gpio_id[idx]!=0)
		{
			if(la_gpio_signal_gpio_id[idx]<32)
			{
				int bitValue=(1<<(la_gpio_signal_gpio_id[idx]));
				REG32(0xbb000038)|=(bitValue);
				REG32(0xb8003308)|=(bitValue);
				REG32(0xb800330c)&=~(bitValue);
			}
			else
			{
				int bitValue=(1<<(la_gpio_signal_gpio_id[idx]-32));
				REG32(0xbb00003c)|=(bitValue);
				REG32(0xb8003324)|=(bitValue);
				REG32(0xb8003328)&=~(bitValue);
			}
		}		
	}
	
	return count;
}


#endif


#ifdef CONFIG_RTK_PTOOL_CPU_PERF

int ptool_proc_profile_enable_status(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	if(profile_stopped==0)
	{		
		printk("CPU Profile Enabled\n");
	}
	else
	{		
		printk("CPU Profile Disabled\n");
	}
	return 0;
}


int ptool_proc_profile_enable(struct file *file,const char *buf,int count,void *data )
{
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	if(proc_data[0]=='1')
	{
		profile_start();		
	}
	else 
	{
		profile_stop();
	}

	return count;
}

int ptool_proc_profile_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	profile_dump();
	return 0;
}

#endif

struct file_operations proc_file_fops[] = {
#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
	{
		read: ptool_proc_la_gpio_dump,
		write: NULL
	},
	{
		read: ptool_proc_la_gpio_dump,
		write: ptool_proc_la_gpio_set
	},
#endif
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
	{
		read: ptool_proc_profile_enable_status,
		write: ptool_proc_profile_enable
	},
	{
		read: ptool_proc_profile_dump,
		write: NULL
	},
#endif	
};


char proc_name[][40]={
#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
	"la_gpio_dump",
	"la_gpio_set",
#endif
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
	"profile_enable",
	"profile_dump",
#endif
};


__init int ptool_init(void)
{
	int i;

#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
	la_gpio_init();
#endif


	if(ptool_dir==NULL){
			ptool_dir = proc_mkdir("ptool", NULL);
	}

	for(i=0;i<sizeof(proc_file_fops)/sizeof(struct file_operations);i++)
	{
		proc_entry = proc_create(proc_name[i], 0, ptool_dir, &proc_file_fops[i]);
		if(!proc_entry){
			printk("create ptool proc[%d] failed!\n",i);
			return -ENOMEM;
		}
	}		

	return 0;
}

module_init(ptool_init);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Realtek Semiconductor Corp.");
MODULE_DESCRIPTION("Realtek performance monitor tool");

#endif

