#!/bin/sh

is_yueme=`cat ../linux-4.4.x/.config | grep "CONFIG_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cmcc=`cat ../linux-4.4.x/.config | grep "CONFIG_CMCC=" | sed s/[^y]//g | sed s/\"//g`
is_cu=`cat ../linux-4.4.x/.config | grep "CONFIG_CU=" | sed s/[^y]//g | sed s/\"//g`
is_cu_yueme=`cat ../linux-4.4.x/.config | grep "CONFIG_CU_BASEON_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cu_cmcc=`cat ../linux-4.4.x/.config | grep "CONFIG_CU_BASEON_CMCC=" | sed s/[^y]//g | sed s/\"//g`
extra_img=""

key="0xa0000443"
#echo "key:"$key

if [ ! -d ./.tmp ]; then
mkdir .tmp
fi

kimg=uImage
rootimg=rootfs

flash_base=0xbd000000
ImageOffsetHex=`echo "obase=16;ibase=10; $(($flash_base))" | bc`

key="0xa0000203"
#echo "key:"$key
../tools/packimg/genhead -i ${kimg} -o ./.tmp/${kimg}.hdr -k $key -f $ImageOffsetHex
cat ./.tmp/${kimg}.hdr ${kimg} > ./.tmp/${kimg}_new
#ls -al ./.tmp/Image.lzma_new


key="0xa0000403"
#echo "key:"$key
../tools/packimg/genhead -i ./${rootimg} -o ./.tmp/${rootimg}.hdr -k $key -f $ImageOffsetHex
cat ./.tmp/${rootimg}.hdr ./${rootimg} > ./.tmp/${rootimg}_new
#ls -al ./.tmp/rootfs_new

if [ "y" = "$is_cmcc" ] || [ "y" = "$is_cu_cmcc" ]; then
    key="0xCAFEBABE"
    echo "key:"$key
    ../tools/packimg/genhead -i ../images/osgi/osgi.img -o ./.tmp/osgi.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
    cat ./.tmp/osgi.hdr ../images/osgi/osgi.img > ./.tmp/osgi_new
    #ls -al osgi_new
    extra_img+=" ./.tmp/osgi_new"
fi

cat ./.tmp/${kimg}_new ./.tmp/${rootimg}_new ${extra_img} > vm.img
ls -al vm.img

#rm ./.tmp/*
