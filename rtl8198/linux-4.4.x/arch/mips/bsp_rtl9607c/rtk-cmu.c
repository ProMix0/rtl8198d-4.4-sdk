#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>
#include <bspchip.h>
#include "rtk-cmu.h"
#ifndef REG32
#define REG32(x) *(unsigned int*)(x)
#endif
#define RTK_CMU_DEBUG
#ifdef RTK_CMU_DEBUG
#define rtk_cmu_printk(fmt, args...) printk("[%s]" fmt, __func__, ## args)
#else
#define rtk_cmu_printk(fmt, args...)
#endif
static unsigned int cmu_divisor=2;
static unsigned int cmu_delay_base=4;
static unsigned int cmu_delay_mul=5;
static unsigned int cmu_wis0rv = 0xE6800F;
static unsigned int cmu_wis1rv = 0xE2050;
atomic_t cmu_setup_done = ATOMIC_INIT(0);

static void (*saved_cpu_wait) (void);
extern void (*cpu_wait)(void);

#define BSP_REGPLLL1          0xB800020C
#define BSP_SYSPLLCTR         0xB8000200
static unsigned int cpu_freq(void) {
	//check reg_en_DIV2_cpu0 for different ocp0 freq. calculation
	if (REG32(BSP_REGPLLL1) & (1<<18)) {
		return ((((REG32(BSP_SYSPLLCTR)>>16)&0x3f)+2)*25000000);
	} else {
		return ((((REG32(BSP_SYSPLLCTR)>>16)&0x3f)+2)*50000000);
	}
}
static unsigned int dram_freq(void) {
	unsigned char pdiv;

	//pdiv 0: /1, 1: /2, 2: /4, 3: /8
	pdiv=1<<((REG32(0xb800023c)>>14)&0x3);
	//(n+3)*25
	return ((((((REG32(0xb8000240)>>24)&0x0ff) + 3) * 25000000)>>1)/pdiv);
}
static void rtk_cmu_wakeup_intr_select(unsigned int wis0, unsigned int wis1) {
	REG32(OC0_CMUWIS0R) = wis0;
	REG32(OC0_CMUWIS1R) = wis1;
}
static void rtk_cmu_disable(void) {
	OC0_CMUGCR_T gcr;
	gcr.f.busy     = 1;
    gcr.f.freq_div = cmu_divisor;
    gcr.f.cmu_mode = DISABLE_CMU;
    OC0_CMUGCRrv = gcr.v;
}
static unsigned int cmu_cpu_mask = 0;
static void rtk_cmu_enable(void) {
	OC0_CMUGCR_T gcr;

	if (!((1 << smp_processor_id()) & cmu_cpu_mask)) {
		//printk("cpu %d is not in the mask\n", smp_processor_id());
		return;
	}
#if 0
	else {
		printk("cpu %d try to do cmu\n", smp_processor_id());
	}
#endif
	if (!atomic_read(&cmu_setup_done))
		return;

	gcr.v = OC0_CMUGCRrv;
	if(!gcr.f.busy)
		return;

    gcr.f.busy     = 0;
    gcr.f.cmu_mode = ENABLE_CMU_DYNAMIC_MODE;
    OC0_CMUGCRrv = gcr.v;
}
static void rtk_cmu_setup(void) {
    unsigned int divisor=cmu_divisor;
    unsigned int delay_base=cmu_delay_base;
    unsigned int delay_mul=cmu_delay_mul;
    unsigned int sleep_clk;
    OC0_CMUCR_T cr;//, fake_cr;
//    OC0_CMUGCR_T gcr;
	//setup Wake-up Interrupt Selection Register
	rtk_cmu_wakeup_intr_select(cmu_wis0rv, cmu_wis1rv);
	//rtk_cmu_printk("setup Wake-up Interrupt Selection Register\n");
	//setup OC0_CMUCR
    cr.v = OC0_CMUCRrv;
    //Set auto_bz / interrupt type / delay_mul
    cr.f.auto_bz = 1;
    cr.f.int_cxn_type = 1;
    cr.f.dly_mul = delay_mul;
	//rtk_cmu_printk("fill auto_bz / interrupt type / delay_mul\n");
    //Calculate slow bit for wk & slp (SRAM)
    cr.f.se_sram_rom_wk = cr.f.se_sram_rom;
    cr.f.se_sram_rom_slp= 1;
    //cr.f.se_sram_rom_hs = 1;
	cr.f.se_sram_rom_hs = 0;
	//rtk_cmu_printk("fill SRAM slowbit\n");

    //Calculate slow bit for wk & slp (DRAM)
    sleep_clk = cpu_freq()/(1<<divisor);
    cr.f.se_dram_wk = cr.f.se_dram;
    cr.f.se_dram_slp= (sleep_clk < dram_freq()*2)?1:0;
    //cr.f.se_dram_hs = 1;
	cr.f.se_dram_hs = 0;
	//rtk_cmu_printk("fill DRAM slowbit\n");
#if 0
    /*Due to the CMU bug, when using handshake mode, the [wk_slow_bit] should be set to the error value first*/
    fake_cr.v = cr.v;
    fake_cr.f.se_sram_rom_wk = ~cr.f.se_sram_rom_wk;
    fake_cr.f.se_sram_rom_hs = 0;
    fake_cr.f.se_dram_wk = ~cr.f.se_dram_wk;
    fake_cr.f.se_dram_hs = 0;
    //rtk_cmu_printk("fill fake cr\n");
#endif
    //Change to Disable mode
	rtk_cmu_disable();
	rtk_cmu_printk("disable cmu\n");
    //Set Sleep Delay Control Register
    OC0_CMUSDCRrv = delay_base;
	rtk_cmu_printk("set OC0_CMUSDCRrv\n");
    //OC0_CMUCRrv = fake_cr.v;
	//rtk_cmu_printk("set OC0_CMUCRrv to fake cr\n");
    OC0_CMUCRrv = cr.v;
	rtk_cmu_printk("set OC0_CMUCRrv to cr\n");
	//atomic_set(&cmu_setup_done, 1);
	//cmu_cpu_mask = 1;

    rtk_cmu_printk("CMU state: GCR=0x%08x,  SDCR=0x%08x, CR=0x%08x, SCR=0x%08x\n", OC0_CMUGCRrv, OC0_CMUSDCRrv, OC0_CMUCRrv, OC0_CMUSCRrv);
}
void arch_wait(void) {
	//rtk_cmu_printk("enable cmu\n");
	if (!atomic_read(&cmu_setup_done))
		return;

	rtk_cmu_enable();
}

static void rtk_cpu_wait(void)
{
	arch_wait();
	if(saved_cpu_wait)
		saved_cpu_wait();
}

static void rtk_cpu_wait_setup(void)
{
	saved_cpu_wait = cpu_wait;
	cpu_wait = rtk_cpu_wait;
}

static void show_usage(void){
	printk("	div [value] : set cmu freq divisor to [value]\n");
	printk("	db [value] : set cmu delay base to [value]\n");
	printk("	dm [value] : set cmu delay mul to [value]\n");
	printk("	wis [value0] [value1] : set cmu wis0r to [value0], wis1r to [vaule1], value0/1 is hex, prefix 0x is not needed\n");
	printk("	s 1 : start setup cmu, div/db/dm/w should be set before, if you want to overwrite the default vaules\n");
	printk("	m [value] : set cmu cpu mask, enable cpu(s) in the mask to enable cmu, value is hex, prefix 0x is not needed\n");
	printk("    auto [0/1] : auto start CMU, depends on idle threshold\n");
	printk("    interval [>100(ms)] : timer interval in milliseconds to handle auto CMU. \n");
	printk("    threshold [1-99] : idle threshold in percentage. \n");
}
static u64 cmu_last_idle;
static u64 cmu_last_total;
static u32 cmu_tick;
static u32 cmu_interval = 1000; //1000ms by default
static u32 cmu_threshold = 90;
static struct timer_list cmu_timer;
static int rtk_cmu_write_proc(struct file * file, const char __user * userbuf, size_t count, loff_t * off) {
	char buf[32];
	int len;
	int val;
	unsigned int mask;
	unsigned int w0rv, w1rv;

	len = min(sizeof(buf), count);
	if (copy_from_user(buf, userbuf, len))
		return -E2BIG;

	if (strncmp(buf, "help", 4) == 0) {
		show_usage();
	} else if (strncmp(buf, "div ", 4) == 0) {
		if(1==sscanf(buf, "div %d", &val)){
			cmu_divisor = val;

			printk("set cmu freq divisor to %d\n", cmu_divisor);
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "db ", 3) == 0) {
		if(1==sscanf(buf, "db %d", &val)){
			cmu_delay_base = val;

			printk("set cmu delay base to %d\n", cmu_delay_base);
		} else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "dm ", 3) == 0) {
		if(1==sscanf(buf, "dm %d", &val)){
			cmu_delay_mul = val;

			printk("set cmu delay mul to %d\n", cmu_delay_mul);
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "e ", 2) == 0) {
		if(1==sscanf(buf, "e %d", &val)){
			if (val)
				rtk_cmu_enable();
			else
				rtk_cmu_disable();

			printk("%s cmu\n", val?"enable":"disable");
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "s ", 2) == 0) {
		if(1==sscanf(buf, "s %d", &val)){
			switch (val) {
				case 1:
				rtk_cmu_setup();
				printk("setup cmu\n");
					break;

				case 2:
					cmu_cpu_mask = 0xf;
					atomic_set(&cmu_setup_done, 1);
					printk("start cmu\n");
					break;

				case 3:
					cmu_cpu_mask = 0x0;
					atomic_set(&cmu_setup_done, 0);
					printk("stop cmu\n");
					break;

				default:
					goto ERROR_PARA;
			}
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "m ", 2) == 0) {
		if(1==sscanf(buf, "m %x", &mask)){
			cmu_cpu_mask = mask;
			printk("set cmu cpu mask to 0x%x\n", cmu_cpu_mask);
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "wis ", 4) == 0) {
		if(2==sscanf(buf, "w %x %x", &w0rv, &w1rv)){
			cmu_wis0rv = w0rv;
			cmu_wis1rv = w1rv;
			printk("set cmu wis0r to 0x%x, wis1r to 0x%x\n", cmu_wis0rv, cmu_wis1rv);
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "auto ", 5) == 0) {
		if (1==sscanf(buf, "auto %d", &val)) {
			if (val && !timer_pending(&cmu_timer)) {
				//setup CMU
				rtk_cmu_setup();
				cmu_tick = 0;
				cmu_last_idle = cmu_last_total = 0;
				mod_timer(&cmu_timer, jiffies+msecs_to_jiffies(cmu_interval));
				printk("\nTimer start\n");
			} else if (!val) {
				del_timer(&cmu_timer);
				printk("\nTimer stopped\n");
				//stop cmu
				cmu_cpu_mask = 0x0;
				atomic_set(&cmu_setup_done, 0);
			}
		}
	} else if (strncmp(buf, "threshold ", 10) == 0){
		if(1==sscanf(buf, "threshold %d", &val)){
			cmu_threshold = val;
		}
	} else if (strncmp(buf, "interval ", 9) == 0){
		if(1==sscanf(buf, "interval %d", &val)){
			if(val<100){
				goto ERROR_PARA;
			} else {
				cmu_interval = val;
			}
		}
	} else {
		goto ERROR_PARA;
	}
	return count;

ERROR_PARA:
	printk("error parameter...\n");
	show_usage();
	return -EPERM;
}

static int rtk_cmu_read_proc(struct seq_file *f, void *data)
{
	  seq_printf(f, "cpu freq: %dMHz, dram freq: %dMHz\n",  cpu_freq()/1000000, dram_freq()/1000000);
	  seq_printf(f, "cmu freq divisor: %d\n", cmu_divisor);
	  seq_printf(f, "cmu delay base: %d\n", cmu_delay_base);
	  seq_printf(f, "cmu delay mul: %d\n", cmu_delay_mul);
	  seq_printf(f, "cmu cpu mask: 0x%x\n\n", cmu_cpu_mask);
	  seq_printf(f, "CMU state: GCR=0x%08x,  SDCR=0x%08x, CR=0x%08x, SCR=0x%08x\n", OC0_CMUGCRrv, OC0_CMUSDCRrv, OC0_CMUCRrv, OC0_CMUSCRrv);
	  seq_printf(f, "CMU wake: WIS0R=0x%08x,  WIS1R=0x%08x\n\n", REG32(OC0_CMUWIS0R), REG32(OC0_CMUWIS1R));
      return 0;
}
static int read_proc_open_rtk_cmu(struct inode *inode, struct file *file) {
    return(single_open(file, rtk_cmu_read_proc, NULL));
}
static ssize_t write_proc_rtk_cmu(struct file *file, const char __user * userbuf, size_t count, loff_t * off) {
    return rtk_cmu_write_proc(file, userbuf, count, NULL);
}
static struct file_operations fops_proc_rtk_cmu = {
    .open     = read_proc_open_rtk_cmu,
    .read     = seq_read,
    .llseek   = seq_lseek,
    .release  = single_release,
    .write    = write_proc_rtk_cmu,
};
static struct proc_dir_entry *rtk_cmu_proc=NULL;
static u64 get_idle_time(int cpu)
{
	u64 idle, idle_time = -1ULL;
	if (cpu_online(cpu))
		idle_time = get_cpu_idle_time_us(cpu, NULL);
	if (idle_time == -1ULL)
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
		idle = usecs_to_cputime64(idle_time);
	return idle;
}
static void cmu_periodic_fn(unsigned long arg) {
	int i, cpu;
	u64 current_idle = 0, current_total = 0;
	mod_timer(&cmu_timer, jiffies+msecs_to_jiffies(cmu_interval));
	for_each_possible_cpu(cpu) {
		for (i = 0; i < NR_STATS; i++) {
			current_total += kcpustat_cpu(cpu).cpustat[i];
		}
		current_idle += get_idle_time(cpu);
	}
	if (cmu_last_total) {
		u64 diff_total, diff_idle;
		u32 idle;
		diff_total = current_total - cmu_last_total;
		diff_idle = current_idle - cmu_last_idle;
		idle =  div64_u64(diff_idle * 100, diff_total);
		if(idle > cmu_threshold){
			cmu_cpu_mask = 0xf;
			atomic_set(&cmu_setup_done, 1);
		}else{
			cmu_cpu_mask = 0x0;
			atomic_set(&cmu_setup_done, 0);
		}
	}
	cmu_last_total = current_total;
	cmu_last_idle = current_idle;
}
static int rtk_cmu_proc_init(void) {
	rtk_cmu_proc = proc_create_data("cmu", 0644, NULL, &fops_proc_rtk_cmu, NULL);
	if (!rtk_cmu_proc) {
		printk("Realtek CMU, create proc failed!\n");
	}
	return 0;
}
static int rtk_cmu_proc_clean(void) {
	remove_proc_entry("cmu", NULL);
	return 0;
}
static int __init rtk_cmu_init(void) {
	//rtk_cmu_setup();
	rtk_cmu_proc_init();
	rtk_cpu_wait_setup();
	setup_timer(&cmu_timer, cmu_periodic_fn, 0);
	return 0;
}
void __exit rtk_cmu_exit(void) {
	rtk_cmu_proc_clean();
	//return 0;
}
module_init(rtk_cmu_init);
module_exit(rtk_cmu_exit);

