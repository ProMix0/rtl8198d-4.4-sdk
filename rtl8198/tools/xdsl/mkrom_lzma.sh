#!/bin/sh

LINUX_BIN=linux.bin
LINUX_COMPRESS=$LINUX_BIN.lzma

$OBJCOPY -O binary -R .bss -R .note -R .comment -S vmlinux linux.bin
[ -e $LINUX_COMPRESS ] && rm $LINUX_COMPRESS
../tools/u_boot_lzma e $LINUX_BIN $LINUX_COMPRESS
../tools/u_boot_mkimage -A mips -O linux -T kernel -C lzma -a 80000000 -e 80000000 -n "Linux Kernel Image" -d $LINUX_COMPRESS uImage

if [ "$CONFIG_MTD_NAND_LUNA" = "y" ] || [ "$CONFIG_MTD_SPI_NAND_LUNA" = "y" ]
then
	echo 
	echo CONFIG_MTD_NAND_RTK_LUNA=$CONFIG_MTD_NAND_RTK_LUNA
	echo Aligning uImage to 2k/page boundary for NAND platform
	mv uImage uImage.orig
	sz=`stat --printf="%s" uImage.orig`
	pagecnt=$(( (sz+2047) / 2048 )) 
	dd if=uImage.orig ibs=2k count=$pagecnt of=uImage conv=sync 
	ls -l uImage.orig uImage
	echo 
fi

cp uImage $IMAGEDIR

key="0xa0000203"
echo "key:"$key

flash_base="0x$CONFIG_FLASH_BASE"
ImageOffsetHex=`echo "obase=16;ibase=10; $(($flash_base))" | bc`

../tools/packimg/genhead -i uImage -o uImage.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
cat uImage.hdr uImage > uImage_new
ls -al uImage_new


key="0xa0000403"
echo "key:"$key
../tools/packimg/genhead -i ../rootfs -o rootfs.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
cat rootfs.hdr ../rootfs > rootfs_new
ls -al rootfs_new

cat uImage_new ../rootfs > vm.img
ls -al vm.img
cp vm.img $IMAGEDIR

