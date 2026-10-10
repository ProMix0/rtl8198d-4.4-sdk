#include "ca_types.h"
#ifdef CLI_KERNEL_SPACE
#include <linux/module.h>
#else
#include <string.h>
#endif
#include "cli_cmn.h"

extern void cli_plat_init(void);
extern void cli_aal_init(void);


int cli_atoi(char * str)
{
    int value = 0 ;
    if(str == NULL){
        return 0;
    }

    if(strlen(str) <= 2  || !(str[0] == '0' && str[1] =='x') ) {
        sscanf(str,"%d",&value);
    }else{
        sscanf(str,"%x",&value);
    }

    return value;
}

ca_uint64_t cli_str2ll(char *str)
{
    ca_uint64_t number = 0;
    char *c = str;
    ca_uint64_t base = 10;

    if(str == NULL)
        return 0;

    if(strlen((const char*)str) >= 2 && str[0] == '0' && str[1] == 'x')
    {
        if(strlen((const char*)str) == 2)
        {
            return 0;
        }
        c += 2;
        base = 16;
    }

    while(*c != '\0')
    {
        if(base == 10 && !(*c >= '0' &&  *c <= '9'))
            return 0;

        if(*c >= '0' &&  *c <= '9')
        {
            number = number*base + (unsigned) (long long)(*c - '0');
        }
        else if(*c >= 'A' &&  *c <= 'F')
        {
            number = number*base + (unsigned) (long long)(*c - 'A') + 10;
        }
        else if(*c >= 'a' &&  *c <= 'f')
        {
            number = number*base + (unsigned) (long long)(*c - 'a') + 10;
        }
        else
            return 0;

        c++;
    }

    return number;
}


ca_int32_t cli_plat_atoi(ca_int8_t *str)
{
    ca_int8_t *c = str;
    ca_int32_t num = 0;
    
    if(str == NULL)
        return num;

    while(*c >= '0' && *c <= '9' && *c != '\0')
    {
        num = (num*10)+*c-'0';
        c++;
    }

    return num;
}

ca_status_t ca_str2ca_ip(ca_int8_t *ip_str , ca_ip_address_t *ipaddr)
{
    ca_int8_t *c = NULL;
    ca_int32_t parse_state = 0;
    ca_int32_t idx = 0;
    ca_int32_t ip[4]={0,0,0,0};

    if(ip_str == NULL || ipaddr == NULL)
        return CA_E_PARAM;

    /******************************************************************
      parse_state DESC
      ------------------------------------------------------
      0 init status
      1 convert one byte, ignore the following digit
      2 previous char is '.'
      ------------------------------------------------------
     ******************************************************************/

    c = ip_str;
    while(*c != '\0')
    {
        if((parse_state == 0)&&(*c == '0'))
            return CA_E_PARAM;

        switch(*c)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if(parse_state == 0 || parse_state == 2)
                {
                    ip[idx++] = cli_plat_atoi(c);
                    if(ip[idx-1] > 255 || ip[idx-1] < 0)
                    return CA_E_PARAM;
                }
                c++;
                parse_state = 1;
                break;
            case '.':
                if(parse_state != 1)
                    return CA_E_PARAM;

                c++;
                parse_state = 2;
                break;
            default:
                return CA_E_PARAM;
        }
    }

    if(parse_state != 1)
        return CA_E_PARAM;

    if(idx != 4)
        return CA_E_PARAM;
    
    ipaddr->afi = CA_IPV4;
    ipaddr->ip_addr.ipv4_addr=(unsigned int)(((ip[0]<<24)&0xFF000000)|((ip[1]<<16)&0xFF0000)|((ip[2]<<8)&0xFF00)|((ip[3]<<0)&0xFF)); 
    ipaddr->addr_len = 32;
    
    return CA_E_OK;
}

ca_status_t ca_str2ca_mac(char * str, ca_mac_addr_t * mac)
{
    ca_int32_t mac1,mac2,mac3,mac4,mac5,mac6;
    ca_uint32_t num;
    if( str == NULL || mac == NULL){
        return CA_E_PARAM;
    }

    num = sscanf(str,"%2x:%2x:%2x:%2x:%2x:%2x",
            &mac1,
            &mac2,
            &mac3,
            &mac4,
            &mac5,
            &mac6);

    if(num == 6){
        (*mac)[0] = mac1;
        (*mac)[1] = mac2;
        (*mac)[2] = mac3;
        (*mac)[3] = mac4;
        (*mac)[4] = mac5;
        (*mac)[5] = mac6;
        return CA_E_OK;
    }

    return CA_E_PARAM;

}


int cli_k_product_init(void)
{
#ifdef CLI_KERNEL_SPACE
    cli_kioctl_init();
#endif
  //  cli_plat_init();
    cli_aal_init();

    
    return 0;
}

#ifdef CLI_KERNEL_SPACE
static void cli_k_product_exit(void)
{
#ifdef CONFIG_LUNA_G3_SERIES
    cli_kioctl_exit();
#endif
}

MODULE_LICENSE("GPL");
module_init(cli_k_product_init);
module_exit(cli_k_product_exit);
#endif

