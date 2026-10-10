#ifndef _UTIL_AES_H_
#define _UTIL_AES_H_

#include "ca_types.h"


#define CA_AES_VAL_CHECK(val)         if(((val) > 16) || ((val) <= 0)) { \
                                          OS_PRINTF("parameter error!!val is :%d\n",val);\
                                          return CA_E_PARAM; \
                                      }
#define CA_AES_CHECK_DIRECTION(val)         if(((val) != 1) && ((val) != 2)) { \
                                                    OS_PRINTF("parameter error!!val is :%d\n",val);\
                                                    return CA_E_PARAM; \
                                                }

#define CA_AES_PTR_CHECK(val)         if(val == NULL) { \
                                          OS_PRINTF("parameter error!!val is :%p\n",val);\
                                          return CA_E_PARAM; \
                                      }

#define AES_BLOCK_SIZE      16
#define AES_ROUNDS          10  // 12, 14
#define AES_ROUND_KEY_SIZE  176 // AES-128 has 10 rounds, and there is a AddRoundKey before first round. (10+1)x16=176.

/*#define the macros below to 1/0 to enable/disable the mode of operation.

CBC enables AES128 encryption in CBC-mode of operation and handles 0-padding.
ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

The #ifndef-guard allows it to be configured before #include'ing or at compile time.*/

ca_status_t aes128_ecb_encrypt (
    CA_IN ca_uint8_t aes_key[],
    CA_IN ca_uint8_t key[],
    CA_OUT ca_uint8_t encryped_key[]);
ca_status_t aes128_ecb_decrypt (
    CA_IN ca_uint8_t aes_key[],
	CA_IN ca_uint8_t encryped_key[],
	CA_OUT ca_uint8_t key[]);

ca_status_t aes128_cmac_compute(
    CA_IN		ca_uint8_t				key[],
    CA_IN		ca_uint8_t				msg[],
    CA_IN		ca_uint32_t				msg_len,
    CA_IN		ca_uint8_t				mac_size,
    CA_OUT		ca_uint8_t				mac[]);

ca_status_t random_key_generate (
    CA_OUT ca_uint8_t rand_key[]);

#endif

