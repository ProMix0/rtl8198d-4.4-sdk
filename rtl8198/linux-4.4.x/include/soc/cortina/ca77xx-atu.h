#ifndef _CA77XX_ATU_H_
#define _CA77XX_ATU_H_

#define ATU_ENTRY_NUM	8

#define ACE_S0_ADDR	0x0200000000
#define ACE_S1_ADDR	0x0400000000
#define ACE_S2_ADDR	0x1000000000

struct atu_attr {
	u64 in;
	u32 size;
	u64 out;
};

#define AXDOMAIN_NON_SHAREABLE   0
#define AXDOMAIN_INNER_SHAREABLE 1
#define AXDOMAIN_OUTER_SHAREABLE 2
#define AXDOMAIN_SYSTEM          3

#define AXBAR_NORMAL_ACCESS_S    0
#define AXBAR_MEMORY             1
#define AXBAR_NORMAL_ACCESS_I    2
#define AXBAR_SYNCHRONIZATION    3

#define ARSNOOP_READNOSNOOP      0x0
#define ARSNOOP_READONCE         0x0
#define ARSNOOP_READSHARED       0x1
#define ARSNOOP_READCLEAN        0x2
#define ARSNOOP_READNOTSHARED    0x3
#define ARSNOOP_READUNIQUE       0x7
#define ARSNOOP_CLEANUNIQUE      0xB
#define ARSNOOP_MAKEUNIQUE       0xC
#define ARSNOOP_CLEANSHARED      0x8
#define ARSNOOP_CLEANINVALID     0x9
#define ARSNOOP_MAKEINVALID      0xD
#define ARSNOOP_BARRIER          0x0
#define ARSNOOP_DVM_COMPLETE     0xE
#define ARSNOOP_DVM_MESSAGE      0xF

#define AWSNOOP_WRITENOSNOOP     0x0
#define AWSNOOP_WRITEUNIQUE      0x0
#define AWSNOOP_WRITELINEUNIQUE  0x1
#define AWSNOOP_WRITECLEAN       0x2
#define AWSNOOP_WRITEBACK        0x3
#define AWSNOOP_EVICT            0x4
#define AWSNOOP_WRITEEVICTA      0x5
#define AWSNOOP_BARRIER          0x0

union user_attr {
	volatile struct {
		u32	axuser	  :  6; /* bits 5:0 */
		u32	axdomain  :  2; /* bits 7:6 */
		u32	axbar	  :  2; /* bits 9:8 */
		u32	axsnoop   :  4; /* bits 13:10 */
		u32	rsrvd	  : 18;
	} bf;
	volatile u32 wrd;
};

#define AXCACHE_BUFFERABLE		0x1
#define AXCACHE_CACHEABLE		0x2
#define AXCACHE_READ_ALLOCATE		0x4
#define AXCACHE_WRITE_ALLOCATE		0x8

#define AXPROT_PRIVILEGED_ACCESS	0x1 /* not set - unprivileged access */
#define AXPROT_NONSECURE_ACCESS		0x2 /* not set - secure access */
#define AXPROT_INSTRUCTION_ACCESS	0x4 /* not set - data access */

#define HPROT_DATA_ACCESS		0x1 /* not set - opcode fetch */
#define HPROT_PRIVILEGED_ACCESS		0x2 /* not set - user access */
#define HPROT_BUFFERABLE		0x4 /* not set - not bufferable */
#define HPROT_CACHEABLE			0x8 /* not set - not cacheable */

struct ace_attr {
	u8	arcache;
	u8	awcache;
	u8	arqos;
	u8	awqos;
	u8	arprot;
	u8	awprot;
	u8	hprot;
	u8	cache_en;
	u8	qos_en;
	u8	prot_en;
	u8	user_en;
	volatile union user_attr	aruser;
	volatile union user_attr	awuser;
	volatile union user_attr	huser;
};

int ca77xx_atu_set(const char *name, int idx, struct atu_attr *atu, int enable);
int ca77xx_atu_get(const char *name, int idx, struct atu_attr *atu,
		   int *enable);
int ca77xx_atu_del(const char *name, int idx);
int ca77xx_ace_set(const char *name, int idx, struct atu_attr *atu,
		   struct ace_attr *ace, int enable);
int ca77xx_ace_get(const char *name, int idx, struct atu_attr *atu,
		   struct ace_attr *ace, int *enable);
int ca77xx_ace_del(const char *name, int idx);

#endif /* _CA77XX_ATU_H_ */
