#!/bin/sh
#set -x
#################################################################
# Venus Partitions 
#################################################################
#device nand0 <ca_spinand_flash>, # parts = 5
# #: name                size            offset          mask_flags
# 0: boot                0x00180000      0x00000000      0
# 1: env                 0x00040000      0x00200000      0
# 2: env2                0x00040000      0x00240000      0
# 3: static_conf         0x00040000      0x00280000      0
# 4: ubi_device          0x0f5c0000      0x002c0000      0
#################################################################
ROOTDIR=`pwd`
BOOTLOADER_DIR=${ROOTDIR}/bootloader
PROG_IMG_DIR=${ROOTDIR}/images/programmer_images
BIN2NAND_TOOL="${ROOTDIR}/tools/usr/bin/bin2nand"

PLATFORM_CONFIG="Flash_and_DDR_Support.txt"
DEFAULT_PFL_ENV1_R=0x400000
DEFAULT_PFL_ENV2_R=0x500000
STATIC_CONFIG_SIZE=262144

BOOTLOADER_IMG="bootloader.bin"
UBOOTENV_IMG="uboot-env.bin"
UBI_DEVICE_IMG="G3_ubi_device.ubi"

ALL_ONE_FILE="all_one_file.bin"

COMBINED_LOADER_IMAGE="combined_loader_image.bin"
COMBINED_ONEBIN_IMAGE="combined_onebin_image.bin"
BCH_COMBINED_LOADER_IMAGE="bch_combined_loader_image.bin"
BCH_COMBINED_ONEBIN_IMAGE="bch_combined_onebin_image.bin"

SFLASH_PROGGRAMER_IMG_LOADER="SFLASH_PROGGRAMER_LOADER.bin"
SFLASH_PROGGRAMER_IMG_COMBINED="SFLASH_PROGGRAMER_FULLSYS.bin"
PNAND_PROGGRAMER_IMG_LOADER="PNAND_PROGGRAMER_LOADER.bin"
PNAND_PROGGRAMER_IMG_COMBINED="PNAND_PROGGRAMER_FULLSYS.bin"


CHUNK_SIZE=4096
#usage() {
#     echo "prog <in-filename> <out-filename> <thread_n>"
#}
# func <in-filename> <out-filename> <thread_n>
bin2nand_split() {
    if [ "$#" -lt 2 ]; then
        usage
        exit 1
    fi 

    if [ ! -f "$1" ]; then
        echo "$1 not exist"
        exit 1
    fi

    let cpu_nums=`cat /proc/cpuinfo | grep processor | wc -l`
    
    set -e
    let file_size=`stat -c '%s' $1`
    let split_size="($file_size/$cpu_nums+$CHUNK_SIZE-1)/$CHUNK_SIZE*$CHUNK_SIZE"

    split -b $split_size $1 -d tmp_

    for file in tmp_*; do 
    {
        ${BIN2NAND_TOOL} $file --ecc-layout=BCH16 --pagesize=$PAGE_SIZE > fl_$file
        rm -f $file
    } &
    done

    wait

    cat  fl_tmp_* > $2
    rm -f fl_tmp_*

    set +e
}


get_flash_type(){
    if [ -f ${BOOTLOADER_DIR}/${PLATFORM_CONFIG} ];then
        echo "@grep -v ONFi ${ROOTDIR}/bootloader/${PLATFORM_CONFIG} "
        grep -v ONFi ${BOOTLOADER_DIR}/${PLATFORM_CONFIG} | grep -v "^#" | grep -v "TYPE=" | grep -v ".*UBI.*_ARGS" |  sed '/^\s*$/d' > .flash_support.lst
        while IFS="," read -r value1 
        do
            BOOTDEV="PARALLEL"
        done < ./.flash_support.lst
    else
        echo "ERROR: Fail to get flash type and model"
    fi
    rm .flash_support.lst
}

get_image_combine_addr(){
    echo "------------------------------------" 

    grep -q "^CONFIG_UBOOT_ENVFILE" ${ROOTDIR}/.config
    if [ $? -eq 0 ]; then
        grep "CONFIG_UBOOT_ENVFILE" ${ROOTDIR}/.config > ./.uboot-env-file.txt
        . ./.uboot-env-file.txt
        echo "Use ${CONFIG_UBOOT_ENVFILE}"  | tee ./.programmer-image-combine.log
        grep "^pfl_bbl1_r"   ${BOOTLOADER_DIR}/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
        grep "^pfl_bbl2_r"   ${BOOTLOADER_DIR}/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
        grep "^pfl_env1_r"   ${BOOTLOADER_DIR}/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
        grep "^pfl_env2_r"   ${BOOTLOADER_DIR}/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
        grep "^pfl_env_size" ${BOOTLOADER_DIR}/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
    else
        echo "CONFIG_UBOOT_ENVFILE=\"USE_DEFAULT_SETTING\"" > ./.uboot-env-file.txt
        echo "pfl_env1_r=${DEFAULT_PFL_ENV1_R}" >> ./.uboot-env-file.txt
        echo "pfl_env2_r=${DEFAULT_PFL_ENV2_R}" >> ./.uboot-env-file.txt
    fi
 
    . ./.uboot-env-file.txt
    bbl1_addr=`printf "%d" ${pfl_bbl1_r}`
    bbl2_addr=`printf "%d" ${pfl_bbl2_r}`
    bbl_size=$(( $bbl2_addr - $bbl1_addr ))
    env1_addr=`printf "%d" ${pfl_env1_r}`
    env2_addr=`printf "%d" ${pfl_env2_r}`
    env_size=`printf "%d" ${pfl_env_size}`
    static_addr=$(( $env2_addr + $env_size ))
    ubidev_addr=$(( $static_addr + $STATIC_CONFIG_SIZE ))

    #### Generate ALL ONE file for padding
    tr '\000' '\377' < /dev/zero| dd of=${ALL_ONE_FILE} bs=1 count=${STATIC_CONFIG_SIZE}    

    
    echo "get_image_combine_addr: bbl1_addr=${bbl1_addr}"
    echo "get_image_combine_addr: bbl2_addr=${bbl2_addr}"
    echo "get_image_combine_addr: env1_addr=${env1_addr}"
    echo "get_image_combine_addr: env2_addr=${env2_addr}"

    if [ "${ONLY_BOOTLOADER}" = 0 ];then
        echo "get_image_combine_addr: static_addr=${static_addr}"
        echo "get_image_combine_addr: ubidev_addr=${ubidev_addr}"
    fi
}

gen_sflash_image(){
    echo "------------------------------------" 
    echo "@Generate SPI flash combined image of bootloader, uboot-env, ubi device images"

    ln -s ${ROOTDIR}/bootloader/image/*loader*.img ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
    ln -s ${ROOTDIR}/bootloader/image/*-env*.bin ${ROOTDIR}/bootloader/image/${UBOOTENV_IMG}
    
    echo "dd if=${BOOTLOADER_DIR}/image/${BOOTLOADER_IMG} of=${COMBINED_LOADER_IMAGE} seek=0  conv=notrunc" | tee -a ./.programmer-image-combine.log
#    echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
#    echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl2_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
#    echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
#    echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
    echo "dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
    echo "dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
    dd if=${BOOTLOADER_DIR}/image/${BOOTLOADER_IMG} of=${COMBINED_LOADER_IMAGE} seek=0  conv=notrunc 
    dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl1_addr}  count=$bbl_size conv=notrunc
    dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl2_addr}  conv=notrunc
    dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc
    dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc
    dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc
    dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc
    if [ "${BOOTDEV}" = "SPI" ];then
        echo "cp ${COMBINED_LOADER_IMAGE} ${SFLASH_PROGGRAMER_IMG_LOADER}" | tee -a ./.programmer-image-combine.log          
        cp ${COMBINED_LOADER_IMAGE} ${SFLASH_PROGGRAMER_IMG_LOADER}
    fi

    if [ "${ONLY_BOOTLOADER}" = 0 ];then
        cp ${COMBINED_LOADER_IMAGE} ${COMBINED_ONEBIN_IMAGE}
#        echo "gen_combined_image: static_addr=${static_addr}"
#        echo "gen_combined_image: ubidev_addr=${ubidev_addr}"
#        echo "dd if=${ALL_ONE_FILE} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${static_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        echo "dd if=${ROOTDIR}/images/${UBI_DEVICE_IMG} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${ubidev_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        dd if=${ALL_ONE_FILE} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${static_addr}  conv=notrunc
        dd if=${ROOTDIR}/images/${UBI_DEVICE_IMG} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${ubidev_addr}  conv=notrunc
        
        if [ "${BOOTDEV}" = "SPI" ];then
            echo "cp ${COMBINED_ONEBIN_IMAGE} ${SFLASH_PROGGRAMER_IMG_COMBINED}" | tee -a ./.programmer-image-combine.log          
            cp ${COMBINED_ONEBIN_IMAGE} ${SFLASH_PROGGRAMER_IMG_COMBINED}
        fi
    fi
   
    rm ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
    rm ${ROOTDIR}/bootloader/image/${UBOOTENV_IMG} 
}

gen_pnand_bch_img(){
    echo "------------------------------------" 
    echo "@Generate PNAND BCH images" | tee ./.programmer-image-combine.log   
    echo "@grep -v ONFi ${BOOTLOADER_DIR}/${PLATFORM_CONFIG}" | tee -a ./.programmer-image-combine.log   
	grep -v ONFi ${BOOTLOADER_DIR}/${PLATFORM_CONFIG} | grep -v "^#" | grep -v "TYPE=" | grep -v ".*UBI.*_ARGS" |  sed '/^\s*$/d' > .flash_support.lst
	while IFS="," read -r value1 value2 value3 value4 value5 value6 remainder
	do
		## ONFi (Parallel NAND) Model, ECC_type, OOB size,Page size, Block size
		MODEL=`echo ${value1} | tr -d '[:space:]'`
		ECC_TYPE=`echo ${value2} | tr -d '[:space:]'`
		OOB_SIZE=`echo ${value3} | tr -d '[:space:]'`
		PAGE_SIZE=`echo ${value4} | tr -d '[:space:]'`	 
		if [ -z "${MODEL}" ] || [ -z "${ECC_TYPE}" ] || [ -z "${OOB_SIZE}" ] || [ -z "${PAGE_SIZE}" ]; then
            echo "Error: Fail to get complete PNAND model!!" | tee -a ./.programmer-image-combine.log   
            break;
		fi
        echo "MODEL=${MODEL}"
        echo "ECC_TYPE=${ECC_TYPE}"
        echo "PAGE_SIZE=${PAGE_SIZE}"
        echo "OOB_SIZE=${OOB_SIZE}"
        if [ "${PAGE_SIZE}" = "0x800" ]; then
            PAGE_SIZE=2048
            ln -s ${ROOTDIR}/bootloader/image/*_2Kpage_loader.img ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
		elif [ "${PAGE_SIZE}" = "0x1000" ]; then 
            PAGE_SIZE=4096
            ln -s ${ROOTDIR}/bootloader/image/*_4Kpage_loader.img ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
        else break
		fi

        if [ "${OOB_SIZE}" = "0x40" ]; then OOB_SIZE=64
		elif [ "${OOB_SIZE}" = "0x80" ]; then OOB_SIZE=128
		else break
        fi

        mkdir ${MODEL}
        ln -s ${ROOTDIR}/bootloader/image/*-env*.bin ${ROOTDIR}/bootloader/image/${UBOOTENV_IMG}
    
        echo "dd if=${BOOTLOADER_DIR}/image/${BOOTLOADER_IMG} of=${COMBINED_LOADER_IMAGE} seek=0  conv=notrunc" | tee -a ./.programmer-image-combine.log
#       echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
#       echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl2_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
#       echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
#       echo "dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        echo "dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        dd if=${BOOTLOADER_DIR}/image/${BOOTLOADER_IMG} of=${COMBINED_LOADER_IMAGE} seek=0  conv=notrunc 
        dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl1_addr}  conv=notrunc
        dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${bbl2_addr}  conv=notrunc
        dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc
        dd if=${ALL_ONE_FILE} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc
        dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc
        dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${COMBINED_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc
        
        if [ "${ECC_TYPE}" = 3 ];then
            echo "bin2nand_split ${COMBINED_LOADER_IMAGE} ${MODEL}/${PNAND_PROGGRAMER_IMG_LOADER}"  | tee -a ./.programmer-image-combine.log   
            bin2nand_split ${COMBINED_LOADER_IMAGE} ${MODEL}/${PNAND_PROGGRAMER_IMG_LOADER}
            
            if [ "${ONLY_BOOTLOADER}" = 0 ];then
                cp ${COMBINED_LOADER_IMAGE} ${COMBINED_ONEBIN_IMAGE}
                echo "dd if=${ROOTDIR}/images/${UBI_DEVICE_IMG} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${ubidev_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
                dd if=${ALL_ONE_FILE} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${static_addr}  conv=notrunc
                dd if=${ROOTDIR}/images/${UBI_DEVICE_IMG} of=${COMBINED_ONEBIN_IMAGE} bs=1 seek=${ubidev_addr}  conv=notrunc

            
                echo "bin2nand_split ${COMBINED_ONEBIN_IMAGE} ${MODEL}/${PNAND_PROGGRAMER_IMG_COMBINED}"  | tee -a ./.programmer-image-combine.log   
                bin2nand_split ${COMBINED_ONEBIN_IMAGE} ${MODEL}/${PNAND_PROGGRAMER_IMG_COMBINED}
            fi
        fi
        rm ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
    done < ./.flash_support.lst
    
    rm ${ROOTDIR}/bootloader/image/${UBOOTENV_IMG} 
    rm ${COMBINED_LOADER_IMAGE}
    rm ${COMBINED_ONEBIN_IMAGE}
}


################################
# main
################################
if [ ! -f ${BIN2NAND_TOOL} ];then
    echo "Can't find ${BIN2NAND_TOOL} !"
    exit 1
fi

if [ ! -d ${ROOTDIR}/images ];then
    echo "Can't find ${ROOTDIR}/images !"
    exit 1
fi

if [ ! -d ${PROG_IMG_DIR} ];then
    mkdir  ${PROG_IMG_DIR}
else
    rm -rf ${PROG_IMG_DIR}/*
fi

#Step1: Check only bootloader
ONLY_BOOTLOADER="$1"
if [ x"${ONLY_BOOTLOADER}" = x ];then
    ONLY_BOOTLOADER=0
else
    ONLY_BOOTLOADER=1
fi
echo "ONLY_BOOTLOADER=${ONLY_BOOTLOADER}"

#Step2: Get flash type: SPI (default) or Parllel
BOOTDEV="SPI"
get_flash_type
echo "BOOTDEV=${BOOTDEV}"

#Step3: cd ${ROOTDIR}/programmer_images
echo "ROOTDIR=${ROOTDIR}"
echo "BOOTLOADER_DIR=${BOOTLOADER_DIR}"
echo "PROG_IMG_DIR=${PROG_IMG_DIR}"
cd ${PROG_IMG_DIR}

#Step3: get address for combine image
get_image_combine_addr

#Step4: cd ${ROOTDIR}/programmer_images
if [ "${BOOTDEV}" = "SPI" ];then
    gen_sflash_image
else
    gen_pnand_bch_img
fi

#Step5: Remove tmpp files
rm ${ALL_ONE_FILE}