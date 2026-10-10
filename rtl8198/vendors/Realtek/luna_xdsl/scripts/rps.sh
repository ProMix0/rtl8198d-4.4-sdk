#!/bin/sh

#if [ "$#" -ne 1 ]; then
#	echo "$0 <on|off> $# $1" 
#	exit 1
#fi

case "$1" in
	on)
		RPS_cpus=4
		RPS_flow_cnt=4096
		WLAN_irq_affn1=8
		WLAN_irq_affn2=2
		;;
	off)
		RPS_cpus=0
		RPS_flow_cnt=0
		WLAN_irq_affn1=F		
		WLAN_irq_affn2=F		
		;;
	*)
		echo "$0 <on|off>"
		exit 1
		;;
esac

for f in /sys/class/net/*; do 
	SET=1
	case "$f" in
	/sys/class/net/eth*)
		;;
	/sys/class/net/wlan*)
		;;
	/sys/class/net/nas*)
		;;
	/sys/class/net/ptm*)
		;;
	/sys/class/net/br*)
		;;			
	*)
		SET=0
		;;
	esac
	
	if [ "$SET" -eq 1 ]; then
		echo $RPS_cpus > $f/queues/rx-0/rps_cpus
		echo $RPS_flow_cnt > $f/queues/rx-0/rps_flow_cnt
	fi 
done

echo $WLAN_irq_affn1 > /proc/irq/31/smp_affinity
echo $WLAN_irq_affn2 > /proc/irq/32/smp_affinity
