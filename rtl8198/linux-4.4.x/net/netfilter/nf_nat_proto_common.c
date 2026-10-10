/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2008 Patrick McHardy <kaber@trash.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/netfilter.h>
#include <linux/export.h>

#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_l3proto.h>
#include <net/netfilter/nf_nat_l4proto.h>

bool nf_nat_l4proto_in_range(const struct nf_conntrack_tuple *tuple,
			     enum nf_nat_manip_type maniptype,
			     const union nf_conntrack_man_proto *min,
			     const union nf_conntrack_man_proto *max)
{
	__be16 port;

	if (maniptype == NF_NAT_MANIP_SRC)
		port = tuple->src.u.all;
	else
		port = tuple->dst.u.all;

	return ntohs(port) >= ntohs(min->all) &&
	       ntohs(port) <= ntohs(max->all);
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_in_range);

/**	
 *					 0                   1
 *                 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *                 +-----------+-----------+-------+
 *  Ports in       |     A     |    PSID   |   j   |
 *the CE port set  |    > 0    |           |       |
 *                 +-----------+-----------+-------+
 *                 |  a bits   |  k bits   |m bits |
                 
 * a bits:  The number of offset bits -- 6 by default, as this excludes the system ports (0-1023).
 *
 * A:  Selects the range of the port number. For 'a' > 0, A MUST be larger than 0. 
 		This ensures that the algorithm excludes the system ports.
		For the default value of 'a' (6), the system ports are excluded by requiring that A be greater than 0. 
		Smaller values of 'a' exclude a larger initial range, e.g., 'a' = 4 will exclude ports 0-4095. 
 *
 * k bits:  The length in bits of the PSID field.
 *
 * PSID:  The Port Set Identifier (PSID).
 *
 * m bits:  The number of contiguous ports is given by 2^m.
 *
 * j:  Selects the specific port within a particular range specified by the concatenation of A and the PSID.
 **/

#define PORT_PSID_HEX_CONVERT(val) (__u16)((__u16)(val<<(16-k))>>a)
#define PORT_PSID_PART_MASK (__u16)((__u16)(0xFFFF<<(16-k))>>a)

#define PORT_BIT_GET(val, mask)	(__u16)(val&mask)
#define PORT_A_VAL_GET(val)	 PORT_BIT_GET(val>>(16-a), (1<<a)-1)
#define PORT_j_VAL_GET(val)	PORT_BIT_GET(val, (1<<m)-1)

bool _nf_nat_l4ptoto_in_portset(__be16 port, const u8 PSID_offset, 
				const u8 PSID_len, const __be16 PSID_val){	
	u8 a, k, m;
	u16 PSID_mask, PSID_hex, A_val;
	bool ret=false;

	a = PSID_offset;
	k = PSID_len;
	m = 16-a-k;

	A_val = PORT_A_VAL_GET(ntohs(port));
	if (A_val == 0)//exclude the system ports
		return ret;
	
	PSID_mask = PORT_PSID_PART_MASK;
	PSID_hex = PORT_PSID_HEX_CONVERT(ntohs(PSID_val));

	//printk("[%s:%d] ntohs(port)=%u, PSID_mask=0x%x, PSID_hex=0x%x\n",__FUNCTION__,__LINE__, ntohs(port),PSID_mask,PSID_hex);
	ret = (ntohs(port)&PSID_mask == PSID_hex&PSID_mask);
	return ret;
}

bool nf_nat_l4proto_in_portset(const struct nf_conntrack_tuple *tuple,
			     enum nf_nat_manip_type maniptype,
			     const u8 PSID_offset, const u8 PSID_len, const __be16 PSID_val)
{
	__be16 port, PSID_mask;

	if (maniptype == NF_NAT_MANIP_SRC)
		port = tuple->src.u.all;
	else
		port = tuple->dst.u.all;
	return _nf_nat_l4ptoto_in_portset(port, PSID_offset, PSID_len, PSID_val);
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_in_portset);

/*	nf_nat_l4proto_unique_tuple_PSID, get a new port from PSID port-set.

	The strategy of getting the port:
	If a=4, k=8, m=4, PSID=52(0x34) and old_port = 0x5112, then A_val=5, j_val=2;
	(1) we first try the ports with A_val=5, j_val changed from 2, 3, 4...f, 0, 1,
	 like 0x5342, 0x5343, ...0x534f, 0x5340, 0x5341.
	 
	(2) If step (1) fail, then try the ports with A_val=6(increased), j_val changed from 2, 3, 4...f, 0, 1,
	 like 0x6342, 0x6343, ...0x634f, 0x6340, 0x6341.
	 
	 The sequence of ports tired is as follows:
	 0x5342, 0x5343, ...0x534f, 0x5340, 0x5341
	 0x6342, 0x6343, ...0x634f, 0x6340, 0x6341
	 ...
	 0xf342, 0xf343, ...0xf34f, 0xf340, 0xf341
	 0x1342, 0x1343, ...0x134f, 0x1340, 0x1341
	 ...
	 0x4342, 0x4343, ...0x434f, 0x4340, 0x4341
*/
void nf_nat_l4proto_unique_tuple_PSID(struct nf_conntrack_tuple *tuple,
				 const struct nf_nat_range *range, const struct nf_conn *ct,
				 __be16 *target_port)
{
	u16 old_port, new_port;
	u8 a, k, m;
	u16 PSID_hex, PSID_mask;
	u16 A_val, j_val, A_max, j_max;
	u32 i, j;

	a = range->psid.offset;
	k = range->psid.len;
	m = 16-a-k;

	A_max = (u16)(1<<a);
	j_max = (u16)(1<<m);
	
	PSID_hex = PORT_PSID_HEX_CONVERT(ntohs(range->psid.val));
	PSID_mask = PORT_PSID_PART_MASK;


	old_port = ntohs(*target_port);	
	A_val = PORT_A_VAL_GET(old_port);
	j_val = PORT_j_VAL_GET(old_port);
	/*
	printk("[%s:%d] a=%u, k=%u, m=%u, A_max=%u, j_max=%u\n",__FUNCTION__,__LINE__, a,k,m,A_max, j_max);
	printk("[%s:%d] old_port=%u, A_val=%u, j_val=%u\n",__FUNCTION__,__LINE__, old_port, A_val, j_val);
	*/
	for (i = A_val; i < A_max+A_val; i++){
		if (i%A_max == 0) // start from 1 to exclude the system ports
			continue;
		
		for (j = j_val; j < j_max+j_val; j++){
			new_port = (u16)(((i%A_max)<<(16-a) | PSID_hex | (j%j_max))&0xffff);// i | 0x0340 | j
			*target_port = htons(new_port);
			if (!nf_nat_used_tuple(tuple, ct)){			
				//printk("[%s:%d] old_port=%u, new_port=%u\n",__FUNCTION__,__LINE__, old_port, new_port);
				return;
			}else {
				continue;
			}
		}
	}
	
	//printk("[%s:%d] old_port=%u, new_port=%u\n",__FUNCTION__,__LINE__, old_port, new_port);
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_unique_tuple_PSID);


void nf_nat_l4proto_unique_tuple(const struct nf_nat_l3proto *l3proto,
				 struct nf_conntrack_tuple *tuple,
				 const struct nf_nat_range *range,
				 enum nf_nat_manip_type maniptype,
				 const struct nf_conn *ct,
				 u16 *rover)
{
	unsigned int range_size, min, max, i;
	__be16 *portptr;
	u_int16_t off;

	if (maniptype == NF_NAT_MANIP_SRC)
		portptr = &tuple->src.u.all;
	else
		portptr = &tuple->dst.u.all;

	if (range->flags & NF_NAT_RANGE_PORT_PSID){
		return nf_nat_l4proto_unique_tuple_PSID(tuple, range, ct, portptr);
	}
	
	/* If no range specified... */
	if (!(range->flags & NF_NAT_RANGE_PROTO_SPECIFIED)) {
		/* If it's dst rewrite, can't change port */
		if (maniptype == NF_NAT_MANIP_DST)
			return;

		if (ntohs(*portptr) < 1024) {
			/* Loose convention: >> 512 is credential passing */
			if (ntohs(*portptr) < 512) {
				min = 1;
				range_size = 511 - min + 1;
			} else {
				min = 600;
				range_size = 1023 - min + 1;
			}
		} else {
			min = 1024;
			range_size = 65535 - 1024 + 1;
		}
	} else {
		min = ntohs(range->min_proto.all);
		max = ntohs(range->max_proto.all);
		if (unlikely(max < min))
			swap(max, min);
		range_size = max - min + 1;
	}

	if (range->flags & NF_NAT_RANGE_PROTO_RANDOM) {
		off = l3proto->secure_port(tuple, maniptype == NF_NAT_MANIP_SRC
						  ? tuple->dst.u.all
						  : tuple->src.u.all);
	} else if (range->flags & NF_NAT_RANGE_PROTO_RANDOM_FULLY) {
		off = prandom_u32();
	} else {
		off = *rover;
	}

	for (i = 0; ; ++off) {
		*portptr = htons(min + off % range_size);	
		if (++i != range_size && nf_nat_used_tuple(tuple, ct))
			continue;
		
		if (!(range->flags & NF_NAT_RANGE_PROTO_RANDOM_ALL))
			*rover = off;
		return;
	}
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_unique_tuple);

#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
int nf_nat_l4proto_nlattr_to_range(struct nlattr *tb[],
				   struct nf_nat_range *range)
{
	if (tb[CTA_PROTONAT_PORT_MIN]) {
		range->min_proto.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MIN]);
		range->max_proto.all = range->min_proto.all;
		range->flags |= NF_NAT_RANGE_PROTO_SPECIFIED;
	}
	if (tb[CTA_PROTONAT_PORT_MAX]) {
		range->max_proto.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MAX]);
		range->flags |= NF_NAT_RANGE_PROTO_SPECIFIED;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(nf_nat_l4proto_nlattr_to_range);
#endif
