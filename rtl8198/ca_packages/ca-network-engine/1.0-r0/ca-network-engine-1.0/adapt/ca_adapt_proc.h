#ifndef __CA_ADAPT_PROC_H__
#define __CA_ADAPT_PROC_H__

extern struct proc_dir_entry *proc_adapt_dir;

int ca_adapt_proc_init(void);
void ca_adapt_proc_exit(void);

#endif /* __CA_API_PROC_H__ */

