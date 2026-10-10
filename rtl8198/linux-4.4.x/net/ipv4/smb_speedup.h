#ifdef CONFIG_RTK_SMB_SPEEDUP
int getSMBflag(void);
int checkSMBIPPort(struct sk_buff *skb, int hook);
int checkSMBLocalIPPort(u32 srcip, u16 dport);
#endif
