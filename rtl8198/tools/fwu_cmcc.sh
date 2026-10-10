#!/bin/sh

# luna firmware upgrade  script
# $1 image destination (0 or 1)
# Kernel and root file system images are assumed to be located at the same directory named uImage and rootfs respectively
# ToDo: use arugements to refer to kernel/rootfs location.

########### G3 ##########################################################
### dtb    : ca7774-engboard.dtb or ca8276 Device Tree Binary ###########
### kimage : Image or Image.lzma
### rootfs : rootfs_G3_1.ubi
#########################################################################
d_img="dtb"
k_img="kimage"
r_img="rootfs"
img_ver="fwu_ver"
md5_cmp="md5.txt"
md5_cmd="/bin/md5sum"
#md5 run-time result
md5_tmp="md5_tmp"
md5_rt_result="md5_rt_result.txt"
new_fw_ver="new_fw_ver.txt"
cur_fw_ver="cur_fw_ver.txt"
env_sw_ver="env_sw_ver.txt"
hw_ver_file="hw_ver"
skip_hwver_check="/tmp/skip_hwver_check"

# For CMCC
o_img="osgi.img"
osgi_upgraded=0
osgi_install_prebundle=0
install_prebundle_flag="install_prebundle_flag"
osgi_partition_prebundle_dir="/usr/local/class/prebundle"
osgi_var_prebundle_dir="/var/osgi_app/prebundle"
osgi_var_runbundle_dir="/var/osgi_app/bundle"
osgi_var_intall_prebundle="/var/osgi_app/install_prebundle_flag"
osgi_var_no_osgi_partition="/var/osgi_app/osgi_no_Partition"

# For YueMe framework
framework_img="framework.img"
framework_sh="framework.sh"
framework_upgraded=0

# Stop this script upon any error
# set -e

if [ "`tar -tf $2 $o_img`" = "$o_img" ]; then
    osgi_upgraded=1
    rm -rf $osgi_var_no_osgi_partition
else
    touch $osgi_var_no_osgi_partition
fi

if [ "`tar -tf $2 $install_prebundle_flag`" = "$install_prebundle_flag" ]; then
    osgi_install_prebundle=1
    touch $osgi_var_intall_prebundle
    flash set CMCC_JAR_LINK 0
fi
if [ -d $osgi_var_prebundle_dir ]; then
	echo "/var/osgi_app/prebundle exist!"
else
	mkdir $osgi_var_prebundle_dir
	cp -rf $osgi_partition_prebundle_dir/* $osgi_var_prebundle_dir
	rm -rf $osgi_var_runbundle_dir
	flash set CMCC_JAR_LINK 0
	echo "Copy prebundles from OSGI partition to /var/osgi_app/prebundle"
fi

if [ "`tar -tf $2 $framework_sh`" = "$framework_sh" ] && [ "`tar -tf $2 $framework_img`" = "$framework_img" ]; then
    echo "Updaing framework from $2"
    tar -xf $2 $framework_sh
    grep $framework_sh $md5_cmp > $md5_tmp
    $md5_cmd $framework_sh > $md5_rt_result
    diff $md5_rt_result $md5_tmp

    if [ $? != 0 ]; then
        echo "$framework_sh md5_sum inconsistent, aborted image updating !"
        exit 1
    fi

    # Run firmware upgrade script extracted from image tar ball
    sh $framework_sh $2
    framework_upgraded=1
fi

if [ "`tar -tf $2 $k_img`" = '' ] && [ $framework_upgraded = 1 ]; then
    echo "No uImage for upgrading, skip"
    exit 2
fi

echo "Updating image $1 with file $2"

# Find out kernel/rootfs mtd partition according to image destination
d_mtd="/dev/"`cat /proc/mtd | grep \"DTB"$1"\" | sed 's/:.*$//g'`
k_mtd="/dev/"`cat /proc/mtd | grep \"k"$1"\" | sed 's/:.*$//g'`
r_mtd="/dev/"`cat /proc/mtd | grep \"r"$1"\" | sed 's/:.*$//g'`
o_mtd="/dev/"`cat /proc/mtd | grep \"osgi\" | sed 's/:.*$//g'`
d_mtd_size=`cat /proc/mtd | grep \"DTB"$1"\" | sed 's/^.*: //g' | sed 's/ .*$//g'`
k_mtd_size=`cat /proc/mtd | grep \"k"$1"\" | sed 's/^.*: //g' | sed 's/ .*$//g'`
r_mtd_size=`cat /proc/mtd | grep \"r"$1"\" | sed 's/^.*: //g' | sed 's/ .*$//g'`
o_mtd_size=`cat /proc/mtd | grep \"osgi\" | sed 's/^.*: //g' | sed 's/ .*$//g'`
echo "DTB image is located at $d_mtd"
echo "kernel image is located at $k_mtd"
echo "rootfs image is located at $r_mtd"
echo "osgi image is located at $o_mtd"

if [ -f $skip_hwver_check ]; then
    echo "Skip HW_VER check!!"
else
    img_hw_ver=`tar -xf $2 $hw_ver_file -O`
    mib_hw_ver=`flash get HW_HWVER | sed s/HW_HWVER=//g`
    if [ "$img_hw_ver" = "skip" ]; then
        echo "skip HW_VER check!!"
    else
        echo "img_hw_ver=$img_hw_ver mib_hw_ver=$mib_hw_ver"
        if [ "$img_hw_ver" != "$mib_hw_ver" ]; then
            echo "HW_VER $img_hw_ver inconsistent, aborted image updating !"
            exit 1
        fi
    fi
fi



# Extract DTB image
tar -xf $2 $d_img -O | md5sum | sed 's/-/'$d_img'/g' > $md5_rt_result
# Check integrity
grep $d_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    echo "$d_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

# Extract kernel image
tar -xf $2 $k_img -O | md5sum | sed 's/-/'$k_img'/g' > $md5_rt_result
# Check integrity
grep $k_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    echo "$k_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

# Extract rootfs image
tar -xf $2 $r_img -O | md5sum | sed 's/-/'$r_img'/g' > $md5_rt_result
# Check integrity
grep $r_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    # rm $r_img
    echo "$r_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

if [ $osgi_upgraded = 1 ]; then
# Extract osgi image
tar -xf $2 $o_img -O | md5sum | sed 's/-/'$o_img'/g' > $md5_rt_result
# Check integrity
grep $o_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    # rm $o_img
    echo "$o_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi
fi

echo "Integrity of $k_img & $r_img is okay."

# Check upgrade firmware's version with current firmware version
tar -xf $2 $img_ver
if [ $? != 0 ]; then
	echo "1" > /var/firmware_upgrade_status
	echo "Firmware version incorrect: no fwu_ver in img.tar !"
	exit 1
fi

cat $img_ver > $new_fw_ver
cat /etc/version > $cur_fw_ver

#cat $new_fw_ver | grep -n '^V[0-9]*.[0-9]*.[0-9]*-[0-9][0-9]*'
cat $new_fw_ver | grep -n '^V[0-9]*.[0-9]*.[0-9]*'
if [ $? != 0 ]; then
	echo "1" > /var/firmware_upgrade_status
	echo "Firmware version incorrect: `cat $new_fw_ver` !"
	exit 1
fi

echo "Try to upgrade firmware version from `cat $cur_fw_ver`"
echo "                                  to `cat $new_fw_ver`"

if [ "`cat $new_fw_ver`" == "`cat $cur_fw_ver`" ]; then
	echo "4" > /var/firmware_upgrade_status
    echo "Current firmware version already is `cat $cur_fw_ver` !"
    exit 1
fi

echo "Firware version check okay."


##### DTB ####
tar -xf $2 $d_img
string="`ls -l | grep $d_img`"
mtd_size_dec="`printf %d 0x$d_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "DTB size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi

#### kernel ####
tar -xf $2 $k_img
string="`ls -l | grep $k_img`"
mtd_size_dec="`printf %d 0x$k_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "uImage size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi
#### rootfs ####
tar -xf $2 $r_img
string="`ls -l | grep $r_img`"
mtd_size_dec="`printf %d 0x$r_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "rootfs size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi

if [ $osgi_upgraded = 1 ]; then
tar -xf $2 $o_img
string="`ls -l | grep $o_img`"
mtd_size_dec="`printf %d 0x$o_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "osgi.img size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi
fi
echo "DTB, kerenl and rootfs size check okay, start updating ..."
# Erase DTB partition
flash_eraseall $d_mtd
# Write DTB partition
echo "Writing $d_img to $d_mtd"
cp $d_img $d_mtd

# Erase kernel partition
flash_eraseall $k_mtd
# Write kernel partition
echo "Writing $k_img to $k_mtd"
cp $k_img $k_mtd

# Erase rootfs partition
flash_eraseall $r_mtd
# Write rootfs partition
echo "Writing $r_img to $r_mtd"
cp $r_img $r_mtd

if [ $osgi_upgraded = 1 ]; then
# Erase osgi partition
flash_eraseall $o_mtd
# Write osgi partition
echo "Writing $o_img to $o_mtd"
cp $o_img $o_mtd
fi

cat $new_fw_ver | grep CST
if [ $? = 0 ]; then
	echo `cat $new_fw_ver` | sed 's/ *--.*$//g' > $env_sw_ver
else
	cat $new_fw_ver > $env_sw_ver
fi
# Write image version information
nv setenv sw_version"$1" "`cat $env_sw_ver`"

# Clean up temporary files
rm -f $md5_cmp $md5_tmp $md5_rt_result $img_ver $new_fw_ver $cur_fw_ver $env_sw_ver $d_img $k_img $r_img $2

# Post processing (for future extension consideration)

echo "Successfully updated image $1!!"

