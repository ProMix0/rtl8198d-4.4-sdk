/*
 * Processor capabilities determination functions.
 *
 * Copyright (C) xxxx  the Anonymous
 * Copyright (C) 1994 - 2006 Ralf Baechle
 * Copyright (C) 2003, 2004  Maciej W. Rozycki
 * Copyright (C) 2001, 2004, 2011, 2012	 MIPS Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Modified for RLX Processors
 * Copyright (C) 2008-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/smp.h>
#include <linux/stddef.h>
#include <linux/export.h>

#include <asm/bugs.h>
#include <asm/cpu.h>
#include <asm/cpu-features.h>
#include <asm/cpu-type.h>
#include <asm/fpu.h>
#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/msa.h>
#include <asm/watch.h>
#include <asm/elf.h>
#include <asm/pgtable-bits.h>
#include <asm/spram.h>
#include <asm/uaccess.h>

/* Hardware capabilities */
unsigned int elf_hwcap __read_mostly;

/*
 * Get the FPU Implementation/Revision.
 */
static inline unsigned long cpu_get_fpu_id(void)
{
	unsigned long tmp, fpu_id;

	tmp = read_c0_status();
	__enable_fpu(FPU_AS_IS);
	fpu_id = read_32bit_cp1_register(CP1_REVISION);
	write_c0_status(tmp);
	return fpu_id;
}

/*
 * Check if the CPU has an external FPU.
 */
static inline int __cpu_has_fpu(void)
{
	return (cpu_get_fpu_id() & FPIR_IMP_MASK) != FPIR_IMP_NONE;
}

static inline unsigned long cpu_get_msa_id(void)
{
	unsigned long status, msa_id;

	status = read_c0_status();
	__enable_fpu(FPU_64BIT);
	enable_msa();
	msa_id = read_msa_ir();
	disable_msa();
	write_c0_status(status);
	return msa_id;
}

/*
 * Determine the FCSR mask for FPU hardware.
 */
static inline void cpu_set_fpu_fcsr_mask(struct cpuinfo_mips *c)
{
	unsigned long sr, mask, fcsr, fcsr0, fcsr1;

	fcsr = c->fpu_csr31;
	mask = FPU_CSR_ALL_X | FPU_CSR_ALL_E | FPU_CSR_ALL_S | FPU_CSR_RM;

	sr = read_c0_status();
	__enable_fpu(FPU_AS_IS);

	fcsr0 = fcsr & mask;
	write_32bit_cp1_register(CP1_STATUS, fcsr0);
	fcsr0 = read_32bit_cp1_register(CP1_STATUS);

	fcsr1 = fcsr | ~mask;
	write_32bit_cp1_register(CP1_STATUS, fcsr1);
	fcsr1 = read_32bit_cp1_register(CP1_STATUS);

	write_32bit_cp1_register(CP1_STATUS, fcsr);

	write_c0_status(sr);

	c->fpu_msk31 = ~(fcsr0 ^ fcsr1) & ~mask;
}

/*
 * Set the FIR feature flags for the FPU emulator.
 */
static void cpu_set_nofpu_id(struct cpuinfo_mips *c)
{
	u32 value;

	value = 0;
	if (c->isa_level & (MIPS_CPU_ISA_M32R1 | MIPS_CPU_ISA_M64R1 |
			    MIPS_CPU_ISA_M32R2 | MIPS_CPU_ISA_M64R2 |
			    MIPS_CPU_ISA_M32R6 | MIPS_CPU_ISA_M64R6))
		value |= MIPS_FPIR_D | MIPS_FPIR_S;
	if (c->isa_level & (MIPS_CPU_ISA_M32R2 | MIPS_CPU_ISA_M64R2 |
			    MIPS_CPU_ISA_M32R6 | MIPS_CPU_ISA_M64R6))
		value |= MIPS_FPIR_F64 | MIPS_FPIR_L | MIPS_FPIR_W;
	c->fpu_id = value;
}

/* Determined FPU emulator mask to use for the boot CPU with "nofpu".  */
static unsigned int mips_nofpu_msk31;

/*
 * Set options for FPU hardware.
 */
static void cpu_set_fpu_opts(struct cpuinfo_mips *c)
{
	c->fpu_id = cpu_get_fpu_id();
	mips_nofpu_msk31 = c->fpu_msk31;

	if (c->isa_level & (MIPS_CPU_ISA_M32R1 | MIPS_CPU_ISA_M64R1 |
			    MIPS_CPU_ISA_M32R2 | MIPS_CPU_ISA_M64R2 |
			    MIPS_CPU_ISA_M32R6 | MIPS_CPU_ISA_M64R6)) {
		if (c->fpu_id & MIPS_FPIR_3D)
			c->ases |= MIPS_ASE_MIPS3D;
		if (c->fpu_id & MIPS_FPIR_FREP)
			c->options |= MIPS_CPU_FRE;
	}

	cpu_set_fpu_fcsr_mask(c);
}

/*
 * Set options for the FPU emulator.
 */
static void cpu_set_nofpu_opts(struct cpuinfo_mips *c)
{
	c->options &= ~MIPS_CPU_FPU;
	c->fpu_msk31 = mips_nofpu_msk31;

	cpu_set_nofpu_id(c);
}

static int mips_fpu_disabled;

static int __init fpu_disable(char *s)
{
	cpu_set_nofpu_opts(&boot_cpu_data);
	mips_fpu_disabled = 1;

	return 1;
}

__setup("nofpu", fpu_disable);

static int mips_dsp_disabled;

static int __init dsp_disable(char *s)
{
	cpu_data[0].ases &= ~(MIPS_ASE_DSP | MIPS_ASE_DSP2P);
	mips_dsp_disabled = 1;

	return 1;
}

__setup("nodsp", dsp_disable);

static int mips_htw_disabled;

static int __init htw_disable(char *s)
{
	mips_htw_disabled = 1;
	cpu_data[0].options &= ~MIPS_CPU_HTW;
	write_c0_pwctl(read_c0_pwctl() &
		       ~(1 << MIPS_PWCTL_PWEN_SHIFT));

	return 1;
}

__setup("nohtw", htw_disable);

static int mips_ftlb_disabled;
static int mips_has_ftlb_configured;

static int set_ftlb_enable(struct cpuinfo_mips *c, int enable);

static int __init ftlb_disable(char *s)
{
	unsigned int config4, mmuextdef;

	/*
	 * If the core hasn't done any FTLB configuration, there is nothing
	 * for us to do here.
	 */
	if (!mips_has_ftlb_configured)
		return 1;

	/* Disable it in the boot cpu */
	if (set_ftlb_enable(&cpu_data[0], 0)) {
		pr_warn("Can't turn FTLB off\n");
		return 1;
	}

	back_to_back_c0_hazard();

	config4 = read_c0_config4();

	/* Check that FTLB has been disabled */
	mmuextdef = config4 & MIPS_CONF4_MMUEXTDEF;
	/* MMUSIZEEXT == VTLB ON, FTLB OFF */
	if (mmuextdef == MIPS_CONF4_MMUEXTDEF_FTLBSIZEEXT) {
		/* This should never happen */
		pr_warn("FTLB could not be disabled!\n");
		return 1;
	}

	mips_ftlb_disabled = 1;
	mips_has_ftlb_configured = 0;

	/*
	 * noftlb is mainly used for debug purposes so print
	 * an informative message instead of using pr_debug()
	 */
	pr_info("FTLB has been disabled\n");

	/*
	 * Some of these bits are duplicated in the decode_config4.
	 * MIPS_CONF4_MMUEXTDEF_MMUSIZEEXT is the only possible case
	 * once FTLB has been disabled so undo what decode_config4 did.
	 */
	cpu_data[0].tlbsize -= cpu_data[0].tlbsizeftlbways *
			       cpu_data[0].tlbsizeftlbsets;
	cpu_data[0].tlbsizeftlbsets = 0;
	cpu_data[0].tlbsizeftlbways = 0;

	return 1;
}

__setup("noftlb", ftlb_disable);


static inline void check_errata(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;

	switch (current_cpu_type()) {
	case CPU_34K:
		/*
		 * Erratum "RPS May Cause Incorrect Instruction Execution"
		 * This code only handles VPE0, any SMP/RTOS code
		 * making use of VPE1 will be responsable for that VPE.
		 */
		if ((c->processor_id & PRID_REV_MASK) <= PRID_REV_34K_V1_0_2)
			write_c0_config7(read_c0_config7() | MIPS_CONF7_RPS);
		break;
	default:
		break;
	}
}

void __init check_bugs32(void)
{
	check_errata();
}

/*
 * Probe whether cpu has config register by trying to play with
 * alternate cache bit and see whether it matters.
 * It's used by cpu_probe to distinguish between R3000A and R3081.
 */
static inline int cpu_has_confreg(void)
{
#ifdef CONFIG_CPU_R3000
	extern unsigned long r3k_cache_size(unsigned long);
	unsigned long size1, size2;
	unsigned long cfg = read_c0_conf();

	size1 = r3k_cache_size(ST0_ISC);
	write_c0_conf(cfg ^ R30XX_CONF_AC);
	size2 = r3k_cache_size(ST0_ISC);
	write_c0_conf(cfg);
	return size1 != size2;
#else
	return 0;
#endif
}

static inline void set_elf_platform(int cpu, const char *plat)
{
	if (cpu == 0)
		__elf_platform = plat;
}

static inline void cpu_probe_vmbits(struct cpuinfo_mips *c)
{
#ifdef __NEED_VMBITS_PROBE
	write_c0_entryhi(0x3fffffffffffe000ULL);
	back_to_back_c0_hazard();
	c->vmbits = fls64(read_c0_entryhi() & 0x3fffffffffffe000ULL);
#endif
}

static void set_isa(struct cpuinfo_mips *c, unsigned int isa)
{
	switch (isa) {
	case MIPS_CPU_ISA_M64R2:
		c->isa_level |= MIPS_CPU_ISA_M32R2 | MIPS_CPU_ISA_M64R2;
	case MIPS_CPU_ISA_M64R1:
		c->isa_level |= MIPS_CPU_ISA_M32R1 | MIPS_CPU_ISA_M64R1;
	case MIPS_CPU_ISA_V:
		c->isa_level |= MIPS_CPU_ISA_V;
	case MIPS_CPU_ISA_IV:
		c->isa_level |= MIPS_CPU_ISA_IV;
	case MIPS_CPU_ISA_III:
		c->isa_level |= MIPS_CPU_ISA_II | MIPS_CPU_ISA_III;
		break;

	/* R6 incompatible with everything else */
	case MIPS_CPU_ISA_M64R6:
		c->isa_level |= MIPS_CPU_ISA_M32R6 | MIPS_CPU_ISA_M64R6;
	case MIPS_CPU_ISA_M32R6:
		c->isa_level |= MIPS_CPU_ISA_M32R6;
		/* Break here so we don't add incompatible ISAs */
		break;
	case MIPS_CPU_ISA_M32R2:
		c->isa_level |= MIPS_CPU_ISA_M32R2;
	case MIPS_CPU_ISA_M32R1:
		c->isa_level |= MIPS_CPU_ISA_M32R1;
	case MIPS_CPU_ISA_II:
		c->isa_level |= MIPS_CPU_ISA_II;
		break;
	}
}

static char unknown_isa[] = KERN_ERR \
	"Unsupported ISA type, c0.config0: %d.";

static unsigned int calculate_ftlb_probability(struct cpuinfo_mips *c)
{

	unsigned int probability = c->tlbsize / c->tlbsizevtlb;

	/*
	 * 0 = All TLBWR instructions go to FTLB
	 * 1 = 15:1: For every 16 TBLWR instructions, 15 go to the
	 * FTLB and 1 goes to the VTLB.
	 * 2 = 7:1: As above with 7:1 ratio.
	 * 3 = 3:1: As above with 3:1 ratio.
	 *
	 * Use the linear midpoint as the probability threshold.
	 */
	if (probability >= 12)
		return 1;
	else if (probability >= 6)
		return 2;
	else
		/*
		 * So FTLB is less than 4 times bigger than VTLB.
		 * A 3:1 ratio can still be useful though.
		 */
		return 3;
}

static int set_ftlb_enable(struct cpuinfo_mips *c, int enable)
{
	unsigned int config;

	if (!cpu_has_ftlb)
		return 1;

	/* It's implementation dependent how the FTLB can be enabled */
	switch (c->cputype) {
	case CPU_PROAPTIV:
	case CPU_P5600:
		/* proAptiv & related cores use Config6 to enable the FTLB */
		config = read_c0_config6();
		/* Clear the old probability value */
		config &= ~(3 << MIPS_CONF6_FTLBP_SHIFT);
		if (enable)
			/* Enable FTLB */
			write_c0_config6(config |
					 (calculate_ftlb_probability(c)
					  << MIPS_CONF6_FTLBP_SHIFT)
					 | MIPS_CONF6_FTLBEN);
		else
			/* Disable FTLB */
			write_c0_config6(config &  ~MIPS_CONF6_FTLBEN);
		break;
	case CPU_I6400:
		/* I6400 & related cores use Config7 to configure FTLB */
		config = read_c0_config7();
		/* Clear the old probability value */
		config &= ~(3 << MIPS_CONF7_FTLBP_SHIFT);
		write_c0_config7(config | (calculate_ftlb_probability(c)
					   << MIPS_CONF7_FTLBP_SHIFT));
		break;
	default:
		return 1;
	}

	return 0;
}

static inline unsigned int decode_config0(struct cpuinfo_mips *c)
{
	unsigned int config0;
	int isa, mt;

	config0 = read_c0_config();

	/*
	 * Look for Standard TLB or Dual VTLB and FTLB
	 */
	mt = config0 & MIPS_CONF_MT;
	if (mt == MIPS_CONF_MT_TLB)
		c->options |= MIPS_CPU_TLB;
	else if (mt == MIPS_CONF_MT_FTLB)
		c->options |= MIPS_CPU_TLB | MIPS_CPU_FTLB;

	isa = (config0 & MIPS_CONF_AT) >> 13;
	switch (isa) {
	case 0:
		switch ((config0 & MIPS_CONF_AR) >> 10) {
		case 0:
			set_isa(c, MIPS_CPU_ISA_M32R1);
			break;
		case 1:
			set_isa(c, MIPS_CPU_ISA_M32R2);
			break;
		case 2:
			set_isa(c, MIPS_CPU_ISA_M32R6);
			break;
		default:
			goto unknown;
		}
		break;
	case 2:
		switch ((config0 & MIPS_CONF_AR) >> 10) {
		case 0:
			set_isa(c, MIPS_CPU_ISA_M64R1);
			break;
		case 1:
			set_isa(c, MIPS_CPU_ISA_M64R2);
			break;
		case 2:
			set_isa(c, MIPS_CPU_ISA_M64R6);
			break;
		default:
			goto unknown;
		}
		break;
	default:
		goto unknown;
	}

	return config0 & MIPS_CONF_M;

unknown:
	panic(unknown_isa, config0);
}

static inline unsigned int decode_config1(struct cpuinfo_mips *c)
{
	unsigned int config1;

	config1 = read_c0_config1();

	if (config1 & MIPS_CONF1_MD)
		c->ases |= MIPS_ASE_MDMX;
	if (config1 & MIPS_CONF1_WR)
		c->options |= MIPS_CPU_WATCH;
	if (config1 & MIPS_CONF1_CA)
		c->ases |= MIPS_ASE_MIPS16;
	if (config1 & MIPS_CONF1_EP)
		c->options |= MIPS_CPU_EJTAG;
	if (config1 & MIPS_CONF1_FP) {
		c->options |= MIPS_CPU_FPU;
		c->options |= MIPS_CPU_32FPR;
	}
	if (cpu_has_tlb) {
		c->tlbsize = ((config1 & MIPS_CONF1_TLBS) >> 25) + 1;
		c->tlbsizevtlb = c->tlbsize;
		c->tlbsizeftlbsets = 0;
	}

	return config1 & MIPS_CONF_M;
}

static inline unsigned int decode_config2(struct cpuinfo_mips *c)
{
	unsigned int config2;

	config2 = read_c0_config2();

	if (config2 & MIPS_CONF2_SL)
		c->scache.flags &= ~MIPS_CACHE_NOT_PRESENT;

	return config2 & MIPS_CONF_M;
}

static inline unsigned int decode_config3(struct cpuinfo_mips *c)
{
	unsigned int config3;

	config3 = read_c0_config3();

	if (config3 & MIPS_CONF3_SM) {
		c->ases |= MIPS_ASE_SMARTMIPS;
		c->options |= MIPS_CPU_RIXI;
	}
	if (config3 & MIPS_CONF3_RXI)
		c->options |= MIPS_CPU_RIXI;
	if (config3 & MIPS_CONF3_DSP)
		c->ases |= MIPS_ASE_DSP;
	if (config3 & MIPS_CONF3_DSP2P)
		c->ases |= MIPS_ASE_DSP2P;
	if (config3 & MIPS_CONF3_VINT)
		c->options |= MIPS_CPU_VINT;
	if (config3 & MIPS_CONF3_VEIC)
		c->options |= MIPS_CPU_VEIC;
	if (config3 & MIPS_CONF3_MT)
		c->ases |= MIPS_ASE_MIPSMT;
	if (config3 & MIPS_CONF3_ULRI)
		c->options |= MIPS_CPU_ULRI;
	if (config3 & MIPS_CONF3_ISA)
		c->options |= MIPS_CPU_MICROMIPS;
	if (config3 & MIPS_CONF3_VZ)
		c->ases |= MIPS_ASE_VZ;
	if (config3 & MIPS_CONF3_SC)
		c->options |= MIPS_CPU_SEGMENTS;
	if (config3 & MIPS_CONF3_MSA)
		c->ases |= MIPS_ASE_MSA;
	if (config3 & MIPS_CONF3_PW) {
		c->htw_seq = 0;
		c->options |= MIPS_CPU_HTW;
	}
	if (config3 & MIPS_CONF3_CDMM)
		c->options |= MIPS_CPU_CDMM;
	if (config3 & MIPS_CONF3_SP)
		c->options |= MIPS_CPU_SP;

	return config3 & MIPS_CONF_M;
}

static inline unsigned int decode_config4(struct cpuinfo_mips *c)
{
	unsigned int config4;
	unsigned int newcf4;
	unsigned int mmuextdef;
	unsigned int ftlb_page = MIPS_CONF4_FTLBPAGESIZE;

	config4 = read_c0_config4();

	if (cpu_has_tlb) {
		if (((config4 & MIPS_CONF4_IE) >> 29) == 2)
			c->options |= MIPS_CPU_TLBINV;

		/*
		 * R6 has dropped the MMUExtDef field from config4.
		 * On R6 the fields always describe the FTLB, and only if it is
		 * present according to Config.MT.
		 */
		if (!cpu_has_mips_r6)
			mmuextdef = config4 & MIPS_CONF4_MMUEXTDEF;
		else if (cpu_has_ftlb)
			mmuextdef = MIPS_CONF4_MMUEXTDEF_VTLBSIZEEXT;
		else
			mmuextdef = 0;

		switch (mmuextdef) {
		case MIPS_CONF4_MMUEXTDEF_MMUSIZEEXT:
			c->tlbsize += (config4 & MIPS_CONF4_MMUSIZEEXT) * 0x40;
			c->tlbsizevtlb = c->tlbsize;
			break;
		case MIPS_CONF4_MMUEXTDEF_VTLBSIZEEXT:
			c->tlbsizevtlb +=
				((config4 & MIPS_CONF4_VTLBSIZEEXT) >>
				  MIPS_CONF4_VTLBSIZEEXT_SHIFT) * 0x40;
			c->tlbsize = c->tlbsizevtlb;
			ftlb_page = MIPS_CONF4_VFTLBPAGESIZE;
			/* fall through */
		case MIPS_CONF4_MMUEXTDEF_FTLBSIZEEXT:
			if (mips_ftlb_disabled)
				break;
			newcf4 = (config4 & ~ftlb_page) |
				(page_size_ftlb(mmuextdef) <<
				 MIPS_CONF4_FTLBPAGESIZE_SHIFT);
			write_c0_config4(newcf4);
			back_to_back_c0_hazard();
			config4 = read_c0_config4();
			if (config4 != newcf4) {
				pr_err("PAGE_SIZE 0x%lx is not supported by FTLB (config4=0x%x)\n",
				       PAGE_SIZE, config4);
				/* Switch FTLB off */
				set_ftlb_enable(c, 0);
				break;
			}
			c->tlbsizeftlbsets = 1 <<
				((config4 & MIPS_CONF4_FTLBSETS) >>
				 MIPS_CONF4_FTLBSETS_SHIFT);
			c->tlbsizeftlbways = ((config4 & MIPS_CONF4_FTLBWAYS) >>
					      MIPS_CONF4_FTLBWAYS_SHIFT) + 2;
			c->tlbsize += c->tlbsizeftlbways * c->tlbsizeftlbsets;
			mips_has_ftlb_configured = 1;
			break;
		}
	}

	c->kscratch_mask = (config4 >> 16) & 0xff;

	return config4 & MIPS_CONF_M;
}

static inline unsigned int decode_config5(struct cpuinfo_mips *c)
{
	unsigned int config5;

	config5 = read_c0_config5();
	config5 &= ~(MIPS_CONF5_UFR | MIPS_CONF5_UFE);
	write_c0_config5(config5);

	if (config5 & MIPS_CONF5_EVA)
		c->options |= MIPS_CPU_EVA;
	if (config5 & MIPS_CONF5_MRP)
		c->options |= MIPS_CPU_MAAR;
	if (config5 & MIPS_CONF5_LLB)
		c->options |= MIPS_CPU_RW_LLB;
#ifdef CONFIG_XPA
	if (config5 & MIPS_CONF5_MVH)
		c->options |= MIPS_CPU_XPA;
#endif

	return config5 & MIPS_CONF_M;
}

static void decode_configs(struct cpuinfo_mips *c)
{
	int ok;

	/* MIPS32 or MIPS64 compliant CPU.  */
	c->options = MIPS_CPU_4KEX | MIPS_CPU_4K_CACHE | MIPS_CPU_COUNTER |
		     MIPS_CPU_DIVEC | MIPS_CPU_LLSC | MIPS_CPU_MCHECK;

	c->scache.flags = MIPS_CACHE_NOT_PRESENT;

	/* Enable FTLB if present and not disabled */
	set_ftlb_enable(c, !mips_ftlb_disabled);

	ok = decode_config0(c);			/* Read Config registers.  */
	BUG_ON(!ok);				/* Arch spec violation!	 */
	if (ok)
		ok = decode_config1(c);
	if (ok)
		ok = decode_config2(c);
	if (ok)
		ok = decode_config3(c);
	if (ok)
		ok = decode_config4(c);
	if (ok)
		ok = decode_config5(c);

#ifdef CONFIG_HARDWARE_WATCHPOINTS
	mips_probe_watch_registers(c);
#endif

	if (cpu_has_rixi) {
		/* Enable the RIXI exceptions */
		set_c0_pagegrain(PG_IEC);
		back_to_back_c0_hazard();
		/* Verify the IEC bit is set */
		if (read_c0_pagegrain() & PG_IEC)
			c->options |= MIPS_CPU_RIXIEX;
	}

#ifndef CONFIG_MIPS_CPS
	if (cpu_has_mips_r2_r6) {
		c->core = get_ebase_cpunum();
		if (cpu_has_mipsmt)
			c->core >>= fls(core_nvpes()) - 1;
	}
#endif
}

#define R4K_OPTS (MIPS_CPU_TLB | MIPS_CPU_4KEX | MIPS_CPU_4K_CACHE \
		| MIPS_CPU_COUNTER)

static inline void cpu_probe_mips(struct cpuinfo_mips *c, unsigned int cpu)
{
	c->writecombine = _CACHE_UNCACHED_ACCELERATED;
	switch (c->processor_id & PRID_IMP_MASK) {
	case PRID_IMP_QEMU_GENERIC:
		c->writecombine = _CACHE_UNCACHED;
		c->cputype = CPU_QEMU_GENERIC;
		__cpu_name[cpu] = "MIPS GENERIC QEMU";
		break;
	case PRID_IMP_4KC:
		c->cputype = CPU_4KC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 4Kc";
		break;
	case PRID_IMP_4KEC:
	case PRID_IMP_4KECR2:
		c->cputype = CPU_4KEC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 4KEc";
		break;
	case PRID_IMP_4KSC:
	case PRID_IMP_4KSD:
		c->cputype = CPU_4KSC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 4KSc";
		break;
	case PRID_IMP_5KC:
		c->cputype = CPU_5KC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 5Kc";
		break;
	case PRID_IMP_5KE:
		c->cputype = CPU_5KE;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 5KE";
		break;
	case PRID_IMP_20KC:
		c->cputype = CPU_20KC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 20Kc";
		break;
	case PRID_IMP_24K:
		c->cputype = CPU_24K;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 24Kc";
		break;
	case PRID_IMP_24KE:
		c->cputype = CPU_24K;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 24KEc";
		break;
	case PRID_IMP_25KF:
		c->cputype = CPU_25KF;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 25Kc";
		break;
	case PRID_IMP_34K:
		c->cputype = CPU_34K;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 34Kc";
		break;
	case PRID_IMP_74K:
		c->cputype = CPU_74K;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 74Kc";
		break;
	case PRID_IMP_M14KC:
		c->cputype = CPU_M14KC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS M14Kc";
		break;
	case PRID_IMP_M14KEC:
		c->cputype = CPU_M14KEC;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS M14KEc";
		break;
	case PRID_IMP_1004K:
		c->cputype = CPU_1004K;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 1004Kc";
		break;
	case PRID_IMP_1074K:
		c->cputype = CPU_1074K;
		c->writecombine = _CACHE_UNCACHED;
		__cpu_name[cpu] = "MIPS 1074Kc";
		break;
	case PRID_IMP_INTERAPTIV_UP:
		c->cputype = CPU_INTERAPTIV;
		__cpu_name[cpu] = "MIPS interAptiv";
		break;
	case PRID_IMP_INTERAPTIV_MP:
		c->cputype = CPU_INTERAPTIV;
		__cpu_name[cpu] = "MIPS interAptiv (multi)";
		break;
	case PRID_IMP_PROAPTIV_UP:
		c->cputype = CPU_PROAPTIV;
		__cpu_name[cpu] = "MIPS proAptiv";
		break;
	case PRID_IMP_PROAPTIV_MP:
		c->cputype = CPU_PROAPTIV;
		__cpu_name[cpu] = "MIPS proAptiv (multi)";
		break;
	case PRID_IMP_P5600:
		c->cputype = CPU_P5600;
		__cpu_name[cpu] = "MIPS P5600";
		break;
	case PRID_IMP_I6400:
		c->cputype = CPU_I6400;
		__cpu_name[cpu] = "MIPS I6400";
		break;
	case PRID_IMP_M5150:
		c->cputype = CPU_M5150;
		__cpu_name[cpu] = "MIPS M5150";
		break;
	}

	decode_configs(c);

#ifdef CONFIG_CPU_HAS_SPRAM
	spram_config();
#endif
}

static inline void cpu_probe_taroko(struct cpuinfo_mips *c, unsigned int cpu)
{
	c->options = MIPS_CPU_TLB | MIPS_CPU_3K_CACHE;
	c->tlbsize = cpu_tlb_entry;  /* defined in bspcpu.h */
	c->processor_id = read_c0_prid();

#ifdef CONFIG_CPU_HAS_FPU
	c->fpu_id = cpu_get_fpu_id();
	c->options |= MIPS_CPU_FPU;
	c->options |= MIPS_CPU_32FPR;
#else
	c->fpu_id = FPIR_IMP_NONE;
	c->options |= MIPS_CPU_NOFPUEX;
#endif

#ifdef CONFIG_HARDWARE_WATCHPOINTS
	c->options |= MIPS_CPU_WATCH;
	mips_probe_watch_registers(c);
	c->watch_reg_use_cnt = c->watch_reg_count / 2;
#endif

	__cpu_name[cpu] = "Taroko";
	set_elf_platform(cpu, "Taroko");

#ifdef CONFIG_CPU_RLX4281
	c->cputype = CPU_RLX4281;
#endif
#ifdef CONFIG_CPU_RLX5281
	c->cputype = CPU_RLX5281;
#endif
#ifdef CONFIG_CPU_RLX4181
	c->cputype = CPU_RLX4181;
#endif
#ifdef CONFIG_CPU_RLX5181
	c->cputype = CPU_RLX5181;
#endif
}

#ifdef CONFIG_64BIT
/* For use by uaccess.h */
u64 __ua_limit;
EXPORT_SYMBOL(__ua_limit);
#endif

const char *__cpu_name[NR_CPUS];
const char *__elf_platform;

void cpu_probe(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned int cpu = smp_processor_id();

	c->processor_id = PRID_IMP_UNKNOWN;
	c->fpu_id	= FPIR_IMP_NONE;
	c->cputype	= CPU_UNKNOWN;
	c->writecombine = _CACHE_UNCACHED;

	c->fpu_csr31	= FPU_CSR_RN;
	c->fpu_msk31	= FPU_CSR_RSVD | FPU_CSR_ABS2008 | FPU_CSR_NAN2008;

	c->processor_id = read_c0_prid();
	if (cpu_has_mips_r)
		cpu_probe_mips(c, cpu);
	else
		cpu_probe_taroko(c, cpu);

	BUG_ON(!__cpu_name[cpu]);
	BUG_ON(c->cputype == CPU_UNKNOWN);

	/*
	 * Platform code can force the cpu type to optimize code
	 * generation. In that case be sure the cpu type is correctly
	 * manually setup otherwise it could trigger some nasty bugs.
	 */
	BUG_ON(current_cpu_type() != c->cputype);

	if (!cpu_has_fpu || mips_fpu_disabled)
		c->options &= ~MIPS_CPU_FPU;

	if (!cpu_has_dsp || mips_dsp_disabled)
		c->ases &= ~(MIPS_ASE_DSP | MIPS_ASE_DSP2P);

	if (mips_htw_disabled) {
		c->options &= ~MIPS_CPU_HTW;
		write_c0_pwctl(read_c0_pwctl() &
			       ~(1 << MIPS_PWCTL_PWEN_SHIFT));
	}

	if (cpu_has_fpu && c->options & MIPS_CPU_FPU)
		cpu_set_fpu_opts(c);
	else
		cpu_set_nofpu_opts(c);

	if (cpu_has_bp_ghist)
		write_c0_r10k_diag(read_c0_r10k_diag() |
				   R10K_DIAG_E_GHIST);

	if (cpu_has_mips_r2_r6) {
		c->srsets = ((read_c0_srsctl() >> 26) & 0x0f) + 1;
		/* R2 has Performance Counter Interrupt indicator */
		c->options |= MIPS_CPU_PCI;
	}
	else
		c->srsets = 1;

	if (cpu_has_mips_r6)
		elf_hwcap |= HWCAP_MIPS_R6;

	if (cpu_has_msa) {
		c->msa_id = cpu_get_msa_id();
		WARN(c->msa_id & MSA_IR_WRPF,
		     "Vector register partitioning unimplemented!");
		elf_hwcap |= HWCAP_MIPS_MSA;
	}

	cpu_probe_vmbits(c);

#ifdef CONFIG_64BIT
	if (cpu == 0)
		__ua_limit = ~((1ull << cpu_vmbits) - 1);
#endif
}

void cpu_report(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;

	pr_info("CPU%d revision is: %08x (%s)\n",
		smp_processor_id(), c->processor_id, cpu_name_string());
	if (c->options & MIPS_CPU_FPU)
		printk(KERN_INFO "FPU revision is: %08x\n", c->fpu_id);
	if (cpu_has_msa)
		pr_info("MSA revision is: %08x\n", c->msa_id);
}
