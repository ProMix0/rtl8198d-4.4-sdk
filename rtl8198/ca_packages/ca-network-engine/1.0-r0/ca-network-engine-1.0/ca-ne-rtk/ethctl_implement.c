#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/netdevice.h>
#include "ethctl_implement.h"
#include <bspchip.h>


int enable_ipqos=0;

void eth_ctl(struct eth_arg * arg)
{
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

	switch(cmd1)
	{
		case 1:		//tx
		case 2:		//rx
		case 3:		//phy on
		case 4:		//phy off
		case 5:		//read phy
		case 6:		//write phy
		case 7:		//LED all on
		case 8:		//iocmd set
			/* TODO: */
			break;
		case 9:		//gpio read
			/* TODO: */
			break;	
		case 10:  //GPIO test
	    	/* TODO: */
	    	break;
		// Added by Mason Yu for PPP LED		
		case 12:  //PPP on	
		#if defined(CONFIG_STD_LED)
			gpio_LED_PPP(1);
		#elif defined(CONFIG_GPIO_LED_CHT_E8B)           
            gpio_LED_PPP(1);
		#endif
			break;
		
		// Added by Mason Yu for PPP LED
		case 13:  //PPP off	
		#ifdef CONFIG_GPIO_LED_CHT_E8B			
            g_ppp_up--;
			if (g_ppp_up<0)
				g_ppp_up = 0;
		#endif
		#if defined(CONFIG_STD_LED)				
			gpio_LED_PPP(0);
		#elif defined(CONFIG_GPIO_LED_CHT_E8B)
                    if (g_ppp_up == 0)
                       gpio_LED_PPP(0);
		#endif 	
			break;
			
		case 14:	//watchdog command
		case 15:	//internet up
		case 16:	//internet off
			/* TODO: */
            break;
		case 21: // conntrack killall
			/* TODO: */
			break;
		case 29:	// dump vlan info
		case 30:	// dump igmp info
		case 41:	// nic phy power up
		case 42:	// nic phy power down
			/* TODO: */
			break;
			
		case 50:
			if(cmd2==1){
				printk("enable ipqos!!!\n");
				enable_ipqos=1;
			}
			else{
				printk("disable ipqos!!!\n");
				enable_ipqos=0;
			}
			break;

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
				printk("Error ! can't find usb0 device\n");
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
#ifdef CONFIG_RTK_HOST_SPEEDUP
		case 55://get hostspeedup stream stats
			{
				unsigned int sip, dip;
				unsigned short sport, dport;
				sip = cmd2;
				dip = cmd3;
				sport = cmd4;
				dport = cmd5;
				extern int HostSpeedUP_Stat_get(u32 dstip, u32 srcip, u16 dport, u16 sport);				
				arg->cmd2 = HostSpeedUP_Stat_get(dip, sip, dport, sport);
			}
			break;
#endif
		default:
			printk("error cmd\n");
	}
}

