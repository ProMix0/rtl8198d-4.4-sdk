#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <soc/cortina/rtk_vlan_common.h>

unsigned int rtk_vlan_debug = 0x0;
unsigned char rtk_vlan_dbg_buff[RTK_VLAN_MAX_DBG_MSG_LEN];
EXPORT_SYMBOL(rtk_vlan_debug);
EXPORT_SYMBOL(rtk_vlan_dbg_buff);


/* if insert tag failed, it will free skb in this function */
int rtk_insert_vlan_tag_and_priority(unsigned short vid, unsigned char priority, struct sk_buff **pskb)
{
	struct sk_buff *newskb = NULL;
	struct sk_buff *skb = NULL;
	int skb_copied = 0;

	if (!pskb || !*pskb)
		return FAIL;

	skb = *pskb;
	if (*((unsigned short *)(skb->data+ETH_ALEN*2)) != __constant_htons(ETH_P_8021Q)) 
	{
		if (skb_cloned(skb)){
			newskb = skb_copy(skb, GFP_ATOMIC);
			if (newskb == NULL) {
				dev_kfree_skb_any(skb);
				return FAIL;
			}
			dev_kfree_skb_any(skb);
			skb = newskb;
			*pskb = newskb;
			skb_copied = 1;
		}
		
		if ((skb_headroom(skb) < VLAN_HLEN) && (skb_cow(skb, VLAN_HLEN) != 0))
		{
			printk("%s %d: error! (skb_headroom(skb) == %d < 4). Enlarge it failed! \n",
				__FUNCTION__, __LINE__, skb_headroom(skb));
			dev_kfree_skb_any(skb);
			return FAIL;
		}
		
		memmove(skb->data-VLAN_HLEN, skb->data, ETH_ALEN<<1);
		skb_push(skb,VLAN_HLEN);			
		*((unsigned short *)(skb->data+(ETH_ALEN<<1))) = __constant_htons(ETH_P_8021Q);
		*((unsigned short *)(skb->data+(ETH_ALEN<<1)+2)) = __constant_htons((0xfff & vid) | ((0x7 & priority)<<13));
	}
	
	if(skb_copied)
		return RTK_SKB_COPIED;
	
	return SUCCESS;
}
EXPORT_SYMBOL(rtk_insert_vlan_tag_and_priority);

int rtk_mod_vlan_tag_priority(unsigned char priority, struct sk_buff **pskb)
{
	struct sk_buff *newskb = NULL;
	struct sk_buff *skb = NULL;
	unsigned short vid;
	int skb_copied = 0;

	if (!pskb || !*pskb)
		return FAIL;

	skb = *pskb;
	
	if (*((unsigned short *)(skb->data+ETH_ALEN*2)) == __constant_htons(ETH_P_8021Q)){
		if (skb_cloned(skb)){
			newskb = skb_copy(skb, GFP_ATOMIC);
			if (newskb == NULL) {
				dev_kfree_skb_any(skb);
				return FAIL;
			}
			dev_kfree_skb_any(skb);
			skb = newskb;
			*pskb = newskb;
			skb_copied = 1;
		}

		vid = __constant_ntohs(*((unsigned short *)(skb->data+(ETH_ALEN<<1)+2))) & 0xfff;
		*((unsigned short *)(skb->data+(ETH_ALEN<<1)+2)) = __constant_htons(vid | ((0x7 & priority)<<13));
	}

	if(skb_copied)
		return RTK_SKB_COPIED;
	
	return SUCCESS;
}
EXPORT_SYMBOL(rtk_mod_vlan_tag_priority);

int remove_vlan_tag(struct sk_buff **pskb)
{
	struct sk_buff *newskb = NULL;
	struct sk_buff *skb = *pskb;
	int skb_copied = 0;
	
	if(*((unsigned short *)(skb->data+(ETH_ALEN<<1))) == __constant_htons(ETH_P_8021Q))
	{	
		if (skb_cloned(skb)){
			newskb = skb_copy(skb, GFP_ATOMIC);
			if (newskb == NULL) {
				dev_kfree_skb_any(skb);
				return FAIL;
			}
			dev_kfree_skb_any(skb);
			skb = newskb;
			*pskb = newskb;
			skb_copied = 1;
		}

		memmove(skb->data+VLAN_HLEN, skb->data, ETH_ALEN*2); 
		skb_pull(skb, VLAN_HLEN);
	}

	if(skb_copied)
		return RTK_SKB_COPIED;

	return SUCCESS;
}
EXPORT_SYMBOL(remove_vlan_tag);


unsigned short rtk_get_skb_vid(struct sk_buff *skb)
{
   	if(*((unsigned short *)(skb->data+(ETH_ALEN<<1))) == __constant_htons(ETH_P_8021Q))   				
		return ntohs(*(unsigned short*)(skb->data+(ETH_ALEN<<1)+2)) & 0x0fff; 				

   	return 0;
}
EXPORT_SYMBOL(rtk_get_skb_vid);

