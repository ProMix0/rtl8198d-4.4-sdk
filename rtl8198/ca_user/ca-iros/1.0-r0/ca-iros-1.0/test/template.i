%module demo
%{

/* KERNEL CONFIG */
/*__CODE_GEN_H_TAG__*/

%}
%include tclsh.i
/*__CODE_GEN_TAG__*/



extern void ca_init_signal();
extern void ca_data_free(void *data);

extern unsigned int *ca_uint32_create(unsigned int data);
extern unsigned int ca_uint32_get(unsigned int *ptr);

extern unsigned short *ca_uint16_create(unsigned short data);
extern unsigned short ca_uint16_get(unsigned short *ptr);
extern unsigned short *ca_uint16_array_create(unsigned short data,unsigned int size);
extern unsigned short ca_uint16_array_get(unsigned short *ptr,unsigned int index);
extern void ca_uint16_array_set(unsigned short *ptr,unsigned short data,unsigned int index);

extern unsigned char *ca_uint8_create(unsigned char data);
extern unsigned char ca_uint8_get(unsigned char *ptr);
extern unsigned char *ca_uint8_array_create(unsigned char data,unsigned int size);
extern unsigned char ca_uint8_array_get(unsigned char *ptr,unsigned int index);
extern void ca_uint8_array_set(unsigned char *ptr,unsigned char data,unsigned int index);


extern unsigned int *ca_uint32_array_create(unsigned int data,unsigned int size);
extern unsigned int ca_uint32_array_get(unsigned int *ptr,unsigned int index);
extern void ca_uint32_array_set(unsigned int *ptr,unsigned int data, unsigned int index);

extern unsigned long long  *ca_uint64_create(unsigned long long data);
extern unsigned long long  ca_uint64_get(unsigned long long *ptr);
extern unsigned long long  *ca_uint64_array_create(unsigned long long data,unsigned int size);
extern unsigned long long  ca_uint64_array_get(unsigned long long *ptr,unsigned int index);
extern void ca_uint64_array_set(unsigned long long *ptr,unsigned long long data,unsigned int index);


extern ca_mac_addr_t *ca_mac_addr_create(unsigned char mac0, unsigned char mac1, unsigned char mac2, 
      unsigned char mac3, unsigned char mac4, unsigned char mac5);
extern void ca_mac_addr_set(ca_mac_addr_t * mac, unsigned char mac0, unsigned char mac1, unsigned char mac2, 
      unsigned char mac3, unsigned char mac4, unsigned char mac5);
extern unsigned char ca_mac_addr_get(ca_mac_addr_t *mac, unsigned int index );
extern void ca_mac_addr_dump(ca_mac_addr_t * mac);

/*
ca_mac_addr_t *ca_mac_array_create(unsigned char data,unsigned int size);
void ca_mac_array_dump(ca_mac_addr_t *cfg , unsigned int size ) ;
void ca_mac_array_set(ca_mac_addr_t * mac, ca_mac_addr_t *value, unsigned int index);
*/

extern int *ca_int32_create(int data);
extern int ca_int32_get(int *ptr);
extern int *ca_int32_array_create(int data,unsigned int size);
extern int ca_int32_array_get(int *ptr,unsigned int index);
extern void ca_int32_array_set(int *ptr,int data,unsigned int index);

extern ca_l3_ip_addr_t *  ca_l3_ip_addr_create();
extern void ca_l3_ip_addr_set_addr(ca_l3_ip_addr_t * cfg, unsigned int data, unsigned int index);
extern unsigned int ca_l3_ip_addr_get_addr(ca_l3_ip_addr_t * cfg, int index);
extern void ca_l3_ip_addr_set_ipv4_addr(ca_l3_ip_addr_t * cfg, unsigned int data);
extern unsigned int ca_l3_ip_addr_get_ipv4_addr(ca_l3_ip_addr_t * cfg);
extern void ca_l3_ip_addr_set_ipv6_addr(ca_l3_ip_addr_t * cfg, unsigned int data, unsigned int index);
extern unsigned int ca_l3_ip_addr_get_ipv6_addr(ca_l3_ip_addr_t * cfg, unsigned int index);
extern void ca_l3_ip_addr_dump(ca_l3_ip_addr_t * cfg);

extern ca_ip_address_t *  ca_ip_address_create();
extern void ca_ip_address_set_afi(ca_ip_address_t * cfg, unsigned int data);
extern unsigned int ca_ip_address_get_afi(ca_ip_address_t * cfg);
extern void ca_ip_address_set_ip_addr(ca_ip_address_t * cfg, ca_l3_ip_addr_t * ip_addr);
extern ca_l3_ip_addr_t * ca_ip_address_get_ip_addr(ca_ip_address_t * cfg);
extern void ca_ip_address_set_addr_len(ca_ip_address_t * cfg, unsigned char data);
extern unsigned char ca_ip_address_get_addr_len(ca_ip_address_t * cfg);
extern void ca_ip_address_dump(ca_ip_address_t *ip_address);

extern ca_iterator_t *  ca_iterator_create();
extern void ca_iterator_dump(ca_iterator_t * cfg);
extern void ca_iterator_set_entry_count(ca_iterator_t * cfg,unsigned int entry_count);
extern unsigned int ca_iterator_get_entry_count(ca_iterator_t * cfg);
extern void ca_iterator_set_entry_data(ca_iterator_t * cfg, unsigned int * entry_data);
extern unsigned int * ca_iterator_get_entry_data(ca_iterator_t * cfg);
extern void ca_iterator_set_user_buffer_size(ca_iterator_t * cfg,unsigned int user_buffer_size);
extern unsigned int ca_iterator_get_user_buffer_size(ca_iterator_t * cfg);
extern void ca_iterator_set_prev_handle(ca_iterator_t * cfg, unsigned int * prev_handle);
extern unsigned int * ca_iterator_get_prev_handle(ca_iterator_t * cfg);

