#!/bin/sh

MULTI_KEY=n
IC8671=n
IC8672=y
key="0xa0000003"
echo "key:"$key
if [ "$CONFIG_BLK_DEV_INITRD" = "y" ]
then
make linux
fi

flash_base="0x$CONFIG_FLASH_BASE"
uboot_size="$CONFIG_BOOT_SIZE"
ImageOffsetHex=`echo "obase=16;ibase=10; $(($flash_base+$uboot_size+0xe0000))" | bc`

${CROSS_COMPILE}strip vmlinux --remove-section=.bss
${CROSS_COMPILE}objcopy -S -O binary vmlinux vmlinux.bin
../tools/lzma e vmlinux.bin vmlinux.lzma
ls vmlinux.lzma -l
../tools/packimg/kfspad ../rootfs vmlinux.lzma vmbin.img
../tools/packimg/genhead  -i vmbin.img -o vm.hdr -k $key -f 0x$ImageOffsetHex  -a 0x80000000 -e 0x80000000
cat vm.hdr vmbin.img > vm.img
ls -al vm.img
