#!/bin/bash
echo "insmod rtl8192cd.ko"
insmod /etc/conf/rtl8192cd.ko
#setmib1 2ndchosset 2
#setmib1 use40M 1
#echo "sleep 10 secs"
#sleep 10
hostapd /etc/conf/92fehapdconf -ddd -B
