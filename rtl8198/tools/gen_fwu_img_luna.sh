#!/bin/sh
#Generate luna firmware upgarde image tar ball

echo $1
use_ubifs=1
if [ "$1" = "-ubifs" ]; then
	use_ubifs=0
fi

up_script="fwu.sh"
fw_list="rootfs uImage fwu_ver hw_ver"
framework_list="framework.img framework.sh"
all_list="$up_script $fw_list $framework_list"
md5_result="md5.txt"
dst_dir="../images/img_tar"
romfs_ver="../romfs/etc/version"
fw_tar_file="img.tar"
fw_tar_file_noOsgi="img_noOsgi.tar"
fw_tar_file_noPreBundle="img_noPreBundle.tar"
fw_tar_file2="img_enc.tar"
openssl_exe="openssl"
openssl_key="realtek"
encrypt_algo="aes-128-cbc"
framework_tar_file="framework.tar"
all_tar_file="fw_framework.tar"
pwddir=`pwd`
rootdir=`dirname $pwddir`
firmware_enhance_result="firmware_enhance.txt"

hw_check=`cat ../.config | grep CONFIG_HWVER_CHECK | sed s/CONFIG_HWVER_CHECK=//g | sed s/\"//g`
hw_ver_str=`cat ../.config | grep CONFIG_HW_HWVER | sed s/CONFIG_HW_HWVER=\"//g | sed s/\"//g`
is_yueme=`cat ../linux-4.4.x/.config | grep "CONFIG_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cmcc=`cat ../linux-4.4.x/.config | grep "CONFIG_CMCC=" | sed s/[^y]//g | sed s/\"//g`
is_cu_yueme=`cat ../linux-4.4.x/.config | grep "CONFIG_CU_BASEON_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cu_cmcc=`cat ../linux-4.4.x/.config | grep "CONFIG_CU_BASEON_CMCC=" | sed s/[^y]//g | sed s/\"//g`
is_98d_nor=`cat ../linux-4.4.x/.config | grep "CONFIG_MTD_NOR_SPIF_G3=" | sed s/[^y]//g | sed s/\"//g`
is_jio=`cat ../linux-4.4.x/.config | grep "CONFIG_JIO=" | sed s/[^y]//g | sed s/\"//g`
cmcc_install_prebundle=`cat ../config/.config | grep "CONFIG_CMCC_OSGI_PREBUNDLE=" | sed s/[^y]//g | sed s/\"//g`
is_firmware_enhance=`cat ../config/.config | grep "CONFIG_USER_FIRMWARE_ENHANCE=" | sed s/[^y]//g | sed s/\"//g`


if [ "y" = "$is_cmcc" ]; then
    fw_list_osgi="rootfs uImage osgi.img fwu_ver hw_ver"
    fw_list_osgi_install_prebundle="rootfs uImage osgi.img fwu_ver hw_ver install_prebundle_flag"
    echo "CMCC fw_list=$fw_list_osgi"
fi

if [ "y" = "$is_cu_cmcc" ]; then
    fw_list_osgi="rootfs uImage osgi.img fwu_ver hw_ver"
    fw_list_osgi_install_prebundle="rootfs uImage osgi.img fwu_ver hw_ver install_prebundle_flag"
    echo "CU fw_list=$fw_list_osgi"
fi

test -f hw_ver && rm -rf hw_ver
if [ "$hw_check" = "y" ]; then
    echo $hw_ver_str > hw_ver
else
    echo "skip" > hw_ver
fi

if [ "$use_ubifs" = "0" ]; then
	if [ "$is_cmcc" = "y" ]; then
		cp fwu_ubi_cmcc.sh fwu.sh
	else
  if [ "$is_cu_cmcc" = "y" ]; then
		cp fwu_ubi_cmcc.sh fwu.sh
	else
		cp fwu_ubi.sh fwu.sh
  fi
	fi
else
if [ "$is_cmcc" = "y" ]; then
	cp fwu_luna_ubi_cmcc.sh fwu.sh
elif [ "$is_cu_cmcc" = "y" ]; then
	cp fwu_luna_ubi_cmcc.sh fwu.sh
elif [ "$is_jio" = "y" ]; then
	cp fwu_luna_ubi_jio.sh fwu.sh
elif [ "$is_98d_nor" = "y" ]; then
	[ -f fwu_nor.sh ] && cp fwu_nor.sh fwu.sh
else
	cp fwu_luna_ubi.sh fwu.sh
fi
fi

if [ ! -d $dst_dir ];then
	mkdir -p $dst_dir
fi

#g3_eng
cp ../images/rootfs      $dst_dir/rootfs
cp ../images/uImage            $dst_dir/uImage

cp fwu.sh $dst_dir
cp ../linux-4.4.x/fwu_ver $dst_dir

#[ -f ../images/osgi/osgi.img ] && cp ../images/osgi.img $dst_dir

# If /romfs/etc/version is exist, replace it to /images/fwu_ver 
# then parse&store by nv setenv in fwu.sh
cat $romfs_ver
if [ $? = 0 ]; then
if [ "$is_yueme" = "y" ]; then
	echo `cat $romfs_ver` | sed 's/ *--.*$//g' > $dst_dir/fwu_ver
else
if [ "$is_cu_yueme" = "y" ]; then
	echo `cat $romfs_ver` | sed 's/ *--.*$//g' > $dst_dir/fwu_ver
else
	cat $romfs_ver > $dst_dir/fwu_ver
fi
fi
fi

cp hw_ver $dst_dir
cd $dst_dir


if [ "y" = "$is_cmcc" ] || [ "y" = "$is_cu_cmcc" ]; then
	#img.tar: fwu.sh rootfs uImage osgi.img fwu_ver hw_ver md5.txt
	#img_noOsgi.tar: fwu.sh rootfs uImage fwu_ver hw_ver md5.txt
	cp ../osgi/osgi_noPreBundle.img ./osgi.img
	md5sum $fw_list_osgi $up_script > $md5_result
	tar -cf $fw_tar_file_noPreBundle $up_script $fw_list_osgi $md5_result
	rm -rf ./osgi.img
	cp ../osgi/osgi.img ./osgi.img

	if [ "y" = "$cmcc_install_prebundle" ]; then 
		touch install_prebundle_flag
		md5sum $fw_list_osgi_install_prebundle $up_script > $md5_result
		if [ "y" = "$is_firmware_enhance" ]; then
			$rootdir/tools/firmware_enhance_gen $md5_result > $firmware_enhance_result
			tar -cf $fw_tar_file $up_script $fw_list_osgi_install_prebundle $md5_result $firmware_enhance_result
		else
			tar -cf $fw_tar_file $up_script $fw_list_osgi_install_prebundle $md5_result
		fi
		cat $fw_tar_file | $openssl_exe $encrypt_algo -e -out $fw_tar_file2  -k $openssl_key
	else
		md5sum $fw_list_osgi $up_script > $md5_result
		if [ "y" = "$is_firmware_enhance" ]; then
			$rootdir/tools/firmware_enhance_gen $md5_result > $firmware_enhance_result
			tar -cf $fw_tar_file $up_script $fw_list_osgi $md5_result $firmware_enhance_result
		else
			tar -cf $fw_tar_file $up_script $fw_list_osgi $md5_result
		fi
		cat $fw_tar_file | $openssl_exe $encrypt_algo -e -out $fw_tar_file2  -k $openssl_key
	fi
	md5sum $fw_list $up_script > $md5_result
	tar -cf $fw_tar_file_noOsgi $up_script $fw_list $md5_result
else
	md5sum $fw_list $up_script > $md5_result
	if [ "y" = "$is_firmware_enhance" ]; then
		$rootdir/tools/firmware_enhance_gen $md5_result > $firmware_enhance_result
		tar -cf $fw_tar_file $up_script $fw_list $md5_result $firmware_enhance_result
	else
		tar -cf $fw_tar_file $up_script $fw_list $md5_result
	fi
fi

#### work in images/img_tar

if [ "$is_yueme" = "y" ] && [ -f $rootdir/framework.img ]; then
	cp  $rootdir/framework.img .
	echo Aligning framework.img to 2k/page boundary for NAND platform
	sz=`stat --printf="%s" $rootdir/framework.img`
	pagecnt=$(( (sz+2047) / 2048 ))
	dd if=${rootdir}/framework.img ibs=2k count=$pagecnt of=./framework.img conv=sync
	ls -l ./framework.img
	cp $rootdir/tools/framework.sh .

	md5sum $framework_list $up_script > $md5_result
	tar -cf $framework_tar_file $up_script $framework_list $md5_result
	md5sum $all_list > $md5_result
	tar -cf $all_tar_file $all_list $md5_result
fi
if [ "$is_cu_yueme" = "y" ] && [ -f $rootdir/framework.img ]; then
	cp  $rootdir/framework.img .
	echo Aligning framework.img to 2k/page boundary for NAND platform
	sz=`stat --printf="%s" $rootdir/framework.img`
	pagecnt=$(( (sz+2047) / 2048 ))
	dd if=${rootdir}/framework.img ibs=2k count=$pagecnt of=./framework.img conv=sync
	ls -l ./framework.img
	cp $rootdir/tools/framework.sh .

	md5sum $framework_list $up_script > $md5_result
	tar -cf $framework_tar_file $up_script $framework_list $md5_result
	md5sum $all_list > $md5_result
	tar -cf $all_tar_file $all_list $md5_result
fi
ls -l $fw_tar_file
cp *.tar ../


