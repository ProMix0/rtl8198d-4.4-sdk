#include <generated/ca_ne_autoconf.h>
#include <linux/spinlock.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <net/ipv6.h>
#include "ca_types.h"
#include "aal_common.h"
#include "aal_table.h"
#include "aal_l3_pe.h"
#include "aal_l3fe.h"
#include "osal_common.h"

#define TABLE_TRY_TIMEOUT	1000

static aal_table_t l3pe_sixrd_tbl;
static aal_table_t l3pe_dsl_tbl;

/***************************************************************************
 * PPPoE encapsulation
 ***************************************************************************/

static DEFINE_SPINLOCK(l3pe_pppoe_prot_lock);

/* Set the PPPoE encapsulation: code, version, type.
 *   ver_type[7:4]: version
 *   ver_type[3:0]: type
 */
int aal_l3pe_pppoe_cfg_set(unsigned char code, unsigned char ver_type)
{
	L3FE_PE_PPPOE_CFG_t val;

	val.bf.pppoe_code = code;
	val.bf.sess_ver_type = ver_type;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PPPOE_CFG);

	return AAL_E_OK;
}

/* Get the PPPoE encapsulation: code, version, type.
 *   ver_type[7:4]: version
 *   ver_type[3:0]: type
 */
int aal_l3pe_pppoe_cfg_get(unsigned char *code, unsigned char *ver_type)
{
	L3FE_PE_PPPOE_CFG_t val;

	if ((code == NULL) || (ver_type == NULL)) return AAL_E_NULLPTR;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PPPOE_CFG);
	*code = val.bf.pppoe_code;
	*ver_type = val.bf.sess_ver_type;

	return AAL_E_OK;
}

/* Set the IPv4 protocol for PPPoE encapsulation */
int aal_l3pe_pppoe_ipv4_prot_set(unsigned short prot)
{
	L3FE_PE_PPPOE_PROT_CFG_t val;

	spin_lock(&l3pe_pppoe_prot_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_PPPOE_PROT_CFG);
	val.bf.ipv4_protocol = prot;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PPPOE_PROT_CFG);

	spin_unlock(&l3pe_pppoe_prot_lock);

	return AAL_E_OK;
}

/* Get the IPv4 protocol for PPPoE encapsulation */
int aal_l3pe_pppoe_ipv4_prot_get(unsigned short *prot)
{
	L3FE_PE_PPPOE_PROT_CFG_t val;

	if (prot == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_pppoe_prot_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_PPPOE_PROT_CFG);
	*prot = val.bf.ipv4_protocol;

	spin_unlock(&l3pe_pppoe_prot_lock);

	return AAL_E_OK;
}

/* Set the IPv6 protocol for PPPoE encapsulation */
int aal_l3pe_pppoe_ipv6_prot_set(unsigned short prot)
{
	L3FE_PE_PPPOE_PROT_CFG_t val;

	spin_lock(&l3pe_pppoe_prot_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_PPPOE_PROT_CFG);
	val.bf.ipv6_protocol = prot;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PPPOE_PROT_CFG);

	spin_unlock(&l3pe_pppoe_prot_lock);

	return AAL_E_OK;
}

/* Get the IPv6 protocol for PPPoE encapsulation */
int aal_l3pe_pppoe_ipv6_prot_get(unsigned short *prot)
{
	L3FE_PE_PPPOE_PROT_CFG_t val;

	if (prot == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_pppoe_prot_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_PPPOE_PROT_CFG);
	*prot = val.bf.ipv6_protocol;

	spin_unlock(&l3pe_pppoe_prot_lock);

	return AAL_E_OK;
}

/***************************************************************************
 * PE configuration
 ***************************************************************************/

int aal_l3pe_dsl_tbl_clear(void)
{
	L3FE_PE_DSL_CLR_t val;

	val.bf.clear = 1;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_CLR);

	return AAL_E_OK;
}

int aal_l3pe_6rd_tbl_clear(void)
{
	L3FE_PE_XRD_CLR_t val;

	val.bf.clear = 1;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_XRD_CLR);

	return AAL_E_OK;
}

int aal_l3pe_pe_stomp_crc_set(bool mode)
{
	L3FE_PE_PE_CFG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	val.bf.stomp_crc = (mode) ? 1 : 0;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PE_CFG);

	return AAL_E_OK;
}

int aal_l3pe_pe_stomp_crc_get(bool *mode)
{
	L3FE_PE_PE_CFG_t val;

	if (mode == NULL) return AAL_E_NULLPTR;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	*mode = (val.bf.stomp_crc == 1) ? true : false;

	return AAL_E_OK;
}

int aal_l3pe_pe_bypass_crc_set(bool mode)
{
	L3FE_PE_PE_CFG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	val.bf.bypass_crc_update = (mode) ? 1 : 0;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PE_CFG);

	return AAL_E_OK;
}

int aal_l3pe_pe_bypass_crc_get(bool *mode)
{
	L3FE_PE_PE_CFG_t val;

	if (mode == NULL) return AAL_E_NULLPTR;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	*mode = (val.bf.bypass_crc_update == 1) ? true : false;

	return AAL_E_OK;
}

int aal_l3pe_pe_ignore_tqm_fe_rdy_set(bool mode)
{
	L3FE_PE_PE_CFG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	val.bf.ignore_tqm_fe_rdy = (mode) ? 1 : 0;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PE_CFG);

	return AAL_E_OK;
}

int aal_l3pe_pe_ignore_tqm_fe_rdy_get(bool *mode)
{
	L3FE_PE_PE_CFG_t val;

	if (mode == NULL) return AAL_E_NULLPTR;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	*mode = (val.bf.ignore_tqm_fe_rdy == 1) ? true : false;

	return AAL_E_OK;
}

int aal_l3pe_pe_gemid_map_get(bool *mode)
{
	L3FE_PE_PE_CFG_t val;

	if (mode == NULL) return AAL_E_NULLPTR;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	*mode = (val.bf.gemid_map == 1) ? true : false;

	return AAL_E_OK;
}

int aal_l3pe_pe_gemid_map_set(bool mode)
{
	L3FE_PE_PE_CFG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	val.bf.gemid_map = (mode) ? 1 : 0;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PE_CFG);

	return AAL_E_OK;
}

int aal_l3pe_pe_ldpid_base_get(int *mode)
{
	L3FE_PE_PE_CFG_t val;

	if (mode == NULL) return AAL_E_NULLPTR;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	*mode = val.bf.ldpid_base;

	return AAL_E_OK;
}

int aal_l3pe_pe_ldpid_base_set(unsigned int mode)
{
	L3FE_PE_PE_CFG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_PE_PE_CFG);
	val.bf.ldpid_base = mode;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_PE_CFG);

	return AAL_E_OK;
}

/***************************************************************************
 * PE Port Egress Behavior idx 0~3
 ***************************************************************************/

int aal_l3pe_port_egress_behavior_set(ca_device_id_t device_id, unsigned int idx, l3pe_port_egress_behavior_t *entry)
{
	unsigned int *ptr;
	int i;

	if (idx > 3)
		return AAL_E_OUTRANGE;

	if (entry == NULL)
		return AAL_E_NULLPTR;

	ptr = (unsigned int *)entry;

	for (i = 0; i < 2 ; i++)
		CA_NE_REG_WRITE(ptr[i], (L3FE_PE_PORT_EGRESS_BEHAVIOR_0_0 + idx * 12) + (i << 2));

	return AAL_E_OK;
}

int aal_l3pe_port_egress_behavior_get(ca_device_id_t device_id, unsigned int idx, l3pe_port_egress_behavior_t *entry)
{
	unsigned int *ptr;
	int i;

	if (idx > 3)
		return AAL_E_OUTRANGE;

	if (entry == NULL)
		return AAL_E_NULLPTR;

	ptr = (unsigned int *)entry;

	for (i = 0; i < 2; i++)
		ptr[i] = CA_NE_REG_READ((L3FE_PE_PORT_EGRESS_BEHAVIOR_0_0 + idx * 12) + (i << 2));

	return AAL_E_OK;
}

int aal_l3pe_port_egress_behavior_init(void)
{
	l3pe_port_egress_behavior_t entry;

	/* behavior dedicated for MC packet */
	entry.ldpid = AAL_LPORT_L3_LAN;
	entry.bypass_l2_edit = 0;
	entry.bypass_l3_edit = 0;
	entry.bypass_l4_edit = 0;
	entry.bypass_l3_encap = 0;
	entry.bypass_l3_decap = 0;
	entry.bypass_padding = 0;
	entry.mtu_check = 1;
	entry.redirect_ldpid = AAL_LPORT_CPU_1;
	entry.redirect_cos = 0;
	entry.cpu_cmd = 1; // HEADER_CPU_T content. 0: nothing, 1: mdata, 2: partial hdr_i info
	aal_l3pe_port_egress_behavior_set(0, 0, &entry);

	/* behavior for UC packet */
	entry.ldpid = AAL_LPORT_ETH_WAN;
	entry.bypass_l2_edit = 0;
	entry.bypass_l3_edit = 0;
	entry.bypass_l4_edit = 0;
	entry.bypass_l3_encap = 0;
	entry.bypass_l3_decap = 0;
	entry.bypass_padding = 0;
	entry.mtu_check = 1;
	entry.redirect_ldpid = AAL_LPORT_CPU_0;
	entry.redirect_cos = 0;
	entry.cpu_cmd = 1;
	aal_l3pe_port_egress_behavior_set(0, 1, &entry);

	/* behavior UC packet */
	entry.ldpid = AAL_LPORT_L3_LAN;
	entry.bypass_l2_edit = 0;
	entry.bypass_l3_edit = 0;
	entry.bypass_l4_edit = 0;
	entry.bypass_l3_encap = 0;
	entry.bypass_l3_decap = 0;
	entry.bypass_padding = 0;
	entry.mtu_check = 1;
	entry.redirect_ldpid = AAL_LPORT_CPU_1;
	entry.redirect_cos = 0;
	entry.cpu_cmd = 1;
	aal_l3pe_port_egress_behavior_set(0, 2, &entry);

	/* default behavior; ldpid is meaningless */
	entry.ldpid = 0;
	entry.bypass_l2_edit = 0;
	entry.bypass_l3_edit = 0;
	entry.bypass_l4_edit = 0;
	entry.bypass_l3_encap = 0;
	entry.bypass_l3_decap = 0;
	entry.bypass_padding = 0;
	entry.mtu_check = 0;
	entry.redirect_ldpid = AAL_LPORT_CPU_0;
	entry.redirect_cos = 0;
	entry.cpu_cmd = 1;
	aal_l3pe_port_egress_behavior_set(0, 3, &entry);

	return 0;
}

/***************************************************************************
 * MTU
 ***************************************************************************/

/* Set MTU size for MTU size encoding.
 * The HW reg can store 4 MTUs. HDR_I.ip_mtu_enc[1:0] decides to use which MTU.
 *   idx: MTU index to be set.
 *   mtu: MTU size, 14 bits.
 */
int aal_l3pe_mtu_size_set(unsigned int idx, unsigned int mtu)
{
	L3FE_PE_MTU_SIZE_0_t val;

	if (idx >= 4) return AAL_E_OUTRANGE;

	switch (idx) {
	case 0:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_0);
		val.bf.mtu_0 = mtu;
		CA_NE_REG_WRITE(val.wrd, L3FE_PE_MTU_SIZE_0);
		break;

	case 1:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_0);
		val.bf.mtu_1 = mtu;
		CA_NE_REG_WRITE(val.wrd, L3FE_PE_MTU_SIZE_0);
		break;

	case 2:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_1);
		val.bf.mtu_0 = mtu;
		CA_NE_REG_WRITE(val.wrd, L3FE_PE_MTU_SIZE_1);
		break;

	case 3:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_1);
		val.bf.mtu_1 = mtu;
		CA_NE_REG_WRITE(val.wrd, L3FE_PE_MTU_SIZE_1);
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}

/* Get MTU size for MTU size encoding.
 * The HW reg can store 4 MTUs. HDR_I.ip_mtu_enc[1:0] decides to use which MTU.
 *   idx: MTU index to be set.
 *   mtu: MTU size, 14 bits.
 */
int aal_l3pe_mtu_size_get(unsigned int idx, unsigned int *mtu)
{
	L3FE_PE_MTU_SIZE_0_t val;

	if (idx >= 4) return AAL_E_OUTRANGE;
	if (mtu == NULL) return AAL_E_NULLPTR;

	switch (idx) {
	case 0:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_0);
		*mtu = val.bf.mtu_0;
		break;

	case 1:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_0);
		*mtu = val.bf.mtu_1;
		break;

	case 2:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_1);
		*mtu = val.bf.mtu_0;
		break;

	case 3:
		val.wrd = CA_NE_REG_READ(L3FE_PE_MTU_SIZE_1);
		*mtu = val.bf.mtu_1;
		break;

	default:
		return AAL_E_OUTRANGE;
	}

	return AAL_E_OK;
}

#define NUM_MTU_ENTRY	4
#define MAX_MTU_SIZE	0x3FFF

struct mtu_mgnt {
	unsigned int mtu_size;
	unsigned int ref_cnt;
};

static struct mtu_mgnt mtu_table[NUM_MTU_ENTRY];
static ca_uint aal_mtu_lock;

static int aal_l3pe_mtu_size_mgnt(void)
{
	int i;

	ca_spin_lock_init(&aal_mtu_lock, SPINLOCK_CMN);

	for (i = 0; i < NUM_MTU_ENTRY; i++) {
		mtu_table[i].mtu_size = 0;
		mtu_table[i].ref_cnt = 0;

		aal_l3pe_mtu_size_set(i, mtu_table[i].mtu_size);
	}

	return 0;
}

int aal_l3pe_mtu_size_add(unsigned int mtu, unsigned int *idx)
{
	int i, ret = AAL_E_TBLFULL;

	if (mtu > MAX_MTU_SIZE)
		return AAL_E_INVALID;

	ca_spin_lock(aal_mtu_lock);

	for (i = 0; i < NUM_MTU_ENTRY; i++) {
		if (mtu_table[i].mtu_size == mtu) {
			mtu_table[i].ref_cnt++;
			*idx = i;
			ret = AAL_E_OK;
			break;
		} else if ((mtu_table[i].mtu_size != mtu) &&
		           (mtu_table[i].ref_cnt == 0)) {
			mtu_table[i].mtu_size = mtu;
			mtu_table[i].ref_cnt++;
			aal_l3pe_mtu_size_set(i, mtu);
			*idx = i;
			ret = AAL_E_OK;
			break;
		}
	}

	ca_spin_unlock(aal_mtu_lock);

	return ret;
}

int aal_l3pe_mtu_size_del(unsigned int idx)
{
	int ret;

	ca_spin_lock(aal_mtu_lock);

	if (mtu_table[idx].ref_cnt > 0) {
		mtu_table[idx].ref_cnt--;

		if (mtu_table[idx].ref_cnt == 0) {
			aal_l3pe_mtu_size_set(idx, 0);
			mtu_table[idx].mtu_size = 0;
		}

		ret = AAL_E_OK;
	} else {
		ret = AAL_E_ENTRYNOTFOUND;
	}

	ca_spin_unlock(aal_mtu_lock);

	return ret;
}

int aal_l3pe_mtu_size_lookup(unsigned int mtu, unsigned int *idx)
{
	int ret = AAL_E_ENTRYNOTFOUND;
	int i;

	if (idx == NULL)
		return AAL_E_NULLPTR;

	ca_spin_lock(aal_mtu_lock);
	for (i = 0; i < NUM_MTU_ENTRY; i++) {
		if ((mtu_table[i].mtu_size == mtu) && (mtu_table[i].ref_cnt > 0)) {
			*idx = i;
			ret = AAL_E_OK;
			break;
		}
	}
	ca_spin_unlock(aal_mtu_lock);

	return ret;
}

/***************************************************************************
 * Mirror
 ***************************************************************************/

/* Set for Mirror (CPU COPY FIFO)
 *   ldpid: set to HDR_A.ldpid
 *   cos: set to HDR_A.cos
 *   fifo_high_thold: high threshold of CPU COPY FIFO
 *   fifo_low_thold: low threshold of CPU COPY FIFO
 */
int aal_l3pe_mirror_set(unsigned int ldpid, unsigned int cos, unsigned int fifo_high_thold, unsigned int fifo_low_thold)
{
	L3FE_PE_MIRROR_CFG_t val;

	val.bf.mirr_ldpid = ldpid;
	val.bf.mirr_cos = cos;
	val.bf.mirr_fifo_high_threshold = fifo_high_thold;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_MIRROR_CFG);

	return AAL_E_OK;
}

/* Set mirror length, max 11 bits */
int aal_l3pe_mirror_len_set(unsigned int len)
{
	L3FE_PE_MIRROR_LEN_CFG_t val;

	val.bf.mirr_len = len;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_MIRROR_LEN_CFG);

	return AAL_E_OK;
}

/***************************************************************************
 * Config - 6RD and DS-Lite
 ***************************************************************************/

static DEFINE_SPINLOCK(l3pe_dsl_6rd_encap_cfg_lock);

/* Set the IPv4 DF bit for 6RD encapsulation */
int aal_l3pe_6rd_df_set(bool df)
{
	L3FE_PE_DSL_6RD_ENCAP_CFG_t val;

	spin_lock(&l3pe_dsl_6rd_encap_cfg_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_CFG);
	val.bf.df_6rd_encap = df ? 1 : 0;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_ENCAP_CFG);

	spin_unlock(&l3pe_dsl_6rd_encap_cfg_lock);

	return AAL_E_OK;
}

/* Get the IPv4 DF bit for 6RD encapsulation */
int aal_l3pe_6rd_df_get(bool *df)
{
	L3FE_PE_DSL_6RD_ENCAP_CFG_t val;

	if (df == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_encap_cfg_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_CFG);
	*df = (val.bf.df_6rd_encap == 1) ? true : false;

	spin_unlock(&l3pe_dsl_6rd_encap_cfg_lock);

	return AAL_E_OK;
}

/* Set the IPv4 protocol field for 6RD encapsulation */
int aal_l3pe_6rd_encap_nh_set(unsigned char nh)
{
	L3FE_PE_DSL_6RD_ENCAP_CFG_t val;

	spin_lock(&l3pe_dsl_6rd_encap_cfg_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_CFG);
	val.bf.prot_6rd_encap = nh;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_ENCAP_CFG);

	spin_unlock(&l3pe_dsl_6rd_encap_cfg_lock);

	return AAL_E_OK;
}

/* Get the IPv4 protocol field for 6RD encapsulation */
int aal_l3pe_6rd_encap_nh_get(unsigned char *nh)
{
	L3FE_PE_DSL_6RD_ENCAP_CFG_t val;

	if (nh == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_encap_cfg_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_CFG);
	*nh = val.bf.prot_6rd_encap;

	spin_unlock(&l3pe_dsl_6rd_encap_cfg_lock);

	return AAL_E_OK;
}

/* Set the IPv6 NextHeader type for DS-Lite encapsulation */
int aal_l3pe_dsl_encap_nh_set(unsigned char nh)
{
	L3FE_PE_DSL_6RD_ENCAP_CFG_t val;

	spin_lock(&l3pe_dsl_6rd_encap_cfg_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_CFG);
	val.bf.nh_dsl_encap = nh;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_ENCAP_CFG);

	spin_unlock(&l3pe_dsl_6rd_encap_cfg_lock);

	return AAL_E_OK;
}

/* Get the IPv6 NextHeader type for DS-Lite encapsulation */
int aal_l3pe_dsl_encap_nh_get(unsigned char *nh)
{
	L3FE_PE_DSL_6RD_ENCAP_CFG_t val;

	if (nh == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_encap_cfg_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_CFG);
	*nh = val.bf.nh_dsl_encap;

	spin_unlock(&l3pe_dsl_6rd_encap_cfg_lock);

	return AAL_E_OK;
}

/***************************************************************************
 * EtherType for Encapsulation - 6RD and DS-Lite
 ***************************************************************************/

static DEFINE_SPINLOCK(l3pe_dsl_6rd_encap_ethertype_lock);

/* Set EtherType field for 6RD encapsulation */
int aal_l3pe_6rd_encap_ethertype_set(uint16_t ethertype)
{
	L3FE_PE_DSL_6RD_ENCAP_TYPE_t val;

	spin_lock(&l3pe_dsl_6rd_encap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_TYPE);
	val.bf.etype_6rd_encap = ethertype;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_ENCAP_TYPE);

	spin_unlock(&l3pe_dsl_6rd_encap_ethertype_lock);

	return AAL_E_OK;
}

/* Get EtherType field for 6RD encapsulation */
int aal_l3pe_6rd_encap_ethertype_get(uint16_t *ethertype)
{
	L3FE_PE_DSL_6RD_ENCAP_TYPE_t val;

	if (ethertype == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_encap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_TYPE);
	*ethertype = val.bf.etype_6rd_encap;

	spin_unlock(&l3pe_dsl_6rd_encap_ethertype_lock);

	return AAL_E_OK;
}

/* Set EtherType field for DS-Lite encapsulation */
int aal_l3pe_dsl_encap_ethertype_set(uint16_t ethertype)
{
	L3FE_PE_DSL_6RD_ENCAP_TYPE_t val;

	spin_lock(&l3pe_dsl_6rd_encap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_TYPE);
	val.bf.etype_dsl_encap = ethertype;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_ENCAP_TYPE);

	spin_unlock(&l3pe_dsl_6rd_encap_ethertype_lock);

	return AAL_E_OK;
}

/* Get EtherType field for DS-Lite encapsulation */
int aal_l3pe_dsl_encap_ethertype_get(uint16_t *ethertype)
{
	L3FE_PE_DSL_6RD_ENCAP_TYPE_t val;

	if (ethertype == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_encap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_ENCAP_TYPE);
	*ethertype = val.bf.etype_dsl_encap;

	spin_unlock(&l3pe_dsl_6rd_encap_ethertype_lock);

	return AAL_E_OK;
}

/***************************************************************************
 * EtherType for Decapsulation - 6RD and DS-Lite
 ***************************************************************************/

static DEFINE_SPINLOCK(l3pe_dsl_6rd_decap_ethertype_lock);

/* Set EtherType field for 6RD decapsulation */
int aal_l3pe_6rd_decap_ethertype_set(uint16_t ethertype)
{
	L3FE_PE_DSL_6RD_DECAP_TYPE_t val;

	spin_lock(&l3pe_dsl_6rd_decap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_DECAP_TYPE);
	val.bf.etype_6rd_decap = ethertype;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_DECAP_TYPE);

	spin_unlock(&l3pe_dsl_6rd_decap_ethertype_lock);

	return AAL_E_OK;
}

/* Get EtherType field for 6RD decapsulation */
int aal_l3pe_6rd_decap_ethertype_get(uint16_t *ethertype)
{
	L3FE_PE_DSL_6RD_DECAP_TYPE_t val;

	if (ethertype == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_decap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_DECAP_TYPE);
	*ethertype = val.bf.etype_6rd_decap;

	spin_unlock(&l3pe_dsl_6rd_decap_ethertype_lock);

	return AAL_E_OK;
}

/* Set EtherType field for DS-Lite decapsulation */
int aal_l3pe_dsl_decap_ethertype_set(uint16_t ethertype)
{
	L3FE_PE_DSL_6RD_DECAP_TYPE_t val;

	spin_lock(&l3pe_dsl_6rd_decap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_DECAP_TYPE);
	val.bf.etype_dsl_decap = ethertype;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DSL_6RD_DECAP_TYPE);

	spin_unlock(&l3pe_dsl_6rd_decap_ethertype_lock);

	return AAL_E_OK;
}

/* Get EtherType field for DS-Lite decapsulation */
int aal_l3pe_dsl_decap_ethertype_get(uint16_t *ethertype)
{
	L3FE_PE_DSL_6RD_DECAP_TYPE_t val;

	if (ethertype == NULL) return AAL_E_NULLPTR;

	spin_lock(&l3pe_dsl_6rd_decap_ethertype_lock);

	val.wrd = CA_NE_REG_READ(L3FE_PE_DSL_6RD_DECAP_TYPE);
	*ethertype = val.bf.etype_dsl_decap;

	spin_unlock(&l3pe_dsl_6rd_decap_ethertype_lock);

	return AAL_E_OK;
}

/***************************************************************************
 * 6RD
 ***************************************************************************/

/* The checks for ingress 6RD packet:
 * 1. IF ID check
 *    - When FIB.pop_l3_en is 1, ingress IF_ID[7:5] must be 3'b001.
 *    - If it fails, result depends on REG.intf_chk_fail_fwd_ctl.
 * 2. Delegated prefix consistency check
 *    - IPv6_prefix == IPv6[127 : 128 - v6_prfx_len]
 *    - IPv4_suffix == IPv6[127 - v6_prfx_len : 128 - v6_prfx_len - (32 - v4_msk_len)]
 *    - If it fails, result depends on REG.sixrd_decap_chk_fail_fwd_ctl.
 * 3. Reverse path check
 *    - Outer IPv4SA/IPv4DA must match br_v4_ip/ce_v4_ip of 6rd_entry.
 *    - Only for MC packet (inner IP is MC) or 6rd_entry.br_only is 1.
 *      CE-to-CE packet must not match the 6RD entry.
 *    - If it fails, result depends on REG.sixrd_decap_chk_fail_fwd_ctl.
 * 4. ECN check
 *    - Check when FIB.pop_l3_chk_ecn_en is 1.
 *    - If outer ECN is set but inner ECN is not, check fails.
 *    - If it fails, result depends on REG.ecn_chk_fail_fwd_ctl.
 *
 * When 6rd.ingr_chk_en is 1:
 * 1. IPv6DA delegated prefix consistency check
 *    - This filters the packet which destination not belonging to local site.
 *    - IPv6 part of IPv6DA delecated prefix matches 6rd_entry.v6_prfx
 *    - IPv4 part of IPv6DA delecated prefix matches suffix of 6rd_entry.ce_ip
 *    - If check passes, go to step 2.
 * 2. IPv6SA prefix check
 *    - This identifies the packet is CE-to-CE.
 *    - IPv6 part of IPv6SA delecated prefix matches 6rd_entry.v6_prfx
 *      If true, packet is from CE.
 *      If false. packet might be from BR. Goto step 3.
 * 3. Reverse path check
 *    - This filters the packet which is not from BR.
 *    - IPv4SA == 6rd_entry.br_v4_ip
 *    - IPv4DA == 6rd_entry.ce_v4_ip
 * 4. IPSA address consistency check if 6rd_entry.ipsa_mtch is 1
 *    - IPv4 part of IPv6SA delecated prefix matches suffix of 6rd_entry.br_ip
 */

int l3pe_sixrd_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_add(&l3pe_sixrd_tbl, entry, rslt_idx);
}

int l3pe_sixrd_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	return generic_entry_add_by_idx(&l3pe_sixrd_tbl, entry, idx);
}

int l3pe_sixrd_tbl_entry_del(unsigned int idx)
{
	return generic_entry_del(&l3pe_sixrd_tbl, idx);
}

int l3pe_sixrd_tbl_entry_set(unsigned int idx, void *entry)
{
	return generic_entry_set(&l3pe_sixrd_tbl, idx, entry);
}

int l3pe_sixrd_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&l3pe_sixrd_tbl, entry, rslt_idx);
}

int l3pe_sixrd_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&l3pe_sixrd_tbl, idx, entry);
}

int l3pe_sixrd_tbl_table_flush(void)
{
	return generic_table_flush(&l3pe_sixrd_tbl);
}

int l3pe_sixrd_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&l3pe_sixrd_tbl);
}

void l3pe_sixrd_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &l3pe_sixrd_tbl;
	l3pe_sixrd_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_fwd_ctrl_t sixrd_ipmc_fwd_ctrl;
	l3fe_stg1_sixrd_ipmc_new_mac_da_t sixrd_ipmc_new_mac_da;
	struct in_addr ce_ip, br_ip;
	struct in6_addr ce_ip6, br_ip6, prfx_ip6;
	uint64_t tmp_u64;
	uint32_t *itmp_u32 = (uint32_t *) &tmp_u64;

	uint32_t tmp_mac;
	uint8_t *cp = (uint8_t *) &tmp_mac;

	if (start_idx >= L3PE_SIXRD_TBL_ENTRY_MAX) {
		printk(KERN_ERR "%s: Out of range!\n", __func__);
		return;
	}

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_ipnip_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get(&sixrd_ipmc_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_sixrd_ipmc_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get(&sixrd_ipmc_new_mac_da)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_sixrd_ipmc_new_mac_da_get() returns %d.\n", ret);
	tmp_mac = (uint32_t) sixrd_ipmc_new_mac_da.bf.addr_prfx;

	if ((count < 0) || (start_idx + count > L3PE_SIXRD_TBL_ENTRY_MAX)) {
		end_idx = L3PE_SIXRD_TBL_ENTRY_MAX;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== 6RD Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = l3pe_sixrd_tbl_entry_get_by_idx(i, &entry);
		} else {
			ret = hw_entry_get(&l3pe_sixrd_tbl, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %01d, ingr_chk_en(1) %01d, br_only(1) %01d\n", entry.valid, entry.ingr_chk_en, entry.br_only);
			printk(KERN_INFO "ipsa_mtch(1) %01d, id_start(16) 0x%04x, id_end(16) 0x%04x\n", entry.ipsa_mtch, entry.id_start, entry.id_end);
			printk(KERN_INFO "ttl_en(1) %01d, ttl(8) 0x%02x, tos_en(1) %01d, tos(8) 0x%02x\n", entry.ttl_en, entry.ttl, entry.tos_en, entry.tos);
			printk(KERN_INFO "br_v4_ip(32) 0x%08x, v6_prfx(64) 0x%016llx, v6_prfx_len(6) %d\n", entry.br_v4_ip, entry.v6_prfx, entry.v6_prfx_len);
			printk(KERN_INFO "v4_msk_len(5) %d, ce_v4_ip(32) 0x%08x\n", entry.v4_msk_len, entry.ce_v4_ip);

			if (entry.valid == 0) continue;
			printk(KERN_INFO "\nMatched 6RD packet:\n");

			ce_ip.s_addr = htonl(entry.ce_v4_ip);
			br_ip.s_addr = htonl(entry.br_v4_ip);
			prfx_ip6.in6_u.u6_addr32[0] = htonl(entry.v6_prfx >> 32);
			prfx_ip6.in6_u.u6_addr32[1] = htonl(entry.v6_prfx & 0xffffffff);
			prfx_ip6.in6_u.u6_addr32[2] = 0;
			prfx_ip6.in6_u.u6_addr32[3] = 0;

			if (entry.ingr_chk_en == 0) {
				/* almost all 6RD packet can be decapsulated */

				/* decap as UC */
				printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
				printk(KERN_INFO "  outer IPv4_DA = any Unicast address\n");
				printk(KERN_INFO "  inner IPv6_SA = any Unicast address\n");
				printk(KERN_INFO "  inner IPv6_DA = any Unicast address\n");

				/* decap as MC */
				printk(KERN_INFO "or\n");
				printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
				printk(KERN_INFO "  outer IPv4_DA = any Unicast address\n");
				printk(KERN_INFO "  inner IPv6_SA = any Unicast address\n");
				printk(KERN_INFO "  inner IPv6_DA = any Multicast address\n");
				if (ipnip_ctrl.bf.sixrd_ipmc_new_mac_en == 0)
					printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
				else
					printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-00-00-00-00 / 16.\n", *(cp + 1), *cp);

				if (ipnip_ctrl.bf.sixrd_ipmc_ilg_chk_en == 0) {
					/* if outer MC and inner UC, HW treats it as UC, decap as UC */
					/* decap as MC */
					printk(KERN_INFO "or\n");
					printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
					printk(KERN_INFO "  outer IPv4_DA = any Multicast address\n");
					printk(KERN_INFO "  inner IPv6_SA = any Unicast address\n");
					printk(KERN_INFO "  inner IPv6_DA = any Multicast address\n");
					if (ipnip_ctrl.bf.sixrd_ipmc_new_mac_en == 0)
						printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
					else
						printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-00-00-00-00 / 16.\n", *(cp + 1), *cp);

					/* decap as UC */
					printk(KERN_INFO "or\n");
					printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
					printk(KERN_INFO "  outer IPv4_DA = any Multicast address\n");
					printk(KERN_INFO "  inner IPv6_SA = any Unicast address\n");
					printk(KERN_INFO "  inner IPv6_DA = any Unicast address\n");
					printk(KERN_INFO "  Treated as UC packet.\n");
				} else {
					/* filter if outer MC and inner UC */
					/* decap as MC */
					printk(KERN_INFO "or\n");
					printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
					printk(KERN_INFO "  outer IPv4_DA = any Multicast address\n");
					printk(KERN_INFO "  inner IPv6_SA = any Unicast address\n");
					printk(KERN_INFO "  inner IPv6_DA = any Multicast address\n");
					if (ipnip_ctrl.bf.sixrd_ipmc_new_mac_en == 0)
						printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
					else
						printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-00-00-00-00 / 16.\n", *(cp + 1), *cp);
				}

			} else { /* if (entry.ingr_chk_en) */

				/* calculate the delegated prefix for ce_ip6 */
				tmp_u64 = (entry.ce_v4_ip << entry.v4_msk_len) >> entry.v4_msk_len; // clear the prefix to get v4_suffix
				tmp_u64 = tmp_u64 << (64 - (32 - entry.v4_msk_len)); // shift to MSB
				tmp_u64 = tmp_u64 >> entry.v6_prfx_len; // shift v4_suffix to proper position for final 6RD addr
				ipv6_addr_prefix(&ce_ip6, &prfx_ip6, entry.v6_prfx_len); // ce_ip6 contains the masked prefix
				ce_ip6.s6_addr32[0] |= htonl(*(itmp_u32+1));
				ce_ip6.s6_addr32[1] |= htonl(*itmp_u32);

				/* calculate the IPv4 part for br_ip6 */
				tmp_u64 = (entry.br_v4_ip << entry.v4_msk_len) >> entry.v4_msk_len; // clear the prefix to get v4_suffix
				tmp_u64 = tmp_u64 << (64 - (32 - entry.v4_msk_len)); // shift to MSB
				tmp_u64 = tmp_u64 >> entry.v6_prfx_len; // shift v4_suffix to proper position for final 6RD addr
				memset(&br_ip6, 0, sizeof(br_ip6));
				br_ip6.s6_addr32[0] |= htonl(*(itmp_u32+1));
				br_ip6.s6_addr32[1] |= htonl(*itmp_u32);

				if ((ipnip_ctrl.bf.sixrd_ipmc_ilg_chk_en == 1) && IN_MULTICAST(ntohl(ce_ip.s_addr)) && !ipv6_addr_is_multicast(&ce_ip6)) {
					printk(KERN_INFO "  WARNING! No packet from BR can match!\n");
					printk(KERN_INFO "  Reason: ce_ip is MC addr, v6_prfx is UC addr, ingr_chk_en=1, sixrd_ipmc_ilg_chk_en=1.\n");
				} else {
					printk(KERN_INFO "  Packet from BR\n");
					printk(KERN_INFO "  outer IPv4_SA = %pI4\n", &br_ip);
					printk(KERN_INFO "  outer IPv4_DA = %pI4\n", &ce_ip);
					if (entry.ipsa_mtch == 0) {
						printk(KERN_INFO "  inner IPv6_SA = any Unicast address\n");
					} else {
						printk(KERN_INFO "  inner IPv6_SA[127:%d] != %pI6\n", 128 - entry.v6_prfx_len, &prfx_ip6);

						/* only compare IPv4 part */
						printk(KERN_INFO "  inner IPv6_SA[%d:%d] = %pI6\n",
							127 - entry.v6_prfx_len, 128 - entry.v6_prfx_len - (32 - entry.v4_msk_len), &br_ip6);
					}
					printk(KERN_INFO "  inner IPv6_DA[127:%d] = %pI6\n", 128 - (entry.v6_prfx_len + (32 - entry.v4_msk_len)), &ce_ip6);
					if (ipv6_addr_is_multicast(&ce_ip6)) {
						if (ipnip_ctrl.bf.sixrd_ipmc_new_mac_en == 0)
							printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
						else
							printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-00-00-00-00 / 16.\n", *(cp + 1), *cp);
					} else {
						printk(KERN_INFO "  Treated as UC packet.\n");
					}
				}

				if (entry.br_only == 0) {
					if (ipv6_addr_is_multicast(&ce_ip6)) {
						printk(KERN_INFO "or\n");
						printk(KERN_INFO "  Packet from CE\n");
						printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
						printk(KERN_INFO "  outer IPv4_DA = any Unicast or Multicast address\n");
						printk(KERN_INFO "  inner IPv6_SA[127:%d] = %pI6\n", 128 - entry.v6_prfx_len, &prfx_ip6);
						printk(KERN_INFO "  inner IPv6_DA[127:%d] = %pI6\n", 128 - (entry.v6_prfx_len + (32 - entry.v4_msk_len)), &ce_ip6);
						if (ipnip_ctrl.bf.sixrd_ipmc_new_mac_en == 0)
							printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
						else
							printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-00-00-00-00 / 16.\n", *(cp + 1), *cp);
						printk(KERN_INFO "  WARNING: No such MC packet in real world but HW can decapsulate it.\n");
						printk(KERN_INFO "  Disable ingr_chk_en of 6RD entry if CE-to-CE MC is required.\n");

					} else {
						printk(KERN_INFO "or\n");
						printk(KERN_INFO "  Packet from CE\n");
						printk(KERN_INFO "  outer IPv4_SA = any Unicast addres\n");
						if (ipnip_ctrl.bf.sixrd_ipmc_ilg_chk_en == 0)
							printk(KERN_INFO "  outer IPv4_DA = any Unicast or Multicast address\n");
						else
							printk(KERN_INFO "  outer IPv4_DA = any Unicast address\n");
						printk(KERN_INFO "  inner IPv6_SA[127:%d] = %pI6\n", 128 - entry.v6_prfx_len, &prfx_ip6);
						printk(KERN_INFO "  inner IPv6_DA[127:%d] = %pI6\n", 128 - (entry.v6_prfx_len + (32 - entry.v4_msk_len)), &ce_ip6);
						printk(KERN_INFO "  Treated as UC packet.\n");
					}
				}

			} /* if (entry.ingr_chk_en) */

			/* TBD: In Linux, (v6_prfx_len + 32 - v4_msk_len > 64) is invalid. */
			if (entry.v6_prfx_len + entry.v4_msk_len > 64)
				printk(KERN_INFO "\nWARNING! The sum of v6_prfx_len and v4_msk_len exceeds 64.\n");

		}
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static aal_table_t l3pe_sixrd_tbl = {
	.table_id = AAL_TABLE_L3PE_SIXRD_TBL,
	.table_name = "L3FE PE 6RD",
	.max_entry = L3PE_SIXRD_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3pe_sixrd_tbl_entry_t),
	.access_reg = L3FE_PE_SIXRD_TBL_ACCESS,
	.data_reg_num = 7,
	.op = {
		.entry_add = l3pe_sixrd_tbl_entry_add,
		.entry_add_by_idx = l3pe_sixrd_tbl_entry_add_by_idx,
		.entry_del = l3pe_sixrd_tbl_entry_del,
		.entry_set = l3pe_sixrd_tbl_entry_set,
		.entry_get = l3pe_sixrd_tbl_entry_get,
		.entry_get_by_idx = l3pe_sixrd_tbl_entry_get_by_idx,
		.table_flush = l3pe_sixrd_tbl_table_flush,
		.get_avail_count = l3pe_sixrd_tbl_get_avail_count,
		.entry_print = l3pe_sixrd_tbl_entry_print,
	},
};

int aal_l3pe_sixrd_tbl_init(void)
{
	aal_table_t *table_p = &l3pe_sixrd_tbl;

	spin_lock_init(&l3pe_sixrd_tbl.lock);

	return __aal_table_register(table_p->table_id, table_p);
}

void aal_l3pe_sixrd_tbl_exit(void)
{
	aal_table_t *table_p = &l3pe_sixrd_tbl;

	__aal_table_unregister(table_p->table_id);
}

/***************************************************************************
 * DS-Lite
 ***************************************************************************/

/* The checks for ingress DS-Lite packet:
 * 1. IF ID check
 *    - When FIB.pop_l3_en is 1, ingress IF_ID[7:5] must be 3'b001.
 *    - If dsl_entry.mc is 0 or 1, inner packet must be UC or MC packet.
 *    - If it fails, result depends on REG.intf_chk_fail_fwd_ctl.
 * 2. Reverse path check
 *    - Outer IPv6SA/IPv6DA must match aftr_ip/b4_ip of dsl_entry.
 *    - UC only
 *    - If it fails, result depends on REG.dsl_decap_chk_fail_fwd_ctl.
 * 3. IPMC illegal check
 *    - Check the packet of outer MC and inner UC.
 *    - When this check is enabled, drop packet.
 *    - When this check is disabled,
 *      If dsl_entry.mc is 0, decapsulate the packet.
 *      If dsl_entry.mc is 1, packet already fails the IF_ID check because it is UC.
 *    - If it fails, result depends on REG.dsl_decap_chk_fail_fwd_ctl.
 * 4. IPMC ext det check
 *    - Check the packet of outer UC and inner MC.
 *    - When this check is enabled, packet is treated as DSLite MC.
 *    - When this check is disabled, packet is still decapsulated, but it's not treated as MC nor UC.
 *    - If it fails, result depends on REG.dsl_decap_chk_fail_fwd_ctl.
 * 5. IPMC address prefix check
 *    - IPv6SA/IPv6DA address prefix must match aftr_ip/b4_ip of dsl_entry.
 *    - aftr_ip contains {UPrefix64 / UPrefix_mask64}.
 *    - b4_ip contains {MPrefix64 / MPrefix_mask64}.
 *    - If it fails, result depends on REG.dsl_decap_chk_fail_fwd_ctl.
 * 6. IPMC address consistency check
 *    - Suffix 32 bits of outer IPv6 address must match inner IPv4 address.
 *    - If it fails, result depends on REG.dsl_decap_chk_fail_fwd_ctl.
 * 7. ECN check
 *    - Check when FIB.pop_l3_chk_ecn_en is 1.
 *    - If outer ECN is set but inner ECN is not, check fails.
 *    - If it fails, result depends on REG.ecn_chk_fail_fwd_ctl.
 */

int l3pe_dsl_tbl_entry_add(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_add(&l3pe_dsl_tbl, entry, rslt_idx);
}

int l3pe_dsl_tbl_entry_add_by_idx(void *entry, unsigned int idx)
{
	return generic_entry_add_by_idx(&l3pe_dsl_tbl, entry, idx);
}

int l3pe_dsl_tbl_entry_del(unsigned int idx)
{
	return generic_entry_del(&l3pe_dsl_tbl, idx);
}

int l3pe_dsl_tbl_entry_set(unsigned int idx, void *entry)
{
	return generic_entry_set(&l3pe_dsl_tbl, idx, entry);
}

int l3pe_dsl_tbl_entry_get(void *entry, unsigned int *rslt_idx)
{
	return generic_entry_get(&l3pe_dsl_tbl, entry, rslt_idx);
}

int l3pe_dsl_tbl_entry_get_by_idx(unsigned int idx, void *entry)
{
	return generic_entry_get_by_idx(&l3pe_dsl_tbl, idx, entry);
}

int l3pe_dsl_tbl_table_flush(void)
{
	return generic_table_flush(&l3pe_dsl_tbl);
}

int l3pe_dsl_tbl_get_avail_count(void)
{
	return generic_get_avail_count(&l3pe_dsl_tbl);
}

void l3pe_dsl_tbl_entry_print(unsigned int start_idx, int count)
{
	aal_table_t *table = &l3pe_dsl_tbl;
	l3pe_dsl_tbl_entry_t entry;
	int end_idx;
	int i, ret;
	unsigned int refcnt;

	struct in6_addr b4_ip, aftr_ip;
	l3fe_stg1_ipnip_ctrl_reg_t ipnip_ctrl;
	l3fe_stg1_ipnip_fail_fwd_ctrl_t ipnip_fail_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_fwd_ctrl_t dsl_ipmc_fwd_ctrl;
	l3fe_stg1_dsl_ipmc_new_mac_da_t dsl_ipmc_new_mac_da;

	uint32_t tmp_mac;
	uint8_t *cp = (uint8_t *) &tmp_mac;

	if (start_idx >= L3PE_DSL_TBL_ENTRY_MAX) {
		printk(KERN_ERR "%s: Out of range!\n", __func__);
		return;
	}

	if ((ret = aal_l3fe_stg1_ipnip_ctrl_get(&ipnip_ctrl)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_ipnip_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get(&ipnip_fail_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_ipnip_fail_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get(&dsl_ipmc_fwd_ctrl)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_dsl_ipmc_fwd_ctrl_get() returns %d.\n", ret);

	if ((ret = aal_l3fe_stg1_dsl_ipmc_new_mac_da_get(&dsl_ipmc_new_mac_da)) != AAL_E_OK)
		printk(KERN_INFO "ERROR! aal_l3fe_stg1_dsl_ipmc_new_mac_da_get() returns %d.\n", ret);
	tmp_mac = (uint32_t) dsl_ipmc_new_mac_da.bf.addr_prfx << 7;

	if ((count < 0) || (start_idx + count > L3PE_DSL_TBL_ENTRY_MAX)) {
		end_idx = L3PE_DSL_TBL_ENTRY_MAX;
	} else {
		end_idx = start_idx + count;
	}

	printk(KERN_INFO "|===== DS-Lite Table ==================================================|\n");

	for (i = start_idx; i < end_idx; i++) {
		if (ca_aal_table_sw_cache) {
			ret = l3pe_dsl_tbl_entry_get_by_idx(i, &entry);
		} else {
			ret = hw_entry_get(&l3pe_dsl_tbl, i, &entry);
		}
		if ((ret == AAL_E_ENTRYNOTRSVD) || (ret == AAL_E_ENTRYNOTFOUND) || (ret == AAL_E_NULLPTR)) {
			printk(KERN_INFO "index %04d, NOT USED\n", i);
		} else {
			refcnt = atomic_read(&table->entry[i].refcnt);

			printk(KERN_INFO "index %04d, ref count %d\n", i, refcnt);
			printk(KERN_INFO "valid(1) %01d, mc(1) %01d, flwlabel(20) 0x%05x\n", entry.valid, entry.mc, entry.flwlabel);
			printk(KERN_INFO "tc_en(1) %d, tc(8) 0x%02x\n", entry.tc_en, entry.tc);
			printk(KERN_INFO "b4_ip(128) %08x-%08x-%08x-%08x\n",
				entry.b4_ip3, entry.b4_ip2, entry.b4_ip1, entry.b4_ip0);
			printk(KERN_INFO "aftr_ip(128) %08x-%08x-%08x-%08x\n",
				entry.aftr_ip3, entry.aftr_ip2, entry.aftr_ip1, entry.aftr_ip0);

			if (entry.valid == 0) continue;
			printk(KERN_INFO "\nMatched DSLite packet:\n");

			if (entry.mc == 0) {
				b4_ip.in6_u.u6_addr32[0] = htonl(entry.b4_ip3);
				b4_ip.in6_u.u6_addr32[1] = htonl(entry.b4_ip2);
				b4_ip.in6_u.u6_addr32[2] = htonl(entry.b4_ip1);
				b4_ip.in6_u.u6_addr32[3] = htonl(entry.b4_ip0);
				aftr_ip.in6_u.u6_addr32[0] = htonl(entry.aftr_ip3);
				aftr_ip.in6_u.u6_addr32[1] = htonl(entry.aftr_ip2);
				aftr_ip.in6_u.u6_addr32[2] = htonl(entry.aftr_ip1);
				aftr_ip.in6_u.u6_addr32[3] = htonl(entry.aftr_ip0);

				/* Available cases: outer UC + inner UC, outer MC + inner UC.
				 * If ipmc_ilg_chk_en is 1,
				 *   (outer_MC + inner_UC) is dropped. (drop code 19)
				 *   (outer_UC + inner_UC) is decapsulated.
				 *   (outer_MC + inner_MC) and (outer_UC + inner_MC) are treated as MC,
				 *     they fails the IF_ID check because entry.mc is 0. (drop code 20)
				 */

				if ((ipv6_addr_is_multicast(&b4_ip)) && (ipnip_ctrl.bf.dsl_rpf_chk_en == 1) && (ipnip_ctrl.bf.dsl_ipmc_ilg_chk_en == 1)) {
					printk(KERN_INFO "  WARNING! No packet can match this entry and pass all checks.\n");
					printk(KERN_INFO "  Reason: b4_ip is MC addr, mc=0, rpf_chk_en=1, dsl_ipmc_ilg_chk_en=1.\n");
					continue;
				}

				if (ipnip_ctrl.bf.dsl_rpf_chk_en == 0) {
					printk(KERN_INFO "  outer IPv6_SA = any Unicast address\n");
					printk(KERN_INFO "  outer IPv6_DA = any Unicast address\n");
				} else {
					printk(KERN_INFO "  outer IPv6_SA = %pI6\n", &aftr_ip);
					printk(KERN_INFO "  outer IPv6_DA = %pI6\n", &b4_ip);
				}
				printk(KERN_INFO "  inner IPv4_SA = any Unicast address\n");
				printk(KERN_INFO "  inner IPv4_DA = any Unicast address\n");

				if (ipv6_addr_is_multicast(&b4_ip) && (ipnip_ctrl.bf.dsl_rpf_chk_en == 1)) // dsl_ipmc_ilg_chk_en must be 0
					printk(KERN_INFO "  Treated as UC packet.\n");

			} else { /* if (entry.mc) */
				b4_ip.in6_u.u6_addr32[0] = htonl(entry.b4_ip3 & entry.b4_ip1);
				b4_ip.in6_u.u6_addr32[1] = htonl(entry.b4_ip2 & entry.b4_ip0);
				b4_ip.in6_u.u6_addr32[2] = 0;
				b4_ip.in6_u.u6_addr32[3] = 0;
				aftr_ip.in6_u.u6_addr32[0] = htonl(entry.aftr_ip3 & entry.aftr_ip1);
				aftr_ip.in6_u.u6_addr32[1] = htonl(entry.aftr_ip2 & entry.aftr_ip0);
				aftr_ip.in6_u.u6_addr32[2] = 0;
				aftr_ip.in6_u.u6_addr32[3] = 0;

				/* Available cases: outer MC + inner MC, outer UC + inner MC.
				 * (outer MC + inner UC) and (outer UC + inner UC) are treated as UC,
				 *   they fails the IF_ID check because entry.mc is 1. (drop code 20)
				 * (outer_MC + inner_UC) also fails the IF_ID check
				 *   because this check is earlier than dsl_ipmc_ilg_chk (drop code 19).
				 */

				if (ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en == 1) {

					/* The valid combination of outer & inner IPDA when
					 * (dsl_ipmc_ilg_chk_en, dsl_ipmc_ext_det_en) is
					 *   (0, 0): outer MC + inner MC.
					 *   (0, 1): outer MC + inner MC, outer UC + inner MC.
					 *   (1, 0): outer MC + inner MC.
					 *   (1, 1): outer MC + inner MC, outer UC + inner MC.
					 */

					if ((ipnip_ctrl.bf.dsl_ipmc_ext_det_en == 0) && !ipv6_addr_is_multicast(&b4_ip)) {
						printk(KERN_INFO "  WARNING! No packet can match this entry and pass all checks.\n");
						printk(KERN_INFO "  The packet of outer_UC + inner_MC can be decapsulated but it is not treated as MC packet.\n");
						continue;
					}

					if (ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en == 1) {
						printk(KERN_INFO "  outer IPv6_SA[127:64] = %pI6/64, IPv6_SA[31:0] = inner IPv4_SA\n", &aftr_ip);
						printk(KERN_INFO "  outer IPv6_DA[127:64] = %pI6/64, IPv6_DA[31:0] = inner IPv4_DA\n", &b4_ip);
					} else {
						printk(KERN_INFO "  outer IPv6_SA[127:64] = %pI6/64\n", &aftr_ip);
						printk(KERN_INFO "  outer IPv6_DA[127:64] = %pI6/64\n", &b4_ip);
					}
					printk(KERN_INFO "  inner IPv4_SA = any Unicast address\n");
					printk(KERN_INFO "  inner IPv4_DA = any Multicast address\n");

					if (!ipv6_addr_is_multicast(&b4_ip) && (ipnip_ctrl.bf.dsl_ipmc_ext_det_en == 0)) {
						/* No idea how outer_UC + inner_MC is treated under this condition.
						 * If it is MC pkt, but new MAC_DA is never applied.
						 * If it is UC pkt, it should fail the IF_ID check.
						 */
						printk(KERN_INFO "  Packet is still decapsulated, but MAC_DA is unchanged after decapsulation.\n");
					} else {
						if (ipnip_ctrl.bf.dsl_ipmc_new_mac_en == 0) {
							printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
						} else {
							printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-%02x-%02x-00-00 / 25.\n",
								*(cp + 3), *(cp + 2), *(cp + 1), *cp);
						}
					}

				} else { /* if (ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en) */

					/* The valid combination of outer & inner IPDA when
					 * (dsl_ipmc_ilg_chk_en, dsl_ipmc_ext_det_en) is
					 *   (0, 0): outer MC + inner MC.
					 *   (0, 1): outer MC + inner MC, outer UC + inner MC.
					 *   (1, 0): outer MC + inner MC.
					 *   (1, 1): outer MC + inner MC, outer UC + inner MC.
					 */

					/* outer MC + inner MC */
					if (ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en == 1) {
						printk(KERN_INFO "  outer IPv6_SA = any Unicast address, IPv6_SA[31:0] = inner IPv4_SA\n");
						printk(KERN_INFO "  outer IPv6_DA = any Multicast address, IPv6_DA[31:0] = inner IPv4_DA\n");
					} else {
						printk(KERN_INFO "  outer IPv6_SA = any Unicast address\n");
						printk(KERN_INFO "  outer IPv6_DA = any Multicast address\n");
					}
					printk(KERN_INFO "  inner IPv4_SA = any Unicast address\n");
					printk(KERN_INFO "  inner IPv4_DA = any Multicast address\n");
					if (ipnip_ctrl.bf.dsl_ipmc_new_mac_en == 0) {
						printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
					} else {
						printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-%02x-%02x-00-00 / 25.\n",
							*(cp + 3), *(cp + 2), *(cp + 1), *cp);
					}

					/* outer UC + inner MC */
					printk(KERN_INFO "or\n");

					if (ipnip_ctrl.bf.dsl_ipmc_addr_consistency_chk_en == 1) {
						printk(KERN_INFO "  outer IPv6_SA = any Unicast address, IPv6_SA[31:0] = inner IPv4_SA\n");
						printk(KERN_INFO "  outer IPv6_DA = any Unicast address, IPv6_DA[31:0] = inner IPv4_DA\n");
					} else {
						printk(KERN_INFO "  outer IPv6_SA = any Unicast address\n");
						printk(KERN_INFO "  outer IPv6_DA = any Unicast address\n");
					}
					printk(KERN_INFO "  inner IPv4_SA = any Unicast address\n");
					printk(KERN_INFO "  inner IPv4_DA = any Multicast address\n");
					if (ipnip_ctrl.bf.dsl_ipmc_ext_det_en == 1) {
						if (ipnip_ctrl.bf.dsl_ipmc_new_mac_en == 0) {
							printk(KERN_INFO "  Treated as MC packet, but MAC_DA is unchanged after decapsulation.\n");
						} else {
							printk(KERN_INFO "  Treated as MC packet, new MAC_DA is %02x-%02x-%02x-%02x-00-00 / 25.\n",
								*(cp + 3), *(cp + 2), *(cp + 1), *cp);
						}
					} else {
						/* No idea how outer_UC + inner_MC is treated under this condition.
						 * If it is MC pkt, but new MAC_DA is never applied.
						 * If it is UC pkt, it should fail the IF_ID check.
						 */
						printk(KERN_INFO "  Packet is still decapsulated, but MAC_DA is unchanged after decapsulation.\n");
					}

				} /* if (ipnip_ctrl.bf.dsl_ipmc_addr_prfx_chk_en == 1) */

			} /* if (entry.mc == 0) */

		} /* if (ret == AAL_E_XXX) */
		printk(KERN_INFO "|------------------------------------------------------------------|\n");
		cond_resched();
	}
}

static aal_table_t l3pe_dsl_tbl = {
	.table_id = AAL_TABLE_L3PE_DSL_TBL,
	.table_name = "L3FE PE DSLite",
	.max_entry = L3PE_DSL_TBL_ENTRY_MAX,
	.used_entry = 0,
	.curr_ptr = 0,
	.entry_size = sizeof(l3pe_dsl_tbl_entry_t),
	.access_reg = L3FE_PE_DSL_TBL_ACCESS,
	.data_reg_num = 10,
	.op = {
		.entry_add = l3pe_dsl_tbl_entry_add,
		.entry_add_by_idx = l3pe_dsl_tbl_entry_add_by_idx,
		.entry_del = l3pe_dsl_tbl_entry_del,
		.entry_set = l3pe_dsl_tbl_entry_set,
		.entry_get = l3pe_dsl_tbl_entry_get,
		.entry_get_by_idx = l3pe_dsl_tbl_entry_get_by_idx,
		.table_flush = l3pe_dsl_tbl_table_flush,
		.get_avail_count = l3pe_dsl_tbl_get_avail_count,
		.entry_print = l3pe_dsl_tbl_entry_print,
	},
};

int aal_l3pe_dsl_tbl_init(void)
{
	aal_table_t *table_p = &l3pe_dsl_tbl;

	spin_lock_init(&l3pe_dsl_tbl.lock);

	return __aal_table_register(table_p->table_id, table_p);
}

void aal_l3pe_dsl_tbl_exit(void)
{
	aal_table_t *table_p = &l3pe_dsl_tbl;

	__aal_table_unregister(table_p->table_id);
}

/***************************************************************************
 * Counter - DROP SRC PKT
 ***************************************************************************/

/*** set drop_src_pkt ***/

static int __aal_l3pe_cntr_drop_src_pkt_set(unsigned int idx, ca_uint64 count)
{
	unsigned int access_reg = L3FE_PE_DROP_SRC_PKT_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_DROP_SRC_ENTRY_MAX) return AAL_E_OUTRANGE;

	ptr = (unsigned int *) &count;

	/* write the value to the specific data registers */
	for (i = 0; i < L3PE_CNTR_DROP_SRC_DATA_REG_NUM; i++)
		CA_NE_REG_WRITE(ptr[i], access_reg + ((L3PE_CNTR_DROP_SRC_DATA_REG_NUM - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return rc;
}

int aal_l3pe_cntr_drop_src_pkt_set(ca_device_id_t device_id, unsigned int idx, ca_uint64 count)
{
	return __aal_l3pe_cntr_drop_src_pkt_set(idx, count);
}

/*** get drop_src_pkt ***/

static int __aal_l3pe_cntr_drop_src_pkt_get(unsigned int idx, ca_uint64 *count)
{
	unsigned int access_reg = L3FE_PE_DROP_SRC_PKT_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_DROP_SRC_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (count == NULL) return AAL_E_NULLPTR;

	ptr = (unsigned int *) count;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT) {
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */
	} else {
		for (i = 0; i < L3PE_CNTR_DROP_SRC_DATA_REG_NUM; i++)
			ptr[i] = CA_NE_REG_READ(access_reg + ((L3PE_CNTR_DROP_SRC_DATA_REG_NUM - i) << 2));
	}

	return rc;
}

int aal_l3pe_cntr_drop_src_pkt_get(ca_device_id_t device_id, unsigned int idx, ca_uint64 *count)
{
	return __aal_l3pe_cntr_drop_src_pkt_get(idx, count);
}

/*** clear drop_src_pkt ***/

int aal_l3pe_cntr_drop_src_pkt_clear(ca_device_id_t device_id)
{
	L3FE_PE_DROP_SRC_PKT_CLR_t val;

	val.bf.clear = 1;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DROP_SRC_PKT_CLR);

	return AAL_E_OK;
}

/***************************************************************************
 * Counter - DROP SRC BYTE
 ***************************************************************************/

/*** set drop_src_byte ***/

static int __aal_l3pe_cntr_drop_src_byte_set(unsigned int idx, ca_uint64 count)
{
	unsigned int access_reg = L3FE_PE_DROP_SRC_BYTE_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_DROP_SRC_ENTRY_MAX) return AAL_E_OUTRANGE;

	ptr = (unsigned int *) &count;

	/* write the value to the specific data registers */
	for (i = 0; i < L3PE_CNTR_DROP_SRC_DATA_REG_NUM; i++)
		CA_NE_REG_WRITE(ptr[i], access_reg + ((L3PE_CNTR_DROP_SRC_DATA_REG_NUM - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return rc;
}

int aal_l3pe_cntr_drop_src_byte_set(ca_device_id_t device_id, unsigned int idx, ca_uint64 count)
{
	return __aal_l3pe_cntr_drop_src_byte_set(idx, count);
}

/*** get drop_src_byte ***/

static int __aal_l3pe_cntr_drop_src_byte_get(unsigned int idx, ca_uint64 *count)
{
	unsigned int access_reg = L3FE_PE_DROP_SRC_BYTE_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_DROP_SRC_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (count == NULL) return AAL_E_NULLPTR;

	ptr = (unsigned int *) count;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT) {
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */
	} else {
		for (i = 0; i < L3PE_CNTR_DROP_SRC_DATA_REG_NUM; i++)
			ptr[i] = CA_NE_REG_READ(access_reg + ((L3PE_CNTR_DROP_SRC_DATA_REG_NUM - i) << 2));
	}

	return rc;
}

int aal_l3pe_cntr_drop_src_byte_get(ca_device_id_t device_id, unsigned int idx, ca_uint64 *count)
{
	return __aal_l3pe_cntr_drop_src_byte_get(idx, count);
}

/*** clear drop_src_byte ***/

int aal_l3pe_cntr_drop_src_byte_clear(ca_device_id_t device_id)
{
	L3FE_PE_DROP_SRC_BYTE_CLR_t val;

	val.bf.clear = 1;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_DROP_SRC_BYTE_CLR);

	return AAL_E_OK;
}

/***************************************************************************
 * Counter - RX IFID
 ***************************************************************************/

/*** set rx_ifid ***/

static int __aal_l3pe_cntr_rx_set(unsigned int idx, l3pe_cntr_rx_t *data)
{
	unsigned int access_reg = L3FE_PE_RX_IFID_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_RX_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	ptr = (unsigned int *) data;

	/* write the value to the specific data registers */
	for (i = 0; i < L3PE_CNTR_RX_DATA_REG_NUM; i++)
		CA_NE_REG_WRITE(ptr[i], access_reg + ((L3PE_CNTR_RX_DATA_REG_NUM - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return rc;
}

int aal_l3pe_cntr_rx_set(ca_device_id_t device_id, unsigned int idx, l3pe_cntr_rx_t *data)
{
	return __aal_l3pe_cntr_rx_set(idx, data);
}

/*** get rx_ifid ***/

static int __aal_l3pe_cntr_rx_get(unsigned int idx, l3pe_cntr_rx_t *data)
{
	unsigned int access_reg = L3FE_PE_RX_IFID_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_RX_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	ptr = (unsigned int *) data;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT) {
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */
	} else {
		for (i = 0; i < L3PE_CNTR_RX_DATA_REG_NUM; i++)
			ptr[i] = CA_NE_REG_READ(access_reg + ((L3PE_CNTR_RX_DATA_REG_NUM - i) << 2));
	}

	return rc;
}

int aal_l3pe_cntr_rx_get(ca_device_id_t device_id, unsigned int idx, l3pe_cntr_rx_t *data)
{
	return __aal_l3pe_cntr_rx_get(idx, data);
}

/*** clear rx_ifid ***/

int aal_l3pe_cntr_rx_clear(ca_device_id_t device_id)
{
	L3FE_PE_RX_IFID_CLR_t val;

	val.bf.clear = 1;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_RX_IFID_CLR);

	return AAL_E_OK;
}

/***************************************************************************
 * Counter - TX IFID
 ***************************************************************************/

/*** set tx_ifid ***/

static int __aal_l3pe_cntr_tx_set(unsigned int idx, l3pe_cntr_tx_t *data)
{
	unsigned int access_reg = L3FE_PE_TX_IFID_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_TX_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	ptr = (unsigned int *) data;

	/* write the value to the specific data registers */
	for (i = 0; i < L3PE_CNTR_TX_DATA_REG_NUM; i++)
		CA_NE_REG_WRITE(ptr[i], access_reg + ((L3PE_CNTR_TX_DATA_REG_NUM - i) << 2));

	/* start the write access (prepare access register) */
	val |= 1 << 30;		/* set rbw to write */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT)
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */

	return rc;
}

int aal_l3pe_cntr_tx_set(ca_device_id_t device_id, unsigned int idx, l3pe_cntr_tx_t *data)
{
	return __aal_l3pe_cntr_tx_set(idx, data);
}

/*** get tx_ifid ***/

static int __aal_l3pe_cntr_tx_get(unsigned int idx, l3pe_cntr_tx_t *data)
{
	unsigned int access_reg = L3FE_PE_TX_IFID_CNT_TBL_ACCESS;
	int i, rc = AAL_E_OK;
	uint32_t val = 0;
	unsigned int *ptr;

	if (idx >= L3PE_CNTR_TX_ENTRY_MAX) return AAL_E_OUTRANGE;
	if (data == NULL) return AAL_E_NULLPTR;

	ptr = (unsigned int *) data;

	/* start the write access (prepare access register) */
	val |= 0 << 30;		/* set rbw to read */
	val |= 1 << 31;		/* set access to start */
	val |= idx;
	CA_NE_REG_WRITE(val, access_reg);

	/* wait write complete */
	i = 0;
	do {
		val = CA_NE_REG_READ(access_reg);
	} while (((1 << 31) & val) && (++i < TABLE_TRY_TIMEOUT));
	if (i >= TABLE_TRY_TIMEOUT) {
		rc = AAL_E_ACCESSINCOMP;	/* ACCESS can't complete */
	} else {
		for (i = 0; i < L3PE_CNTR_TX_DATA_REG_NUM; i++)
			ptr[i] = CA_NE_REG_READ(access_reg + ((L3PE_CNTR_TX_DATA_REG_NUM - i) << 2));
	}

	return rc;
}

int aal_l3pe_cntr_tx_get(ca_device_id_t device_id, unsigned int idx, l3pe_cntr_tx_t *data)
{
	return __aal_l3pe_cntr_tx_get(idx, data);
}

/*** clear tx_ifid ***/

int aal_l3pe_cntr_tx_clear(ca_device_id_t device_id)
{
	L3FE_PE_TX_IFID_CLR_t val;

	val.bf.clear = 1;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_TX_IFID_CLR);

	return AAL_E_OK;
}

/***************************************************************************
 * Counter Mode
 ***************************************************************************/

/* Counter mode.
 * cntr_wrap_around: the bit masks of L3PE_MODE_XXX, each bit represents a counter.
 *   1'b1: wrap around counter when it overflows.
 *   1'b0: stop counter when it overflows.
 */
int aal_l3pe_cntr_cfg_set(ca_device_id_t device_id, ca_uint32_t cntr_mode)
{
	L3FE_PE_MIB_CFG_t val;

	if ((cntr_mode & L3PE_MODE_MASK) == 0)
		return AAL_E_OUTRANGE;

	val.wrd = CA_NE_REG_READ(L3FE_PE_MIB_CFG);
	val.bf.counter_wrap_around = cntr_mode & L3PE_MODE_MASK;
	CA_NE_REG_WRITE(val.wrd, L3FE_PE_MIB_CFG);

	return AAL_E_OK;
}

int aal_l3pe_cntr_cfg_get(ca_device_id_t device_id, ca_uint32_t *cntr_mode)
{
	L3FE_PE_MIB_CFG_t val;

	val.wrd = CA_NE_REG_READ(L3FE_PE_MIB_CFG);
	*cntr_mode = val.bf.counter_wrap_around;

	return AAL_E_OK;
}

/***************************************************************************
 * Initialization
 ***************************************************************************/

int aal_l3pe_init(void)
{
	aal_l3pe_port_egress_behavior_init();
	aal_l3pe_mtu_size_mgnt();

	aal_l3pe_dsl_tbl_clear();
	aal_l3pe_6rd_tbl_clear();

	return AAL_E_OK;
}

EXPORT_SYMBOL(aal_l3pe_cntr_drop_src_pkt_get);
EXPORT_SYMBOL(aal_l3pe_cntr_drop_src_pkt_clear);
EXPORT_SYMBOL(aal_l3pe_mtu_size_add);
EXPORT_SYMBOL(aal_l3pe_mtu_size_del);
EXPORT_SYMBOL(aal_l3pe_mtu_size_lookup);
EXPORT_SYMBOL(aal_l3pe_pe_gemid_map_get);
EXPORT_SYMBOL(aal_l3pe_pe_gemid_map_set);
EXPORT_SYMBOL(aal_l3pe_pe_ldpid_base_get);
EXPORT_SYMBOL(aal_l3pe_pe_ldpid_base_set);
