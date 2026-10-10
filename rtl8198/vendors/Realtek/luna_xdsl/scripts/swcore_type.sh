#!/bin/sh

swcore_vlan_fix()
{
	# add valn 0~4095 with member port 0,1,2,3,4 untag none
	# for vlan passthrough
	#vid=0
	#while [ $vid -le 100 ]
	#do
	#	echo $vid 0x1f 0x0 0 > /proc/rtl865x/cvlan
	#	vid=$((++vid))
	#done
	
	eval `mib get SW_PORT_TBL.0.VID`
	PORT0_VID=${VID:-46}
	eval `mib get SW_PORT_TBL.1.VID`
	PORT1_VID=${VID:-81}
	eval `mib get SW_PORT_TBL.2.VID`
	PORT2_VID=${VID:-82}
	eval `mib get SW_PORT_TBL.3.VID`
	PORT3_VID=${VID:-3879}
	# set vlan 46 with member port 0,4 untag port 0
	# set port 0 pvid 46
	echo $PORT0_VID 0x11 0x01 0 > /proc/rtl865x/cvlan
	echo 0 $PORT0_VID > /proc/rtl865x/pvid
	echo $PORT1_VID 0x12 0x02 0 > /proc/rtl865x/cvlan
	echo 1 $PORT1_VID > /proc/rtl865x/pvid
	echo $PORT2_VID 0x14 0x04 0 > /proc/rtl865x/cvlan
	echo 2 $PORT2_VID > /proc/rtl865x/pvid
	echo $PORT3_VID 0x18 0x08 0 > /proc/rtl865x/cvlan
	echo 3 $PORT3_VID > /proc/rtl865x/pvid
	#sarctl w 0xbb804410 0x7
	#echo delbc > /proc/rtl865x/l2
	#sarctl w 0xbb804428 0x408
}

swcore_no_learning()
{
	echo 9 0x1f 0x1f 0 > /proc/rtl865x/cvlan
	# enable learning limitation, the default learning count is 0
	sarctl w 0xbb804488 0x10001
	sarctl w 0xbb80448c 0x10001
	sarctl w 0xbb804490 0x10001
	sarctl w 0xbb804410 0x0 /* disable L2 */
	sarctl w 0xbb804400 0x20 /* enable fast aging */
	sleep 2
	sarctl w 0xbb804410 0x17 /* enable L2 */
	#echo clean_L2 > /proc/rtl865x/acl_mode
}

swcore_vtuo_hub()
{
	#echo 9 0x3f 0x3f 0 > /proc/rtl865x/cvlan
	# enable learning limitation, the default learning count is 0
	sarctl w 0xbb804488 0x10001
	sarctl w 0xbb80448c 0x10001
	sarctl w 0xbb804490 0x10001
	sarctl w 0xbb804410 0x0 /* disable L2 */
	sarctl w 0xbb804400 0x20 /* enable fast aging */
	sleep 2
	sarctl w 0xbb804410 0x17 /* enable L2 */
	#echo clean_L2 > /proc/rtl865x/acl_mode
}

eval `mib get DEVICE_TYPE`

if [ $DEVICE_TYPE = 100 ]; then
	echo vlan_untag_mode
	swcore_vlan_fix
elif [ $DEVICE_TYPE = 101 ]; then
	echo hub_mode
	swcore_no_learning
elif [ $DEVICE_TYPE = 102 ]; then
	echo vtuo_mode
elif [ $DEVICE_TYPE = 9 ]; then
	echo vtuo_hub_mode
	swcore_vtuo_hub
fi
# for others, it is vtuo switch mode

