#include "os_core.h"
#include "osal_common.h"
#include "util_aes.h"

/*lint -save -w1*/
/*osal_cmn.c*/
EXPORT_SYMBOL(ca_printf);
EXPORT_SYMBOL(ca_current_time);
EXPORT_SYMBOL(ca_current_tick);
EXPORT_SYMBOL(ca_udelay);
EXPORT_SYMBOL(ca_mdelay);
EXPORT_SYMBOL(ca_sleep);

/*osal_mem.c*/
EXPORT_SYMBOL(ca_malloc);
EXPORT_SYMBOL(ca_free);

/*osal_mutex.c*/
EXPORT_SYMBOL(ca_mutex_init);
EXPORT_SYMBOL(ca_mutex_destroy);
EXPORT_SYMBOL(ca_mutex_lock);
EXPORT_SYMBOL(ca_mutex_unlock);

/*osal_sem.c*/
EXPORT_SYMBOL(ca_sem_init);
EXPORT_SYMBOL(ca_sem_destroy);
EXPORT_SYMBOL(ca_sem_wait);
EXPORT_SYMBOL(ca_sem_post);

/*osal_spinlock.c*/
EXPORT_SYMBOL(ca_spin_lock_init);
EXPORT_SYMBOL(ca_spin_lock);
EXPORT_SYMBOL(ca_spin_unlock);
EXPORT_SYMBOL(ca_spin_lock_destroy);

/*osal_timer.c*/
EXPORT_SYMBOL(ca_timer_add);
EXPORT_SYMBOL(ca_circle_timer_add);
EXPORT_SYMBOL(ca_timer_del);
EXPORT_SYMBOL(ca_timer_stop);
EXPORT_SYMBOL(ca_timer_start);
EXPORT_SYMBOL(ca_timer_show);
EXPORT_SYMBOL(ca_timer_restart);

/*util_bitmap.c*/
EXPORT_SYMBOL(ca_bit_set);
EXPORT_SYMBOL(ca_bit_clr);
EXPORT_SYMBOL(ca_bit_tst);
EXPORT_SYMBOL(ca_bit_get_0);
EXPORT_SYMBOL(ca_bit_tst_all);

/*util_rbtree.c*/
EXPORT_SYMBOL(ca_rbtree_create);
EXPORT_SYMBOL(ca_rbtree_destory);
EXPORT_SYMBOL(ca_rbtree_add);
EXPORT_SYMBOL(ca_rbtree_delete);
EXPORT_SYMBOL(ca_rbtree_delete_all);
EXPORT_SYMBOL(ca_rbtree_query);
EXPORT_SYMBOL(ca_rbtree_traverse);
EXPORT_SYMBOL(ca_rbtree_first);
EXPORT_SYMBOL(ca_rbtree_predecessor);
EXPORT_SYMBOL(ca_rbtree_successor);
EXPORT_SYMBOL(ca_rbtree_count);

/*util_list.c*/
EXPORT_SYMBOL(ca_list_create);
EXPORT_SYMBOL(ca_list_destory);
EXPORT_SYMBOL(ca_list_add);
EXPORT_SYMBOL(ca_list_delete);
EXPORT_SYMBOL(ca_list_delete_all);
EXPORT_SYMBOL(ca_list_query);
EXPORT_SYMBOL(ca_list_traverse);
EXPORT_SYMBOL(ca_list_first);
EXPORT_SYMBOL(ca_list_last);
EXPORT_SYMBOL(ca_list_previous);
EXPORT_SYMBOL(ca_list_next);
EXPORT_SYMBOL(ca_list_count);

/*util_table.c*/
EXPORT_SYMBOL(ca_table_create);
EXPORT_SYMBOL(ca_table_destory);
EXPORT_SYMBOL(ca_table_entry_insert);
EXPORT_SYMBOL(ca_table_entry_remove);
EXPORT_SYMBOL(ca_table_entry_remove_all);
EXPORT_SYMBOL(ca_table_entry_search);
EXPORT_SYMBOL(ca_table_entry_traverse);
EXPORT_SYMBOL(ca_table_entry_getfirst);
EXPORT_SYMBOL(ca_table_entry_getnext);
EXPORT_SYMBOL(ca_table_entry_count);

/*util_misc.c*/
EXPORT_SYMBOL(ca_atoi);
EXPORT_SYMBOL(ca_inter_to64);
EXPORT_SYMBOL(ca_str2dec);
EXPORT_SYMBOL(ca_strtol);
EXPORT_SYMBOL(ca_str2decl);
EXPORT_SYMBOL(ca_strtoll);
EXPORT_SYMBOL(ca_strtoul);
EXPORT_SYMBOL(ca_str2mac);
EXPORT_SYMBOL(ca_str2ip);
EXPORT_SYMBOL(ca_ip2str);
EXPORT_SYMBOL(ca_str2ipv6);
EXPORT_SYMBOL(ca_ipv6tostr);
EXPORT_SYMBOL(ca_str2hex);
EXPORT_SYMBOL(ca_rbuf_create);
EXPORT_SYMBOL(ca_rbuf_read);
EXPORT_SYMBOL(ca_rbuf_write);

/*util_aes.c*/
EXPORT_SYMBOL(aes128_ecb_encrypt);
EXPORT_SYMBOL(aes128_ecb_decrypt);
EXPORT_SYMBOL(aes128_cmac_compute);
EXPORT_SYMBOL(random_key_generate);

/*osal_workqueue.c*/
EXPORT_SYMBOL(ca_workqueue_init);
EXPORT_SYMBOL(ca_workqueue_destroy);
EXPORT_SYMBOL(ca_work_add);

/*osal_file.c*/
EXPORT_SYMBOL(ca_file_open);
EXPORT_SYMBOL(ca_file_close);
EXPORT_SYMBOL(ca_file_read);
EXPORT_SYMBOL(ca_file_write);
EXPORT_SYMBOL(ca_file_size);
EXPORT_SYMBOL(ca_file_cp);

/*osal_thread.c*/
EXPORT_SYMBOL(ca_thread_create);

/*osal_queue.c*/
EXPORT_SYMBOL(ca_queue_create);
EXPORT_SYMBOL(ca_queue_delete);
EXPORT_SYMBOL(ca_queue_get);
EXPORT_SYMBOL(ca_queue_put);

/*osal_msic.c*/
EXPORT_SYMBOL(ca_div);

/*lint -restore*/

