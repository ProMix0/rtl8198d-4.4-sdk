#!/bin/sh

interface_id_start=0
interface_id_end=2
interface_max=$((interface_id_end+1))

for i in `seq $interface_id_start $interface_id_end`
do
	if [ ! -d  "/proc/wlan$i" ];
	then
		interface_max=$i
		break;
	fi
done

if [ $interface_max -gt 0 ];
then
	cp /etc/ther.conf /var/ther.conf
	killall ther_control
	ther_control "$i" &
fi
