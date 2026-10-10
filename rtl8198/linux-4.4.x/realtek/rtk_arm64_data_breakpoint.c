/*
 * data_breakpoint.c - Sample HW Breakpoint file to watch kernel data address
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * usage: insmod data_breakpoint.ko ksym=<ksym_name>
 *
 * This file is a kernel module that places a breakpoint over ksym_name kernel
 * variable using Hardware Breakpoint register. The corresponding handler which
 * prints a backtrace is invoked every time a write operation is performed on
 * that variable.
 *
 * Copyright (C) IBM Corporation, 2009
 *
 * Author: K.Prasad <prasad@linux.vnet.ibm.com>
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/kallsyms.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/slab.h>

#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define hw_monitr_count 4

//#define TEST_DATA_BP 
#ifdef TEST_DATA_BP 
int test_val[256] = {[0 ... 31] = 0xdeadbeaf} ;
extern void __do_softirq(void);
#endif
typedef  struct rtk_hw_data{
	u64 hw_monitor_addr;
	u64 hw_bp_addr;
	struct perf_event * __percpu *sample_hbp;
	u8 mon_len;
} rtk_hw_t;

#ifdef TEST_DATA_BP
/*******************************************
 * You should only modify vaddr, len 
 * {VirtualAddress, 0, NULL, Len }
*******************************************/
rtk_hw_t rtk_hw_array[hw_monitr_count] = {
		{(u64)&test_val[0], 0 , NULL, 4},
		{(u64)&test_val[12], 0 , NULL, 8},
		{(u64)__do_softirq, 0 , NULL, 4},
		{0, 0 , NULL},
};
#else
rtk_hw_t rtk_hw_array[hw_monitr_count] = {
		{0, 0 , NULL, 0},
		{0, 0 , NULL, 0},
		{0, 0 , NULL, 0},
		{0, 0 , NULL, 0},
};

#endif
static int rtk_hw_watchpt_add(int idx);

extern struct proc_dir_entry *realtek_proc;
static int rtk_watchpoint_control_proc_read(struct seq_file *m, void *v)
{
	int i;

#if 0
	phys_addr_t paddr;
		paddr = virt_to_phys(&test_val[0]);
		printk("[%s - %d]%pa\n", __func__, __LINE__, &paddr);
#endif	
	for(i=0;i<hw_monitr_count; i++){
		seq_printf(m, "Watchpoint(%d): monitor_addr:0x%llx, mon_len(%d), DBGWVR: 0x%llx, perf_event:%p\n", 
			i, rtk_hw_array[i].hw_monitor_addr, rtk_hw_array[i].mon_len, rtk_hw_array[i].hw_bp_addr, rtk_hw_array[i].sample_hbp);
	}
	

	return 0;

}

static int rtk_watchpoint_control_single_open(struct inode *inode,
                                       struct file *file)
{
    return (single_open(file, rtk_watchpoint_control_proc_read, NULL));
}
extern int pid_max;
static void rtk_watchpoint_test_write(u64 addr, u64 data)
{
	u64 *p;
	p = (u64 *)addr;
	if(!virt_addr_valid(addr)){
		printk("%s, addr:%p, value:0x%8llx. Not Accept!\n", __func__, p, *p);
		return;
	}
	printk("%s, addr:%p, value:0x%8llx\n", __func__, p, *p);
	*p = data;
	printk("%s, addr:%p, value:0x%8llx\n", __func__, p, *p);

	return;
}
static void rtk_watchpoint_test_read(u64 addr)
{
	u64 *p;
	p = (u64 *)addr;
	if(!virt_addr_valid(addr)){
		printk("%s, addr:%p, value:0x%8llx. Not Accept!\n", __func__, p, *p);
		return;
	}
	printk("%s, addr:%p, value:0x%8llx\n", __func__, p, *p);

	return;
}
/******************************************************
*
*
**********************************************************/
#include <asm/hw_breakpoint.h>
#define READ_WB_REG_CASE(OFF, N, REG, VAL)	\
	case (OFF + N):				\
		AARCH64_DBG_READ(N, REG, VAL);	\
		break

#define WRITE_WB_REG_CASE(OFF, N, REG, VAL)	\
	case (OFF + N):				\
		AARCH64_DBG_WRITE(N, REG, VAL);	\
		break

#define GEN_READ_WB_REG_CASES(OFF, REG, VAL)	\
	READ_WB_REG_CASE(OFF,  0, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  1, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  2, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  3, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  4, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  5, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  6, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  7, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  8, REG, VAL);	\
	READ_WB_REG_CASE(OFF,  9, REG, VAL);	\
	READ_WB_REG_CASE(OFF, 10, REG, VAL);	\
	READ_WB_REG_CASE(OFF, 11, REG, VAL);	\
	READ_WB_REG_CASE(OFF, 12, REG, VAL);	\
	READ_WB_REG_CASE(OFF, 13, REG, VAL);	\
	READ_WB_REG_CASE(OFF, 14, REG, VAL);	\
	READ_WB_REG_CASE(OFF, 15, REG, VAL)

#define GEN_WRITE_WB_REG_CASES(OFF, REG, VAL)	\
	WRITE_WB_REG_CASE(OFF,  0, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  1, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  2, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  3, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  4, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  5, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  6, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  7, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  8, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF,  9, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF, 10, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF, 11, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF, 12, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF, 13, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF, 14, REG, VAL);	\
	WRITE_WB_REG_CASE(OFF, 15, REG, VAL)

static void write_wb_reg(int reg, int n, u64 val)
{
	switch (reg + n) {
	GEN_WRITE_WB_REG_CASES(AARCH64_DBG_REG_BVR, AARCH64_DBG_REG_NAME_BVR, val);
	GEN_WRITE_WB_REG_CASES(AARCH64_DBG_REG_BCR, AARCH64_DBG_REG_NAME_BCR, val);
	GEN_WRITE_WB_REG_CASES(AARCH64_DBG_REG_WVR, AARCH64_DBG_REG_NAME_WVR, val);
	GEN_WRITE_WB_REG_CASES(AARCH64_DBG_REG_WCR, AARCH64_DBG_REG_NAME_WCR, val);
	default:
		pr_warning("attempt to write to unknown breakpoint register %d\n", n);
	}
	isb();
}

static u64 read_wb_reg(int reg, int n)
{
	u64 val = 0;

	switch (reg + n) {
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_BVR, AARCH64_DBG_REG_NAME_BVR, val);
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_BCR, AARCH64_DBG_REG_NAME_BCR, val);
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_WVR, AARCH64_DBG_REG_NAME_WVR, val);
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_WCR, AARCH64_DBG_REG_NAME_WCR, val);
	default:
		pr_warning("attempt to read from unknown breakpoint register %d\n", n);
	}

	return val;
}

static void rtk_watchpoint_slot_install(u64 slot_id, u64 mon_addr){

	if(slot_id>=hw_monitr_count){
		printk("SlotID(%llu) over MAX.\n", slot_id);
		return;
	}
	if(rtk_hw_array[slot_id].hw_monitor_addr !=0 ){
		printk("SlotID(%llu)occupied. unisntall first.\n", slot_id);
		return;
	}
	rtk_hw_array[slot_id].hw_monitor_addr = mon_addr;
	rtk_hw_array[slot_id].mon_len = 8;//8 Byte

	printk("[%s]%llu ,0x%llx\n", __func__, slot_id, rtk_hw_array[slot_id].hw_monitor_addr );

	if(rtk_hw_watchpt_add(slot_id) != 0){
		rtk_hw_array[slot_id].hw_monitor_addr = 0;
		rtk_hw_array[slot_id].hw_bp_addr = 0;
		rtk_hw_array[slot_id].sample_hbp = NULL;
	}
	
}

static void rtk_watchpoint_slot_uninstall(u64 slot_id){

	if(slot_id>=hw_monitr_count){
		printk("SlotID(%llu) over MAX.\n", slot_id);
		return;
	}
	if( rtk_hw_array[slot_id].sample_hbp != NULL){
		unregister_wide_hw_breakpoint(rtk_hw_array[slot_id].sample_hbp );
	}
	rtk_hw_array[slot_id].hw_monitor_addr = 0;
	rtk_hw_array[slot_id].hw_bp_addr = 0;
	rtk_hw_array[slot_id].sample_hbp = NULL;
	printk("[%s]slot_id=%llu\n", __func__, slot_id);
}


static void rtk_watchpoint_regs_dump_cpu(void *arg){
	int bp_reg_count;
	int  i;
	u64 regv_v = 0;
	u64 regc_v = 0;
	u32 cpuid = smp_processor_id();
	bp_reg_count = get_num_wrps();
	for(i=0; i< bp_reg_count; i++){
		regc_v = read_wb_reg(AARCH64_DBG_REG_WCR, i);
		printk("[CPU%u]DBGWCR%d: 0x%llx\n", cpuid, i, regc_v);
		regv_v = read_wb_reg(AARCH64_DBG_REG_WVR, i);
		printk("[CPU%u]DBGWVR%d: 0x%llx\n", cpuid, i, regv_v);
	}
	
}

struct rtk_wp_reg_write{
	u32 ctrl;
	u64 val;
	u8 idx;
};

static void rtk_watchpoint_regs_write_cpu(void *arg){
	struct rtk_wp_reg_write *p;
	u32 cpuid = smp_processor_id();
	p = (struct rtk_wp_reg_write *)arg;

	write_wb_reg(AARCH64_DBG_REG_WCR, p->idx, p->ctrl);
	
	printk("[CPU%u]DBGWCR%d: 0x%llx\n", cpuid, p->idx, read_wb_reg(AARCH64_DBG_REG_WCR, p->idx));

	write_wb_reg(AARCH64_DBG_REG_WVR, p->idx, p->val);

	printk("[CPU%u]DBGWVR%d: 0x%llx\n", cpuid, p->idx, read_wb_reg(AARCH64_DBG_REG_WVR, p->idx));
	
}



static void rtk_watchpoint_regs_dump(void){

	int cpu = 0;
	for_each_online_cpu(cpu){
		printk("[%s]cpu%d\n", __func__, cpu);
		smp_call_function_single(cpu, rtk_watchpoint_regs_dump_cpu, NULL, 1 );
	}

}

static void rtk_watchpoint_regs_write(u64 ctrl, u64 val, u64 idx){

	int cpu = 0;
	
	struct rtk_wp_reg_write data;
	
	data.idx = idx;
	data.ctrl = ctrl;
	data.val = val;
	
	for_each_online_cpu(cpu){
		printk("[%s]cpu%d\n", __func__, cpu);
		smp_call_function_single(cpu, rtk_watchpoint_regs_write_cpu, &data, 1 );
	}

}


static ssize_t rtk_watchpoint_control_proc_write(struct file *file,
                                          const char __user * buffer,
                                          size_t count, loff_t * off)
{

    char s[128];

	char option[16];
	u64 value0, value1, other;
    if (copy_from_user(s, buffer, count)) {
        return -EFAULT;
    }
	s[count] = '\0';
	
	if (sscanf(s, "%s %llx %llx", option, &value0, &value1) < 1 )
			return -EINVAL;
	
	if(strcmp("read", option) == 0){
		printk("[HW WatcPoint]Read Vaddr:\n");
		rtk_watchpoint_test_read(value0);
	}else if(strcmp("test", option) == 0){
		printk("[HW WatcPoint]TEST:\n");
		rtk_watchpoint_test_write(value0, value1);
	}else if(strcmp("dump", option) == 0){
		printk("[HW WatcPoint]dump hw register:\n");
		rtk_watchpoint_regs_dump();
	}else if(strcmp("reg_w", option) == 0){
		printk("[HW WatcPoint]write hw register:\n");
		if (sscanf(s, "%s %llx %llx %llx", option, &value0, &value1, &other) < 1 )
			return -EINVAL;
		rtk_watchpoint_regs_write(value0, value1, other);
	}else if(strcmp("install", option) == 0){
		if (sscanf(s, "%s %llu %llx", option, &value0, &value1) < 3 )
			return -EINVAL;
		printk("[HW WatcPoint]install: slot%llu, addr=0x%llx\n", value0, value1);
		rtk_watchpoint_slot_install(value0, value1);//(slot_id, addr)
	}else if(strcmp("uninstall", option) == 0){
		printk("[HW WatcPoint]uninstall: slot_id=%llu\n", value0);
		rtk_watchpoint_slot_uninstall(value0);
	}else{
		printk("[HW WatcPoint]Not Support.\n");
	}
	
    return count;
}


static const struct file_operations fops_wp_control = {
    .open = rtk_watchpoint_control_single_open,
    .write = rtk_watchpoint_control_proc_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static void rtk_watchpoint_test_init(void){
	struct proc_dir_entry *pe;

	pe = proc_create_data("rtk_watchpoint_ctrl",
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
		realtek_proc, &fops_wp_control, NULL);

}
static void rtk_watchpoint_test_remove(void){
	remove_proc_entry("rtk_watchpoint_ctrl", realtek_proc);
}


struct rtk_wp_work {
	 struct work_struct rtk_watchpoint_work;
	 struct perf_event *bp; 
};

static void rtk_watchpoint_routine(struct work_struct *work){
	struct rtk_wp_work *p = container_of(work, struct rtk_wp_work, rtk_watchpoint_work);
	arch_install_hw_breakpoint(p->bp);
	kfree(p);
}


static void  rtk_watchpoint_register_again(struct perf_event *bp){

	//rtk_watchpoint_work = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);
	struct rtk_wp_work *p = kmalloc(sizeof(struct rtk_wp_work), GFP_ATOMIC);
    INIT_WORK(&p->rtk_watchpoint_work, rtk_watchpoint_routine);
	p->bp = bp;
    schedule_work(&p->rtk_watchpoint_work);
}

static inline u64 far_el1_value(void)
{
        u64 val;

        asm volatile("mrs %0, far_el1" : "=r" (val));

        return val;
}
static inline u64 edwar_value(void)
{
        u64 val;

        asm volatile("mrs %0, edwar" : "=r" (val));

        return val;
}

static void rtk_hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{

	printk("[%s]Watch Address:0x%llx , value:0x%llx\n", __func__, 
		bp->attr.bp_addr, *(u64 *)(bp->attr.bp_addr));

	printk("\t    Fault Address Register (EL1):\t 0x%llx\n",
		 far_el1_value());

	print_modules();

	if (regs)
		show_regs(regs);

	dump_stack();

	arch_uninstall_hw_breakpoint(bp);
	rtk_watchpoint_register_again(bp);

}

static int rtk_hw_watchpt_add(int idx){
	int ret;
	u64 alignment_mask;
	u64 addr_offset;
	struct perf_event_attr attr;

	hw_breakpoint_init(&attr);
	//attr.bp_addr = kallsyms_lookup_name(ksym_name);

	if( rtk_hw_array[idx].hw_monitor_addr  ==  0 ){
		return -1;
	}
	
	//arm64: arch_validate_hwbkpt_settings  ask the address must doubleword alignment.
	alignment_mask = 0x7;		
	attr.bp_addr = rtk_hw_array[idx].hw_monitor_addr & ~alignment_mask;
	addr_offset = rtk_hw_array[idx].hw_monitor_addr - attr.bp_addr;

	if((rtk_hw_array[idx].mon_len == 4) && (addr_offset > 0)){
		attr.bp_len = HW_BREAKPOINT_LEN_8;
		rtk_hw_array[idx].mon_len = 8;
	}else if(rtk_hw_array[idx].mon_len == 4){
		attr.bp_len = HW_BREAKPOINT_LEN_4;
	}else if(rtk_hw_array[idx].mon_len  == 8){
		attr.bp_len = HW_BREAKPOINT_LEN_8;
	}else{
		return -1;
	}
	
	//attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
	attr.bp_type = HW_BREAKPOINT_W;

	rtk_hw_array[idx].sample_hbp = register_wide_hw_breakpoint(&attr, rtk_hbp_handler, NULL);
	if (IS_ERR((void __force *)rtk_hw_array[idx].sample_hbp )) {
		ret = PTR_ERR((void __force *)rtk_hw_array[idx].sample_hbp );
		goto fail;
	}
	rtk_hw_array[idx].hw_bp_addr = attr.bp_addr;
	printk("HW Breakpoint event(%d)  for 0x%llx write installed\n", idx, attr.bp_addr);

	return 0;

fail:
	printk("Breakpoint registration failed for index(%d), monitor_addr:0x%llx\n", idx, rtk_hw_array[idx].hw_monitor_addr);
	rtk_hw_array[idx].sample_hbp = NULL;
	return ret;	
}

static void rtk_hw_watchpt_add_all_static(void){
	int i;
	
	for(i=0; i< hw_monitr_count; i++){
			if(rtk_hw_array[i].hw_monitor_addr != 0){
				rtk_hw_watchpt_add(i);	
			}
	}
}



static void rtk_hw_watchpt_del_all(void){
	int idx;
	for(idx=0; idx< hw_monitr_count; idx++){
			if(rtk_hw_array[idx].sample_hbp  != NULL){
				unregister_wide_hw_breakpoint(rtk_hw_array[idx].sample_hbp );
				rtk_hw_array[idx].sample_hbp  = NULL;
			}
	}
	
	
}

static int __init rtk_hw_break_module_init(void)
{
	rtk_watchpoint_test_init();
	rtk_hw_watchpt_add_all_static();
	return 0;
}

static void __exit rtk_hw_break_module_exit(void)
{
	rtk_hw_watchpt_del_all();
	rtk_watchpoint_test_remove();
}

module_init(rtk_hw_break_module_init);
module_exit(rtk_hw_break_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("K.Prasad");
MODULE_DESCRIPTION("ksym breakpoint");
