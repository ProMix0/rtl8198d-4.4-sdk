#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/netdevice.h>
#include "ethctl_implement.h"
#include <bspchip.h>
#include <bspgpio.h>

#ifdef CONFIG_RTK_SOC_RTL9607C
void eth_ctl(struct eth_arg * arg){
	unsigned char cmd1;
	unsigned int cmd2,cmd3;
	unsigned short cmd4;
	unsigned int cmd5;
	struct net_device *dev;	
	
	cmd1=arg->cmd;
	cmd2=arg->cmd2;
	cmd3=arg->cmd3;
	cmd4=(unsigned short)arg->cmd4;
	cmd5 = arg->cmd5;
		
	switch(cmd1){
		case 53: //enable_usb0_wan
			dev = dev_get_by_name(&init_net,"usb0");
			if(dev==NULL)
			{
				printk("Error ! can't find usb0 device\n");
				return;
			}

			if(cmd2==1){		
				printk("Enable Wan flag for usb0\n");
				dev->priv_flags |= IFF_DOMAIN_WAN;
			}
			else{
				printk("Disable Wan flag for usb0\n");
				dev->priv_flags &= ~IFF_DOMAIN_WAN;
			}
			dev_put(dev);		
			break;
        case 54://change nas0 carrier
		    dev = dev_get_by_name(&init_net,"nas0");
            if (dev == NULL)
            {
				printk("Error ! can't find nas0 device\n");
				return;
			}
            if (cmd2 == 1)//carrier on
            {
                if (!netif_carrier_ok(dev))
				{
					netif_carrier_on(dev);
				}
            }
            else if (cmd2 == 0)//carrier off
            {
                if (netif_carrier_ok(dev))
				{
					netif_carrier_off(dev);
				}
            }
            else
            {
                if (netif_carrier_ok(dev))
                    printk("nas0 carrier on\n");
                else
                    printk("nas0 carrier off\n");
            }
            dev_put(dev);
            break;
		default:
			printk("unknown cmd(%d)\n",cmd1);
	}
}
#else /*CONFIG_RTK_SOC_RTL9607C*/
void eth_ctl(struct eth_arg * arg) {
	printk("%s(%d): \n",__func__,__LINE__);
}
#endif 

