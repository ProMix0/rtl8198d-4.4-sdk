#ifndef __CLI_UTILS_H___
#define __CLI_UTILS_H___

#define CLI_ERROR(arg,...)  printf(arg,##__VA_ARGS__)

#define CLI_ASSERT(x)  \
    do{ \
        if (!(x))  { \
            CLI_ERROR("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
            return ; \
        } \
    }while(0)

#define CLI_ASSERT_RET(x,y) \
    do{ \
        if (!(x))  { \
            CLI_ERROR("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
            return (y); \
        } \
    }while(0)



#ifndef TRUE
#define TRUE                  1
#endif

#ifndef FALSE
#define FALSE                 0
#endif

typedef cli_status_t cli_func_t();

#define CLI_MALLOC(size)        malloc((size))
#define CLI_FREE(ptr)           if((ptr) != NULL){ free((ptr)); (ptr) = NULL; }
#define CLI_REALLOC(ptr,size)   realloc((ptr),(size))


cli_int8 * cli_strtrim(cli_int8 * str);

void cli_strncat(
        cli_int8     **string,
        const cli_int8   *text,
        cli_uint32  len);

void cli_strcat(
        cli_int8 **string,
        const cli_int8*text);

cli_int8 *cli_strdup(const cli_int8 *string);

cli_int8 * cli_strndup(const cli_int8 *string, cli_uint32 length);

cli_int8 * cli_strrealloc(cli_int8 *str, cli_uint32 new_len);
void cli_strfree(cli_int8 * string);
cli_boolean cli_isupper(cli_int8 * str);
cli_boolean cli_iskeyword(
        cli_int8 * str);

cli_boolean cli_is_digital(
        cli_int8 * str);

cli_uint64 cli_str2num(
        cli_int8 * str);
#endif
