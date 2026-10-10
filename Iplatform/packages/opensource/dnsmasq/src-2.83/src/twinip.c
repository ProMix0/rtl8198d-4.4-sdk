#include "dnsmasq.h"

#define TWINIP_PROC "/proc/twinip"

struct twinip_param twinip;

static void get_param_from_uci()
{
    struct uci_context *ctx = NULL;
    struct uci_ptr ptr;
    char enable_path[] = "nat.dmz.twinip_enable";
    char mac_path[] = "nat.dmz.mac";
    char enable[10] = "";
    char mac[20] = "";

    ctx = uci_alloc_context();
    if (!ctx)
    {
        return;
    }

    // get enable
    if (uci_lookup_ptr(ctx, &ptr, enable_path, true) != UCI_OK)
    {
        uci_free_context(ctx);
        return;
    }
    if (!(ptr.flags & UCI_LOOKUP_COMPLETE))
    {
        uci_free_context(ctx);
        return;
    }
    snprintf(enable, sizeof(enable), "%s", ptr.o->v.string);

    // get mac
    if (uci_lookup_ptr(ctx, &ptr, mac_path, true) != UCI_OK)
    {
        uci_free_context(ctx);
        return;
    }
    if (!(ptr.flags & UCI_LOOKUP_COMPLETE))
    {
        uci_free_context(ctx);
        return;
    }
    snprintf(mac, sizeof(mac), "%s", ptr.o->v.string);

    uci_free_context(ctx);

    if (!strcmp(enable, "on"))
    {
        twinip.uci_twinip_enable = 1;
    }
    else
    {
        twinip.uci_twinip_enable = 0;
    }

    sscanf(mac, "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
           &twinip.twinip_mac[0], &twinip.twinip_mac[1], &twinip.twinip_mac[2], &twinip.twinip_mac[3], &twinip.twinip_mac[4], &twinip.twinip_mac[5]);

    return;
}

void twinip_init()
{
    // unsigned char tmp_mac[6] = {0xfc,0x34,0x97,0xbc,0xfc,0xc4};
    char buff[1024] = "";
    unsigned char tmp_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    struct in_addr in_tmp;
    twinip.enable = 0;
    twinip.lease_time = 2;
    twinip.fake_ip = inet_addr("0.0.0.0");
    twinip.gateway_ip = inet_addr("0.0.0.0");
    twinip.wan_ip = inet_addr("0.0.0.0");
    twinip.wan_mask = inet_addr("255.255.255.0");
    twinip.dns = inet_addr("0.0.0.0");
    twinip.uci_twinip_enable = 0;
    memset(twinip.twinip_mac, 0, ETH_ALEN);
    memcpy(twinip.twinip_mac, tmp_mac, ETH_ALEN);

    twinip_param_get();
    get_param_from_uci();

    if (twinip.enable)
    {
        in_tmp.s_addr = twinip.fake_ip;
        sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X,%s,%im",
                twinip.twinip_mac[0], twinip.twinip_mac[1], twinip.twinip_mac[2], twinip.twinip_mac[3], twinip.twinip_mac[4], twinip.twinip_mac[5], inet_ntoa(in_tmp), twinip.lease_time);

        one_opt((int)'G', buff, "dhcp-host", "error", 0, 0);
        // system(buff);
    }

    return;
}

void twinip_param_get()
{
    char buff[512] = "";
    char tmp[20] = "";
    FILE *fp = fopen(TWINIP_PROC, "r");
    if (fp == NULL)
    {
        twinip.enable = 0;
        return;
    }

    while (fgets(buff, sizeof(buff), fp) != NULL)
    {
        if (strstr(buff, "enable"))
        {
            sscanf(buff, "enable:%d", &twinip.enable);
            if (0 == twinip.enable)
            {
                break;
            }
        }
        else if (strstr(buff, "gatewayip"))
        {
            memset(tmp, 0, sizeof(tmp));
            sscanf(buff, "gatewayip:%s", tmp);

            twinip.gateway_ip = inet_addr(tmp);
        }
        else if (strstr(buff, "wanip"))
        {
            memset(tmp, 0, sizeof(tmp));
            sscanf(buff, "wanip:%s", tmp);
            twinip.wan_ip = inet_addr(tmp);
        }
        else if (strstr(buff, "fakeip"))
        {
            memset(tmp, 0, sizeof(tmp));
            sscanf(buff, "fakeip:%s", tmp);
            twinip.fake_ip = inet_addr(tmp);
        }
        else if (strstr(buff, "wanmask"))
        {
            memset(tmp, 0, sizeof(tmp));
            sscanf(buff, "wanmask:%s", tmp);
            twinip.wan_mask = inet_addr(tmp);
        }
        else if (strstr(buff, "wandns"))
        {
            memset(tmp, 0, sizeof(tmp));
            sscanf(buff, "wandns:%s", tmp);
            twinip.dns = inet_addr(tmp);
        }
    }
    fclose(fp);
    return;
}