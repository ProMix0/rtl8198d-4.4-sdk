#ifndef CA827X_ATU_H
#define CA827X_ATU_H

/* MIPS Address Space MACROs */
#include <asm/addrspace.h>

/* 
 *  Matches spreadsheet: saturn_addressing_and_fabric_spreadsheet-v2p2.xlsx
 *  Address: !1337KGO
 *
 * ######################################################################################
 *  Please note values below are not arbitrary and are calculated.
 *  Also, we list of up 8 ATU entries here, it may be that they are not 
 * 
 *  For example, the ATU_0_END is based on the "SIZE" in the ATU
 *  ATU_x_PRE is the the upper address (reg.db - ca82xx_registers.h) to be translated 
 *  ATU_x_MASK is the lower address to be passed during tranlation
 *  ATU_x_POST is the upper address after translation
 * 
 *  ATU_0 is for the Boot ROM  reassignment from 0x4204_0000-->0x1fc0_0000 of size 128 kBytes - this is the start of the ROM
 *  ATU_1 is not used and is a spare, it can be added while processor is running - N/A to macro since this is compile time
 *  ATU_2 is not used and is a spare, it can be added while processor is running - N/A to macro since this is compile time
 *  ATU_3 is for the IO Block  reassignment from 0x5000_0000-->0x1000_0000 of size 64  MBytes
 *  ATU_4 is for the NE Block  reassignment from 0xD000_0000-->0x1400_0000 of size 2   MBytes
 *  ATU_5 is for the CPU mem   reassignment from 0xC000_0000-->0x1420_0000 of size 2   MBytes
 *  ATU_6 is for the EAXI mem  reassignment from 0x6000_0000-->0x1e00_0000 of size 32  MBytes
 *  ATU_7 is for the OTP/CSRAM resssignment from 0x4000_0000-->0x1800_0000 of size 128  MBytes - please note, the EAXI has priority 
 *  	 and will overlap.  The large size is to make the ATU work on a power of 2
 */

/* the translations below need to be kept in sync with ATU_PRE mappings in ca82xx_registers.h 
 * and ATU_POST mappings set in the HW spreadseet reference above 
 */
 
#define ATU_0_PRE 	0x42040000
#define ATU_0_MASK	0x0001FFFF
#define ATU_0_POST	0x12040000

#define ATU_1_PRE 	0xfffffff8
#define ATU_1_MASK	0x00000007
#define ATU_1_POST	0xfffffff8

#define ATU_2_PRE	0xfffffff8
#define ATU_2_MASK	0x00000007
#define ATU_2_POST	0xfffffff8

#define ATU_3_PRE	0x50000000
#define ATU_3_MASK	0x03ffffff
#define ATU_3_POST	0x10000000

#define ATU_4_PRE	0xd0000000
#define ATU_4_MASK	0x001fffff
#define ATU_4_POST 	0x14000000

#define ATU_5_PRE	0xc0000000
#define ATU_5_MASK	0x001fffff
#define ATU_5_POST	0x14200000

#define ATU_6_PRE	0x60000000
#define ATU_6_MASK	0x01ffffff
#define ATU_6_POST 	0x1e000000

#define ATU_7_PRE	0x40000000
#define ATU_7_MASK	0x07ffffff
#define ATU_7_POST	0x18000000

#if defined(__ASSEMBLY__) && !defined(__LINKER__)
    .macro atu_xlat_space _space, _reg, _addr
        .if ((\_addr & ~ATU_0_MASK) == ATU_0_PRE)
           li \_reg, ((\_addr & ATU_0_MASK) | ATU_0_POST | \_space)
        .else
          .if ((\_addr & ~ATU_1_MASK) == ATU_1_PRE)
             li \_reg, ((\_addr & ATU_1_MASK) | ATU_1_POST | \_space)
          .else
            .if ((\_addr & ~ATU_2_MASK) == ATU_2_PRE)
               li \_reg, ((\_addr & ATU_2_MASK) | ATU_2_POST | \_space)
            .else
              .if ((\_addr & ~ATU_3_MASK) == ATU_3_PRE)
                 li \_reg, ((\_addr & ATU_3_MASK) | ATU_3_POST | \_space)
              .else
                .if ((\_addr & ~ATU_4_MASK) == ATU_4_PRE)
                   li \_reg, ((\_addr & ATU_4_MASK) | ATU_4_POST | \_space)
                .else
                  .if ((\_addr & ~ATU_5_MASK) == ATU_5_PRE)
                     li \_reg, ((\_addr & ATU_5_MASK) | ATU_5_POST | \_space)
                  .else
                    .if ((\_addr & ~ATU_6_MASK) == ATU_6_PRE)
                       li \_reg, ((\_addr & ATU_6_MASK) | ATU_6_POST | \_space)
                    .else
                      .if ((\_addr & ~ATU_7_MASK) == ATU_7_PRE)
                         li \_reg, ((\_addr & ATU_7_MASK) | ATU_7_POST | \_space)
                      .endif
                    .endif  
                  .endif
                .endif
              .endif
            .endif
          .endif
        .endif
    .endm

    .macro atu_xlat _reg, _addr
	atu_xlat_space 0, \_reg, \_addr
    .endm
    .macro phys_atu_xlat _reg, _addr
	atu_xlat_space 0, \_reg, \_addr
    .endm
    .macro kseg0_atu_xlat _reg, _addr
	atu_xlat_space KSEG0, \_reg, \_addr
    .endm
    .macro kseg1_atu_xlat _reg, _addr
	atu_xlat_space KSEG1, \_reg, \_addr
    .endm

#else /* !  __ASSEMBLY__ */
#define ATU_MATCH0(X) ((X & ~ATU_0_MASK) == ATU_0_PRE) ? ((X & ATU_0_MASK) | ATU_0_POST) : 
#define ATU_MATCH1(X) ((X & ~ATU_1_MASK) == ATU_1_PRE) ? ((X & ATU_1_MASK) | ATU_1_POST) : 
#define ATU_MATCH2(X) ((X & ~ATU_2_MASK) == ATU_2_PRE) ? ((X & ATU_2_MASK) | ATU_2_POST) : 
#define ATU_MATCH3(X) ((X & ~ATU_3_MASK) == ATU_3_PRE) ? ((X & ATU_3_MASK) | ATU_3_POST) : 
#define ATU_MATCH4(X) ((X & ~ATU_4_MASK) == ATU_4_PRE) ? ((X & ATU_4_MASK) | ATU_4_POST) : 
#define ATU_MATCH5(X) ((X & ~ATU_5_MASK) == ATU_5_PRE) ? ((X & ATU_5_MASK) | ATU_5_POST) : 
#define ATU_MATCH6(X) ((X & ~ATU_6_MASK) == ATU_6_PRE) ? ((X & ATU_6_MASK) | ATU_6_POST) : 
#define ATU_MATCH7(X) ((X & ~ATU_7_MASK) == ATU_7_PRE) ? ((X & ATU_7_MASK) | ATU_7_POST) : X

#define ATU_XLAT(X) (ATU_MATCH0(X) ATU_MATCH1(X) ATU_MATCH2(X) ATU_MATCH3(X) ATU_MATCH4(X) ATU_MATCH5(X) ATU_MATCH6(X) ATU_MATCH7(X))

/* MIPS Cached access */
#define KSEG0_ATU_XLAT(x) KSEG0ADDR(ATU_XLAT(x))

/* MIPS Uncached access */
#define KSEG1_ATU_XLAT(x) KSEG1ADDR(ATU_XLAT(x))

/* Physical address to be used for DMA references */
#define PHYS_ATU_XLAT(x) ATU_XLAT(x)
#endif /* !  __ASSEMBLY__ */ 

#endif /* CA827X_ATU_H */
