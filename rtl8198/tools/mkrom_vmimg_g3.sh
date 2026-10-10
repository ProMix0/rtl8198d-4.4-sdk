#!/bin/sh

is_yueme=`cat ../linux-4.4.x/.config | grep "CONFIG_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cmcc=`cat ../linux-4.4.x/.config | grep "CONFIG_CMCC=" | sed s/[^y]//g | sed s/\"//g`
is_cu_yueme=`cat ../linux-4.4.x/.config | grep "CONFIG_CU_BASEON_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cu_cmcc=`cat ../linux-4.4.x/.config | grep "CONFIG_CU_BASEON_CMCC=" | sed s/[^y]//g | sed s/\"//g`
extra_img=""

use_ubifs=1
if [ "$1" = "-ubifs" ]; then
	use_ubifs=0
fi

key="0xa0000443"
#echo "key:"$key

#if no one export DTB_FILE
if [ "x$DTB_FILE" = "x" ]; then
DTB_FILE="ca7774-engboard.dtb"
fi

if [ ! -d ./.tmp ]; then
mkdir .tmp
fi

flash_base=0xbd000000
ImageOffsetHex=`echo "obase=16;ibase=10; $(($flash_base))" | bc`

../tools/packimg/genhead -i ${DTB_FILE} -o ./.tmp/dtb.hdr -k $key -f $ImageOffsetHex
cat ./.tmp/dtb.hdr ${DTB_FILE} > ./.tmp/${DTB_FILE}_new
#ls -al ./.tmp/${DTB_FILE}_new

key="0xa0000203"
#echo "key:"$key
../tools/packimg/genhead -i Image.lzma -o ./.tmp/Image.hdr -k $key -f $ImageOffsetHex
cat ./.tmp/Image.hdr Image.lzma > ./.tmp/Image.lzma_new
#ls -al ./.tmp/Image.lzma_new


key="0xa0000403"
#echo "key:"$key
if [ "$use_ubifs" = "1" ]; then
../tools/packimg/genhead -i ./rootfs_G3_1.ubi -o ./.tmp/rootfs.hdr -k $key -f $ImageOffsetHex
cat ./.tmp/rootfs.hdr ./rootfs_G3_1.ubi > ./.tmp/rootfs_new
else
../tools/packimg/genhead -i ./rootfs -o ./.tmp/rootfs.hdr -k $key -f $ImageOffsetHex
cat ./.tmp/rootfs.hdr ./rootfs > ./.tmp/rootfs_new
fi
#ls -al ./.tmp/rootfs_new

if [ "y" = "$is_cmcc" ] || [ "y" = "$is_cu_cmcc" ]; then 
	key="0xCAFEBABE"
	echo "key:"$key
	../tools/packimg/genhead -i ../images/osgi/osgi.img -o ./.tmp/osgi.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat ./.tmp/osgi.hdr ../images/osgi/osgi.img > ./.tmp/osgi_new
	#ls -al osgi_new
	extra_img+=" ./.tmp/osgi_new"
fi



cat  ./.tmp/${DTB_FILE}_new ./.tmp/Image.lzma_new ./.tmp/rootfs_new ${extra_img} > vm.img
ls -al vm.img

#rm ./.tmp/*
