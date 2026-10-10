/*
 * Copyright (c) Cortina-Access Limited 2017.  All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/**/ 
#ifndef _CA_DRAM_CTRL_H_
#define _CA_DRAM_CTRL_H_
 
/* HW registers */
#define CRT_CTL                0xF4328000
#define PLL_CTL0               0xF4328004
#define PLL_CTL1               0xF4328008
#define PLL_CTL2               0xF432800C
#define PLL_PI0                0xF4328010
#define PLL_PI1                0xF4328014
#define PLL_CTL3               0xF4328018
#define SSC0                   0xF432801C
#define SSC1                   0xF4328020
#define SSC2                   0xF4328024
#define SSC3                   0xF4328028
#define WDOG                   0xF432802C
#define DCC_CMD                0xF4328030
#define DCC_DQS_0              0xF4328034
#define DCC_DQS_1              0xF4328038
#define DCC_DQS_2              0xF432803C
#define DCC_DQS_3              0xF4328040
#define DCC_DQ_0               0xF4328044
#define DCC_DQ_1               0xF4328048
#define DCC_DQ_2               0xF432804C
#define DCC_DQ_3               0xF4328050
#define DCC_CK_0               0xF4328054
#define DCC_CK_1               0xF4328058
#define DLL_SET0_0             0xF432805C
#define DLL_SET0_1             0xF4328060
#define DLL_SET0_2             0xF4328064
#define DLL_SET0_3             0xF4328068
#define DLL_SET1_0             0xF432806C
#define DLL_SET1_1             0xF4328070
#define DLL_SET1_2             0xF4328074
#define DLL_SET1_3             0xF4328078
#define CRT_DMY                0xF432807C
#define AFIFO_STR_0            0xF4328080
#define AFIFO_STR_1            0xF4328084
#define AFIFO_STR_2            0xF4328088
#define TX_FIFO_RST            0xF432808C
#define DLL_SET2               0xF4328090
#define AFIFO_STR_SEL          0xF4328094
#define PLL_PI2                0xF4328098
#define PLL_CTL4               0xF432809C
#define PLL_PI3                0xF43280A0
#define DCC_CS                 0xF43280A4

#define ADR_DLY_0              0xF4328200
#define ADR_DLY_1              0xF4328204
#define ADR_DLY_2              0xF4328208
#define CMD_DLY_0              0xF432820C
#define PAD_OE_CTL             0xF4328210
#define PAD_IDDQ               0xF4328214
#define PAD_ZCTRL_OVRD         0xF4328218
#define PAD_CTRL_PROG          0xF432821C
#define PAD_ZCTRL_STATUS       0xF4328220
#define WRLVL_CTRL             0xF4328224
#define WRLVL_RD_DATA          0xF4328228
#define TEST_CTRL0             0xF432822C
#define DQ_DLY_0_0             0xF4328230
#define DQ_DLY_0_1             0xF4328234
#define DQ_DLY_0_2             0xF4328238
#define DQ_DLY_0_3             0xF432823C
#define DQ_DLY_1               0xF4328240
#define READ_CTRL_0_0          0xF4328244
#define READ_CTRL_0_1          0xF4328248
#define READ_CTRL_0_2          0xF432824C
#define READ_CTRL_0_3          0xF4328250
#define READ_CTRL_1            0xF4328254
#define CMD_DLY_1              0xF4328258
#define OCDP_SET1              0xF432825C
#define OCDN_SET1              0xF4328260
#define DQ_ODT_SEL             0xF4328264
#define CAL_SHIFT_CTRL         0xF4328268
#define CAL_LS_SEL_0           0xF432826C
#define CAL_LS_SEL_1           0xF4328270
#define CAL_LS_SEL_2           0xF4328274
#define CAL_LS_SEL_3           0xF4328278
#define CAL_RS_SEL_0           0xF432827C
#define CAL_RS_SEL_1           0xF4328280
#define CAL_RS_SEL_2           0xF4328284
#define CAL_RS_SEL_3           0xF4328288
#define DQS_IN_DLY_0_0         0xF432828C
#define DQS_IN_DLY_0_1         0xF4328290
#define DQS_IN_DLY_0_2         0xF4328294
#define DQS_IN_DLY_0_3         0xF4328298
#define DQS_IN_DLY_1_0         0xF432829C
#define DQS_IN_DLY_1_1         0xF43282A0
#define DQS_IN_DLY_1_2         0xF43282A4
#define DQS_IN_DLY_1_3         0xF43282A8
#define DQS_IN_DLY_2_0         0xF43282AC
#define DQS_IN_DLY_2_1         0xF43282B0
#define DQS_IN_DLY_2_2         0xF43282B4
#define DQS_IN_DLY_2_3         0xF43282B8
#define DQS_IN_DLY_3_0         0xF43282BC
#define DQS_IN_DLY_3_1         0xF43282C0
#define DQS_IN_DLY_3_2         0xF43282C4
#define DQS_IN_DLY_3_3         0xF43282C8
#define PAD_CTRL_ZPROG         0xF43282CC
#define DT_SET0                0xF43282D0
#define DQ_OCD_SEL             0xF43282D4
#define DQS_P_ODT_SEL          0xF43282D8
#define DQS_N_ODT_SEL          0xF43282DC
#define ODT_TTCP0_SET1         0xF43282E0
#define ODT_TTCN_SET1          0xF43282E4
#define ODT_TTFP_SET1          0xF43282E8
#define ODT_TTFN_SET1          0xF43282EC
#define DQS_OCD_SEL            0xF43282F0
#define ADR_OCD_SEL            0xF43282F4
#define CK_OCD_SEL             0xF43282F8
#define PAD_BUS_0              0xF43282FC
#define PAD_BUS_1              0xF4328300
#define ZQCMP                  0xF4328304
#define ODT_TTCP0_SET0         0xF4328308
#define ODT_TTCN_SET0          0xF432830C
#define ODT_TTFP_SET0          0xF4328310
#define ODT_TTFN_SET0          0xF4328314
#define DPI_CTRL_0             0xF4328318
#define DPI_CTRL_1             0xF432831C
#define CAL_OUT_SEL            0xF4328320
#define CAL_OUT_0_0            0xF4328324
#define CAL_OUT_0_1            0xF4328328
#define CAL_OUT_0_2            0xF432832C
#define CAL_OUT_0_3            0xF4328330
#define WR_FIFO_0_0            0xF4328334
#define WR_FIFO_0_1            0xF4328338
#define WR_FIFO_0_2            0xF432833C
#define WR_FIFO_0_3            0xF4328340
#define WR_FIFO_1_0            0xF4328344
#define WR_FIFO_1_1            0xF4328348
#define WR_FIFO_1_2            0xF432834C
#define WR_FIFO_1_3            0xF4328350
#define RD_FIFO_0_0            0xF4328354
#define RD_FIFO_0_1            0xF4328358
#define RD_FIFO_0_2            0xF432835C
#define RD_FIFO_0_3            0xF4328360
#define RD_FIFO_1_0            0xF4328364
#define RD_FIFO_1_1            0xF4328368
#define RD_FIFO_1_2            0xF432836C
#define RD_FIFO_1_3            0xF4328370
#define OCDP_SET0              0xF4328374
#define OCDN_SET0              0xF4328378
#define TEST_CTRL1             0xF432837C
#define DQ_DLY_2               0xF4328380
#define VALID_WIN_SEL          0xF4328384
#define PAD_RZCTRL_STATUS      0xF4328388
#define PRBS7_TMUX9            0xF432838C
#define VALID_WIN_PH           0xF4328390
#define NULL_DQS               0xF4328394
#define BIST_DEBUG             0xF4328398
#define VALID_WIN_DET          0xF432839C
#define BIST_2TO1_0            0xF43283A0
#define BIST_2TO1_1            0xF43283A4
#define BIST_PT                0xF43283A8
#define INT_CTRL               0xF43283AC
#define INT_STATUS_0           0xF43283B0
#define INT_STATUS_1_0         0xF43283B4
#define INT_STATUS_1_1         0xF43283B8
#define INT_STATUS_1_2         0xF43283BC
#define INT_STATUS_1_3         0xF43283C0
#define INT_STATUS_2           0xF43283C4
#define SSC_STATUS             0xF43283C8
#define DUMMY_0                0xF43283CC
#define DUMMY_1                0xF43283D0
#define DUMMY_2                0xF43283D4
#define DUMMY_3                0xF43283D8
#define TM_DQSEN_CAL_STAT      0xF43283DC
#define DQSEN_CAL_STATUS       0xF43283E0
#define INT_STATUS_3_0         0xF43283E4
#define INT_STATUS_3_1         0xF43283E8
#define INT_STATUS_3_2         0xF43283EC
#define INT_STATUS_3_3         0xF43283F0
#define HALF_CYC_DET           0xF43283F4
#define ODT_TTCP1_SET0         0xF43283F8
#define PRBS7_TMUX0            0xF43283FC
#define PRBS7_TMUX1            0xF4328400
#define PRBS7_TMUX2            0xF4328404
#define PRBS7_TMUX3            0xF4328408
#define PRBS7_TMUX4_0          0xF432840C
#define PRBS7_TMUX4_1          0xF4328410
#define PRBS7_TMUX4_2          0xF4328414
#define PRBS7_TMUX4_3          0xF4328418
#define PRBS7_TMUX5_0          0xF432841C
#define PRBS7_TMUX5_1          0xF4328420
#define PRBS7_TMUX5_2          0xF4328424
#define PRBS7_TMUX5_3          0xF4328428
#define CAL_LS_SEL_DBI_0       0xF432842C
#define CAL_LS_SEL_DBI_1       0xF4328430
#define CAL_LS_SEL_DBI_2       0xF4328434
#define CAL_LS_SEL_DBI_3       0xF4328438
#define CAL_RS_SEL_DBI_0       0xF432843C
#define CAL_RS_SEL_DBI_1       0xF4328440
#define CAL_RS_SEL_DBI_2       0xF4328444
#define CAL_RS_SEL_DBI_3       0xF4328448
#define DQS_IN_DLY_1_DBI_0     0xF432844C
#define DQS_IN_DLY_1_DBI_1     0xF4328450
#define DQS_IN_DLY_1_DBI_2     0xF4328454
#define DQS_IN_DLY_1_DBI_3     0xF4328458
#define DQS_IN_DLY_3_DBI_0     0xF432845C
#define DQS_IN_DLY_3_DBI_1     0xF4328460
#define DQS_IN_DLY_3_DBI_2     0xF4328464
#define DQS_IN_DLY_3_DBI_3     0xF4328468
#define DPI_CTRL_2             0xF432846C
#define ODT_TTCP1_SET1         0xF4328470
#define MON                    0xF4328474

#define PRBS7_TMUX6            0xF432847C
#define PRBS7_TMUX7            0xF4328480
#define WRLVL_DQS_DATA         0xF4328484
#define PRBS7_TMUX8            0xF4328488
#define READ_CTRL_5            0xF432848C
#define VALID_WIN_DET_PFIFO_0  0xF4328490
#define VALID_WIN_DET_PFIFO_1  0xF4328494
#define VALID_WIN_DET_PFIFO_2  0xF4328498
#define VALID_WIN_DET_PFIFO_3  0xF432849C
#define READ_CTRL_7            0xF43284A0
#define DBI_FIFO_0             0xF43284A4
#define DBI_FIFO_1             0xF43284A8
#define DBI_FIFO_2             0xF43284AC
#define DBI_FIFO_3             0xF43284B0
#define MON_DLL0               0xF43284B4
#define MON_DLL1               0xF43284B8

#define READ_CTRL_6_0          0xF43284C0
#define READ_CTRL_6_1          0xF43284C4
#define READ_CTRL_6_2          0xF43284C8
#define READ_CTRL_6_3          0xF43284CC
#define READ_CTRL_8_0          0xF43284D0
#define READ_CTRL_8_1          0xF43284D4
#define READ_CTRL_8_2          0xF43284D8
#define READ_CTRL_8_3          0xF43284DC
#define READ_CTRL_3_0          0xF43284E0
#define READ_CTRL_3_1          0xF43284E4
#define READ_CTRL_3_2          0xF43284E8
#define READ_CTRL_3_3          0xF43284EC
#define INT_STATUS_4_0         0xF43284F0
#define INT_STATUS_4_1         0xF43284F4
#define INT_STATUS_4_2         0xF43284F8
#define INT_STATUS_4_3         0xF43284FC
#define VALID_WIN_DET_NFIFO_0  0xF4328500
#define VALID_WIN_DET_NFIFO_1  0xF4328504
#define VALID_WIN_DET_NFIFO_2  0xF4328508
#define VALID_WIN_DET_NFIFO_3  0xF432850C
#define READ_CTRL_4_0          0xF4328510
#define READ_CTRL_4_1          0xF4328514
#define READ_CTRL_4_2          0xF4328518
#define READ_CTRL_4_3          0xF432851C
#define VALID_WIN_SEL_ODD      0xF4328520
#define VALID_WIN_PH_ODD       0xF4328524
#define PAD_OE_CTL_EX          0xF4328528
#define READ_CTRL_2_0          0xF432852C
#define READ_CTRL_2_1          0xF4328530
#define READ_CTRL_2_2          0xF4328534
#define READ_CTRL_2_3          0xF4328538

#define VREF_DBG_0_0           0xF4328540
#define VREF_DBG_0_1           0xF4328544
#define VREF_DBG_0_2           0xF4328548
#define VREF_DBG_0_3           0xF432854C
#define VREF_DBG_1_0           0xF4328550
#define VREF_DBG_1_1           0xF4328554
#define VREF_DBG_1_2           0xF4328558
#define VREF_DBG_1_3           0xF432855C
#define VREF_DBG_2_0           0xF4328560
#define VREF_DBG_2_1           0xF4328564
#define VREF_DBG_2_2           0xF4328568
#define VREF_DBG_2_3           0xF432856C
#define VREF_DBG_3_0           0xF4328570
#define VREF_DBG_3_1           0xF4328574
#define VREF_DBG_3_2           0xF4328578
#define VREF_DBG_3_3           0xF432857C
#define VREF_DBG_4_0           0xF4328580
#define VREF_DBG_4_1           0xF4328584
#define VREF_DBG_4_2           0xF4328588
#define VREF_DBG_4_3           0xF432858C

#define   PCTL_CCR    0xF4328600
#define   PCTL_DCR    0xF4328604
#define   PCTL_IOCR   0xF4328608
#define   PCTL_CSR    0xF432860C
#define   PCTL_DRR    0xF4328610
#define   PCTL_TPR0   0xF4328614
#define   PCTL_TPR1   0xF4328618
#define   PCTL_TPR2   0xF432861c
#define   PCTL_TPR3   0xF4328620
#define   PCTL_CDPIN_NDGE  0xF4328624	//user definied
#define   PCTL_CDPIN  0xF4328628	//user definied
#define   PCTL_TDPIN  0xF432862c	//to control all soft reset
#define   PCTL_MRINFO 0xF4328630
#define   PCTL_MR0    0xF4328634
#define   PCTL_MR1    0xF4328638
#define   PCTL_MR2    0xF432863c
#define   PCTL_MR3    0xF4328640
#define   PCTL_MR4    0xF4328644
#define   PCTL_MR5    0xF4328648
#define   PCTL_MR6    0xF432864c
#define   PCTL_MR7    0xF4328650
#define   PCTL_DTR0   0xF4328694
#define   PCTL_DTR1   0xF4328698
#define   PCTL_DTAR   0xF432869c
#define   PCTL_ISR    0xF43286A0
#define   PCTL_IMR    0xF43286A4
#define   PCTL_BCR    0xF43286D0
#define   PCTL_BCT    0xF43286D4
#define   PCTL_BCM    0xF43286D8
#define   PCTL_BST    0xF43286DC
#define   PCTL_BSRAM0 0xF43286E0
#define   PCTL_BSRAM1 0xF43286E4
#define   PCTL_BER    0xF43286E8
#define   PCTL_SVN_ID 0xF43286F4
#define   PCTL_IDR    0xF43286F8
#define   PCTL_ELR    0xF43286FC

/* ============= Register ==================== */

struct ca_dram_priv_data {
	void __iomem *ddr_ctrl_base;
	void __iomem *ddr_parameter_buff;
	unsigned int dqs_in_dly_start_value;
	unsigned int byte_lane;
	u32 calibration_time_gap;
	int last_temp;
	int low_temp_threshold;
	int high_temp_threshold;
};

#endif	/* _CA_DRAM_CTRL_H_ */

