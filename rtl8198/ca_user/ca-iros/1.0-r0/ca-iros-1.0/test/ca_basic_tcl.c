#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "ca_types.h"

struct termios oldtermios;

void ca_sigcatch(int sig)
{
    tcsetattr(0, TCSAFLUSH, &oldtermios);
    exit(0);
}

void ca_init_signal()
{

    tcgetattr(0, &oldtermios);

    signal(SIGINT,ca_sigcatch);
    signal(SIGQUIT,ca_sigcatch);
    signal(SIGTERM,ca_sigcatch);
}

void ca_data_free(void *data)
{
    free(data);
}

ca_int32 *ca_int32_create(ca_int32 data)
{
    ca_int32 *u32_ptr = malloc(sizeof(ca_int32));

    *u32_ptr = data;
    return u32_ptr;
}

ca_int32 ca_int32_get(ca_int32 *ptr)
{
    return *ptr;
}

ca_int32 *ca_int32_array_create(ca_int32 data,ca_uint32 size)
{
    ca_int32 *u32_ptr = malloc(sizeof(ca_int32)*size);
    int i;

    for( i =0; i < size;i++){
        u32_ptr[i] = data;
    }
    return u32_ptr;
}

ca_int32 ca_int32_array_get(ca_int32 *ptr,ca_uint32 index)
{
    return ptr[index];
}

void ca_int32_array_set(ca_int32 *ptr,ca_int32 data,ca_uint32 index)
{
     ptr[index] = data;
}

ca_uint32 *ca_uint32_create(ca_uint32 data)
{
    ca_uint32 *u32_ptr = malloc(sizeof(ca_uint32));

    *u32_ptr = data;
    return u32_ptr;
}

ca_uint32 ca_uint32_get(ca_uint32 *ptr)
{
    return *ptr;
}

ca_uint32 *ca_uint32_array_create(ca_uint32 data,ca_uint32 size)
{
    ca_uint32 *u32_ptr = malloc(sizeof(ca_uint32)*size);
    int i;

    for( i =0; i < size;i++){
        u32_ptr[i] = data;
    }
    return u32_ptr;
}

ca_uint32 ca_uint32_array_get(ca_uint32 *ptr,ca_uint32 index)
{
    return ptr[index];
}

void ca_uint32_array_set(ca_uint32 *ptr,ca_uint32 data,ca_uint32 index)
{
     ptr[index] = data;
}

ca_uint16 *ca_uint16_create(ca_uint16 data)
{
    ca_uint16 *u16_ptr = malloc(sizeof(ca_uint16));

    *u16_ptr = data;
    return u16_ptr;
}

ca_uint16 ca_uint16_get(ca_uint16 *ptr)
{
    return *ptr;
}

ca_uint16 *ca_uint16_array_create(ca_uint16 data,ca_uint32 size)
{
    ca_uint16 *u32_ptr = malloc(sizeof(ca_uint16)*size);
    int i;

    for( i =0; i < size;i++){
        u32_ptr[i] = data;
    }
    return u32_ptr;
}

ca_uint16 ca_uint16_array_get(ca_uint16 *ptr,ca_uint32 index)
{
    return ptr[index];
}

void ca_uint16_array_set(ca_uint16 *ptr,ca_uint16 data,ca_uint32 index)
{
     ptr[index] = data;
}

ca_uint8 *ca_uint8_create(ca_uint8 data)
{
    ca_uint8 *u8_ptr = malloc(sizeof(ca_uint8));

    *u8_ptr = data;
    return u8_ptr;
}

ca_uint8 ca_uint8_get(ca_uint8 *ptr)
{
    return *ptr;
}

ca_uint8 *ca_uint8_array_create(ca_uint8 data,ca_uint32 size)
{
    ca_uint8 *u8_ptr = malloc(sizeof(ca_uint8)*size);
    int i;

    for( i =0; i < size;i++){
        u8_ptr[i] = data;
    }
    return u8_ptr;
}

ca_uint8 ca_uint8_array_get(ca_uint8 *ptr,ca_uint32 index)
{
    return ptr[index];
}

void ca_uint8_array_set(ca_uint8 *ptr,ca_uint8 data,ca_uint32 index)
{
     ptr[index] = data;
}

ca_uint64 *ca_uint64_create(ca_uint64 data)
{
    ca_uint64 *u64_ptr = malloc(sizeof(ca_uint64));

    *u64_ptr = data;
    return u64_ptr;
}

ca_uint64 ca_uint64_get(ca_uint64 *ptr)
{
    printf("%llu\n",*ptr);
    return *ptr;
}

ca_uint64 *ca_uint64_array_create(ca_uint64 data,ca_uint32 size)
{
    ca_uint64 *u64_ptr = malloc(sizeof(ca_uint64)*size);
    int i;

    for( i =0; i < size;i++){
        u64_ptr[i] = data;
    }
    return u64_ptr;
}

ca_uint64 ca_uint64_array_get(ca_uint64 *ptr,ca_uint32 index)
{
    printf("%llu\n",ptr[index]);
    return ptr[index];
}

void ca_uint64_array_set(ca_uint64 *ptr,ca_uint64 data,ca_uint32 index)
{
     ptr[index] = data;
}

ca_l3_ip_addr_t *  ca_l3_ip_addr_create()
{
	ca_l3_ip_addr_t * cfg = malloc(sizeof(ca_l3_ip_addr_t)); 

    if(cfg != NULL){
        memset(cfg,0x00,sizeof(ca_l3_ip_addr_t));
    }

    return cfg;
}

void ca_l3_ip_addr_set_addr(ca_l3_ip_addr_t * cfg, ca_uint32 data, ca_uint32 index)
{
    if(cfg != NULL && index < 4)
    {
		cfg->addr[index] = data;
    }

}

ca_uint32  ca_l3_ip_addr_get_addr(ca_l3_ip_addr_t * cfg, ca_uint32 index)
{
	if(cfg == NULL){
        return 0;
    }
    return cfg->addr[index];
}

void ca_l3_ip_addr_set_ipv4_addr(ca_l3_ip_addr_t * cfg, ca_uint32 data)
{
    if(cfg != NULL )
    {
		cfg->ipv4_addr = data;
    }

}

ca_uint32 ca_l3_ip_addr_get_ipv4_addr(ca_l3_ip_addr_t * cfg)
{
	if(cfg == NULL){
        return 0;
    }
    return cfg->ipv4_addr;
}

void ca_l3_ip_addr_set_ipv6_addr(ca_l3_ip_addr_t * cfg, ca_uint32 data, ca_uint32 index)
{
    if(cfg != NULL && index < 4)
    {
    	cfg->ipv6_addr[index] = data;
    }

}

ca_uint32 ca_l3_ip_addr_get_ipv6_addr(ca_l3_ip_addr_t * cfg, ca_uint32 index)
{
	if(cfg == NULL){
        return NULL;
    }
    return cfg->ipv6_addr[index];
}

void ca_l3_ip_addr_dump(ca_l3_ip_addr_t * cfg)
{
	if(cfg == NULL){
        return;
    }

	int i;
    for(i=0;i<4;i++)
    {
    	printf("addr[%d] = 0x%x;\n", i, cfg->addr[i]);
    }
    
    printf("ipv4_addr = 0x%x;\n", cfg->ipv4_addr);
    
	for(i=0;i<4;i++)
    {
    	printf("ipv6_addr[%d] = 0x%x;\n", i, cfg->ipv6_addr[i]);
    }
	    
}

ca_ip_address_t *  ca_ip_address_create()
{
	ca_ip_address_t * cfg = malloc(sizeof(ca_ip_address_t)); 

    if(cfg != NULL){
        memset(cfg,0x00,sizeof(ca_ip_address_t));
    }

    return cfg;
}

void ca_ip_address_set_afi(ca_ip_address_t * cfg, unsigned int data)
{
	if(cfg != NULL)
	{
		cfg->afi = data;
	}
}

unsigned int ca_ip_address_get_afi(ca_ip_address_t * cfg)
{
	if(cfg == NULL){
        return 0;
    }
    return cfg->afi;
}

void ca_ip_address_set_ip_addr(ca_ip_address_t * cfg, ca_l3_ip_addr_t * ip_addr)
{
    if(cfg != NULL 
    	&& cfg->ip_addr.addr != NULL 
    	&& ip_addr->addr!= NULL){
    	
		int i;
    	for(i=0;i<4;i++)
    	{
    		cfg->ip_addr.addr[i] = ip_addr->addr[i];
    	}
    	
    }else if (cfg != NULL 
    			&& cfg->ip_addr.ipv6_addr != NULL 
    			&& ip_addr->ipv6_addr!= NULL){

    	int i;
    	for(i=0;i<4;i++)
    	{
    		cfg->ip_addr.ipv6_addr[i] = ip_addr->ipv6_addr[i];
    	}
		
    }else if(cfg != NULL){
		cfg->ip_addr.ipv4_addr = ip_addr->ipv4_addr;	
    }

}

ca_l3_ip_addr_t *  ca_ip_address_get_ip_addr(ca_ip_address_t * cfg)
{
    if(cfg != NULL){
		return &(cfg->ip_addr);
    }

}

void ca_ip_address_set_addr_len(ca_ip_address_t * cfg, unsigned char data)
{
    if(cfg != NULL)
    {
    	cfg->addr_len = data;	
    }

}

ca_uint8 ca_ip_address_get_addr_len(ca_ip_address_t * cfg)
{
    if(cfg == NULL)
    {
    	return 0;	
    }
    
    return cfg->addr_len;

}

void ca_ip_address_dump(ca_ip_address_t * cfg)
{
    if(cfg == NULL){
        return;
    }
	
    printf("afi = %#x \n",cfg->afi);
    ca_l3_ip_addr_dump(&cfg->ip_addr);
    printf("addr_len = %#x \n",cfg->addr_len);
    
}

ca_mac_addr_t * ca_mac_addr_create(
	ca_uint8 mac0, 
	ca_uint8 mac1, 
	ca_uint8 mac2, 
	ca_uint8 mac3, 
	ca_uint8 mac4, 
	ca_uint8 mac5)
{
    ca_uint8_t *mac = malloc(sizeof(ca_mac_addr_t));

	if(mac != NULL){
    	mac[0] = mac0;
    	mac[1] = mac1;
    	mac[2] = mac2;
    	mac[3] = mac3;
    	mac[4] = mac4;
    	mac[5] = mac5;
	}
	
    return (ca_mac_addr_t *)mac;
}

void ca_mac_addr_dump(ca_mac_addr_t * mac)
{
	ca_uint8_t * tmp_mac = (ca_uint8_t *)mac;

	if(mac == NULL){
    	return;
	}
	printf("mac= %02x %02x %02x %02x %02x %02x \n",
            tmp_mac[0],
            tmp_mac[1],
            tmp_mac[2],
            tmp_mac[3],
            tmp_mac[4],
            tmp_mac[5]);
}

void ca_mac_addr_set(
	ca_mac_addr_t * mac, 
	ca_uint8 mac0, 
	ca_uint8 mac1, 
	ca_uint8 mac2, 
	ca_uint8 mac3, 
	ca_uint8 mac4, 
	ca_uint8 mac5)
{
	ca_uint8_t * tmp_mac = (ca_uint8_t *)mac;
	if(mac == NULL){
    	return;
    }
    
    tmp_mac[0] = mac0;
    tmp_mac[1] = mac1;
    tmp_mac[2] = mac2;
    tmp_mac[3] = mac3;
    tmp_mac[4] = mac4;
    tmp_mac[5] = mac5;
}

ca_uint8  ca_mac_addr_get(ca_mac_addr_t *mac, int index )
{
	ca_uint8_t * tmp_mac = (ca_uint8_t *)mac;
	
	if(mac == NULL){
    	return 0;
    }
    
    return tmp_mac[index];
}

ca_iterator_t *  ca_iterator_create()
{
    ca_iterator_t * cfg = malloc(sizeof(ca_iterator_t)); 

    if(cfg != NULL){
        memset(cfg,0x00,sizeof(ca_iterator_t));
    }

    return cfg;
}

void ca_iterator_dump(ca_iterator_t * cfg)
{
    if(cfg == NULL){
        return;
    }

    printf("entry_count = %#x \n",cfg->entry_count);
    printf("entry_data = %p\n",cfg->entry_data);
    printf("user_buffer_size = %#x \n",cfg->user_buffer_size);
    printf("prev_handle = %p\n",cfg->prev_handle);

}

void ca_iterator_set_entry_count(ca_iterator_t * cfg,ca_uint32_t entry_count)
{
    if(cfg == NULL){
        return ;
    }

    cfg->entry_count = entry_count;
}

ca_uint32_t ca_iterator_get_entry_count(ca_iterator_t * cfg)
{
    if(cfg == NULL){
        return 0;
    }

    return cfg->entry_count;
}

void ca_iterator_set_entry_data(ca_iterator_t * cfg, ca_uint32 * entry_data)
{
    if(cfg == NULL){
        return ;
    }

    cfg->entry_data = entry_data;
}

ca_uint32 * ca_iterator_get_entry_data(ca_iterator_t * cfg)
{
    if(cfg == NULL){
        return 0;
    }

    return (ca_uint32 *)(cfg->entry_data);
}

void ca_iterator_set_user_buffer_size(ca_iterator_t * cfg,ca_uint32_t user_buffer_size)
{
    if(cfg == NULL){
        return ;
    }

    cfg->user_buffer_size = user_buffer_size;
}

ca_uint32_t ca_iterator_get_user_buffer_size(ca_iterator_t * cfg)
{
    if(cfg == NULL){
        return 0;
    }

    return cfg->user_buffer_size;
}

void ca_iterator_set_prev_handle(ca_iterator_t * cfg, ca_uint32 * prev_handle)
{
    if(cfg == NULL){
        return ;
    }

    cfg->prev_handle = prev_handle;
}

ca_uint32 * ca_iterator_get_prev_handle(ca_iterator_t * cfg)
{
    if(cfg == NULL){
        return 0;
    }

    return (ca_uint32 *)(cfg->prev_handle);
}

void ca_data_dump(ca_uint8_t * data, ca_uint32_t len)
{
    ca_uint32_t i = 0;
    if((data == NULL)||(len == 0)){
        return;
    }
    
    printf("DATA DUMP:\r\n");

    for(i=0; i< len; i=i+8)
    {
        printf("%02x %02x %02x %02x %02x %02x %02x %02x; \r\n", \
            *(data), *(data+1), *(data+2), *(data+3), *(data+4), *(data+5), *(data+6), *(data+7));
    }

    return;
}

