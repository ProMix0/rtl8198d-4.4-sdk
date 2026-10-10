#ifndef CA_NI_HOOK_API_H
#define CA_NI_HOOK_API_H 1

struct __ca_ni_hook_ops {
	void (*nho_free_skb_callback)(struct sk_buff *skb);
};

extern struct __ca_ni_hook_ops ca_ni_hook_ops;

#endif
