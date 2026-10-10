#!/bin/sh
PIDFILE=/var/run/udhcpd.pid
#eval `flash get UPNP`
flash get UPNP
if [ $1 = 'bound' ] || [ $1 = 'renew' ]; then
	/var/udhcpc/udhcpc.$interface	
	if [ "$ip" != '' ]; then
		if [ $UPNP = 1 ]; then 
			/bin/upnpctrl down $interface br0
			/bin/upnpctrl up $interface br0
		fi
		/bin/updateddctrl $interface 1
		/bin/ethctl route add		
#		if [ -f /var/run/udhcpd.pid ]; then
#			PID=`cat $PIDFILE`
#			echo "Kick dhcrelay to sync interface-2"
#			kill -17 $PID
#		fi
		
		if [ $1 = 'renew' ] && [ -f /var/run/systemd.pid ]; then
			if [ "$dns" ] && [ "$ip" ]; then
				for i in $dns
				do
				if [ $i = '0.0.0.0' ]|| [ $i = '255.255.255.255' ]; then
				continue
				fi
				
				dns_ip=`echo $i@$ip`
				if ! grep $dns_ip /var/dnsmasq.conf > /dev/null
				then
					PID=`cat /var/run/systemd.pid`
					kill -SIGUSR1 $PID
					break
				fi
				done
			fi	
		fi
	fi
elif [ $1 = 'deconfig' ]; then
	/etc/scripts/udhcpc.deconfig
	
elif [ $1 = 'nak' ]; then
	echo "dhcp nak event occurs"
fi
