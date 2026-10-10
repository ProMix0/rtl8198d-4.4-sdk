#!/bin/sh
ROOTDIR=`pwd`
PLATFORM_CONFIG="Flash_and_DDR_Support.txt"

get_flash_setting(){
  if [ -f ${ROOTDIR}/bootloader/${PLATFORM_CONFIG} ];then
     grep -v ONFi ${ROOTDIR}/bootloader/${PLATFORM_CONFIG} | grep -v "^#" | grep -v "TYPE=" | grep -v ".*UBI.*_ARGS" |  sed '/^\s*$/d' > .flash_support.lst
  else
	 echo "Use default Flash setting: MT29F8G08ABACAWP,3,0xe0,0x1000,0x80000"
     echo "MT29F8G08ABACAWP,3,0xe0,0x1000,0x80000"  > .flash_support.lst
  fi


}

#set -x

#################################################################
# 0. bootloader         : 0x00000000 ~ 0x00400000
# 1. uboot env1         : 0x00400000 ~ 0x00500000
# 2. uboot env2         : 0x00500000 ~ 0x00600000
# 3. static_conf env2   : 0x00600000 ~ 0x00640000
# 4  G3_ubi_device.ubi  : 0x00640000 ~ file_size
#################################################################
DEFAULT_PFL_ENV1_R=0x400000
DEFAULT_PFL_ENV2_R=0x500000
UBOOTENV_IMG="uboot-env.bin"
UBOOTENV_RAW_IMG="uboot-env_raw.bin"

UBI_DEVICE_IMG="G3_ubi_device.ubi"
UBI_DEVICE_RAW_IMG="G3_ubi_device_raw.bin"
STATIC_CONFIG_IMG="config_empyt.bin"
STATIC_CONFIG_RAW_IMG="static_config_raw.bin"
STATIC_CONFIG_SIZE=262144

FULL_RAW_IMAGE="raw_image.bin"
ALL_ONE_FILE="all_one_file.bin"


conbine_raw_images(){
    #1 , cd flash dir
    FLASHDIR=$1
    BOOTI=$2

    if [ "${FLASHDIR}" = "" ];then
        echo "conbine_raw_images: miss Flash model."
        exit 1
    fi

    if [ "${BOOTI}" = "" ];then
        echo "conbine_raw_images: miss bootloader raw image."
        exit 1
    fi

    if [ $ONLY_BOOTLOADER -eq 1 ];then
        RAW_IMAGE_P="${FLASHDIR}/${1}_$FULL_RAW_IMAGE"
    else
        RAW_IMAGE_P="${FLASHDIR}/${1}_LOADER_RAW_IMAGE"
    fi

    #2, bootloader raw image
    cat ${FLASHDIR}/${BOOTI}  > ${RAW_IMAGE_P}

    #3. uboot env 1
    cat ${FLASHDIR}/$UBOOTENV_RAW_IMG >> ${RAW_IMAGE_P}

    #4. uboot env 2
    cat ${FLASHDIR}/$UBOOTENV_RAW_IMG >> ${RAW_IMAGE_P}

    if [ $ONLY_BOOTLOADER -eq 0 ];then
        ls -l ${FLASHDIR}/${STATIC_CONFIG_RAW_IMG}
        [ -f ${FLASHDIR}/${STATIC_CONFIG_RAW_IMG} ] && cat ${FLASHDIR}/${STATIC_CONFIG_RAW_IMG} >> ${RAW_IMAGE_P}
        #5. ubi device
        ls -l ${FLASHDIR}/$UBI_DEVICE_RAW_IMG
        cat ${FLASHDIR}/$UBI_DEVICE_RAW_IMG >> ${RAW_IMAGE_P}
    fi
}

#PAGE_SIZE=2048
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


gen_combined_img(){
    echo "@Generate combined image with bootloader, uboot-env, ubi device images"
    grep -q "^CONFIG_UBOOT_ENVFILE" ${ROOTDIR}/.config
    if [ $? -eq 0 ]; then
        grep "CONFIG_UBOOT_ENVFILE" ${ROOTDIR}/.config > ./.uboot-env-file.txt
        . ./.uboot-env-file.txt
        echo "Use ${CONFIG_UBOOT_ENVFILE}..."  | tee ./bin2nand-combine.log
        grep "^pfl_env1_r" ${ROOTDIR}/bootloader/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
        grep "^pfl_env2_r" ${ROOTDIR}/bootloader/u-boot-env/${CONFIG_UBOOT_ENVFILE} >> ./.uboot-env-file.txt
    else
        echo "CONFIG_UBOOT_ENVFILE=\"USE_DEFAULT_SETTING\"" > ./.uboot-env-file.txt
        echo "pfl_env1_r=${DEFAULT_PFL_ENV1_R}" >> ./.uboot-env-file.txt
        echo "pfl_env2_r=${DEFAULT_PFL_ENV2_R}" >> ./.uboot-env-file.txt
    fi

    . ./.uboot-env-file.txt

    env1_addr=`printf "%d" ${pfl_env1_r}`
    env2_addr=`printf "%d" ${pfl_env2_r}`

    echo "gen_combined_img: env1_addr=${env1_addr}"  | tee -a ./bin2nand-combine.log
    echo "gen_combined_img: env2_addr=${env2_addr}"  | tee -a ./bin2nand-combine.log

    bootloader_size=${env1_addr}
    env_size=$(( $env2_addr - $env1_addr ))

    echo "gen_combined_img: bootloader_size = ${bootloader_size}"   | tee -a ./bin2nand-combine.log
    echo "gen_combined_img: env_size = ${env_size}"   | tee -a ./bin2nand-combine.log

    #################################################################
    ##encode bootloader, env, ubi
    ################################################################
    while IFS="," read -r value1 value2 value3 value4 value5 remainder
    do
        ## ONFi (Parallel NAND) Model, ECC_type, OOB size,Page size, Block size
        MODEL=`echo ${value1} | tr -d '[:space:]'`
        ECC_TYPE=`echo ${value2} | tr -d '[:space:]'`
        OOB_SIZE=`echo ${value3} | tr -d '[:space:]'`
        PAGE_SIZE=`echo ${value4} | tr -d '[:space:]'`
        BLK_SIZE=`echo ${value5} | tr -d '[:space:]'`
        let page_size_with_spare="${PAGE_SIZE}+${OOB_SIZE}"
        
        if [ -z "${MODEL}" ] || [ -z "${ECC_TYPE}" ] || [ -z "${OOB_SIZE}" ] || [ -z "${PAGE_SIZE}" ] || [ -z "${BLK_SIZE}" ] ; then
            break;
        fi

        echo MODEL="[${MODEL}]"  | tee -a ./bin2nand-combine.log
        echo PAGE_SIZE="[${PAGE_SIZE}]"  | tee -a ./bin2nand-combine.log
        echo page_size_with_spare="[${page_size_with_spare}]"  | tee -a ./bin2nand-combine.log

        if [ ! -d ${MODEL} ] ;then
            mkdir ${MODEL}
        else
            echo "${MODEL} directory exist!"
            rm -rf ${MODEL}/*
        fi

        #### Pre-generate ALL ONE file for padding
        allone_size=1081344
        echo "tr '\000' '\377' < /dev/zero| dd of=${ALL_ONE_FILE} bs=1 count=$allone_size" | tee -a ./bin2nand-combine.log
        tr '\000' '\377' < /dev/zero| dd of=${ALL_ONE_FILE} bs=1 count=$allone_size

        list=`find ${ROOTDIR}/images -name "*${MODEL}-gpt-nandinfo.img"`
        echo "-- $list --"  | tee -a ./bin2nand-combine.log
        for i in "${list[@]}"
        do
            #a. Bootloader image
            img_name=`basename $i | awk -F'.' '{ print $1"_raw.bin"}'`
#            echo "${BIN2NAND_TOOL} ${i} --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} --padding=${bootloader_size} > ${MODEL}/${img_name}" | tee -a ./bin2nand-combine.log
            echo "bin2nand_split ${i} ${MODEL}/${img_name}" | tee -a ./bin2nand-combine.log
            bin2nand_split ${i} ${MODEL}/${img_name}

            let padding_start=`stat -c '%s' ${MODEL}/${img_name}`
            let padding_size="$[$[bootloader_size/PAGE_SIZE*page_size_with_spare]-$padding_start]" 
            echo "dd if=${ALL_ONE_FILE} of=${MODEL}/${img_name} seek=$padding_start bs=1 count=$padding_size conv=notrunc" | tee -a ./bin2nand-combine.log
            dd if=${ALL_ONE_FILE} of=${MODEL}/${img_name} seek=$padding_start bs=1 count=$padding_size conv=notrunc
                     
            #b. uboot-env
#            echo "${BIN2NAND_TOOL} ${ROOTDIR}/images/${UBOOTENV_IMG} --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} --padding=${env_size} > ${MODEL}/${UBOOTENV_RAW_IMG}"  | tee -a ./bin2nand-combine.log
            echo "bin2nand_split ${ROOTDIR}/images/${UBOOTENV_IMG} ${MODEL}/${UBOOTENV_RAW_IMG}" | tee -a ./bin2nand-combine.log
            bin2nand_split ${ROOTDIR}/images/${UBOOTENV_IMG} ${MODEL}/${UBOOTENV_RAW_IMG}

            let padding_start=`stat -c '%s' ${MODEL}/${UBOOTENV_RAW_IMG}`
            let padding_size="$[$[env_size/PAGE_SIZE*page_size_with_spare]-$padding_start]" 
            echo "dd if=${ALL_ONE_FILE} of=${MODEL}/${UBOOTENV_RAW_IMG} seek=$padding_start bs=1 count=$padding_size conv=notrunc" | tee -a ./bin2nand-combine.log
            dd if=${ALL_ONE_FILE} of=${MODEL}/${UBOOTENV_RAW_IMG} seek=$padding_start bs=1 count=$padding_size conv=notrunc          

            if [ $ONLY_BOOTLOADER -eq 0 ];then
                gen_static_config_raw_img
                gen_ubi_device_raw_img
            fi

            conbine_raw_images  ${MODEL}  ${img_name}
        done
    done < ./.flash_support.lst
}

gen_static_config_raw_img(){
    if [ -f ${ROOTDIR}/images/${STATIC_CONFIG_IMG} ];then
        echo "${BIN2NAND_TOOL} ${ROOTDIR}/images/${STATIC_CONFIG_IMG} --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} --padding=${STATIC_CONFIG_SIZE} > ${MODEL}/${STATIC_CONFIG_RAW_IMG}" | tee -a ./bin2nand-combine.log
        ${BIN2NAND_TOOL} ${ROOTDIR}/images/${STATIC_CONFIG_IMG} --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} --padding=${STATIC_CONFIG_SIZE} > ${MODEL}/${STATIC_CONFIG_RAW_IMG}
    else
        echo "dd if=${ALL_ONE_FILE} of=${ROOTDIR}/images/${STATIC_CONFIG_RAW_IMG} bs=1 seek=0 count=$STATIC_CONFIG_SIZE conv=notrunc" | tee -a ./bin2nand-combine.log
        dd if=${ALL_ONE_FILE} of=${ROOTDIR}/images/${STATIC_CONFIG_RAW_IMG} bs=1 seek=0 count=$STATIC_CONFIG_SIZE conv=notrunc
    fi
}

gen_ubi_device_raw_img(){
    #c. ubi-device image include dts, kernel and rootfs
    echo "bin2nand_split  ${ROOTDIR}/images/${UBI_DEVICE_IMG} ${MODEL}/${UBI_DEVICE_RAW_IMG}" | tee -a ./bin2nand-combine.log
    bin2nand_split  ${ROOTDIR}/images/${UBI_DEVICE_IMG} ${MODEL}/${UBI_DEVICE_RAW_IMG}
}

gen_raw_img(){
    #  while read read_line; do
    #      MODEL=`echo $read_line | awk -F, '{print $1}' | tr -d '[:space:]'`
    #       NAND_INFO_PARAM=`echo $read_line | awk -F, '{print $2,$3,$4,$5}'`
    while IFS="," read -r value1 value2 value3 value4 value5 remainder
    do
        ## ONFi (Parallel NAND) Model, ECC_type, OOB size,Page size, Block size
        MODEL=`echo ${value1} | tr -d '[:space:]'`
        ECC_TYPE=`echo ${value2} | tr -d '[:space:]'`
        OOB_SIZE=`echo ${value3} | tr -d '[:space:]'`
        PAGE_SIZE=`echo ${value4} | tr -d '[:space:]'`
        BLK_SIZE=`echo ${value5} | tr -d '[:space:]'`

        if [ -z "${MODEL}" ] || [ -z "${ECC_TYPE}" ] || [ -z "${OOB_SIZE}" ] || [ -z "${PAGE_SIZE}" ] || [ -z "${BLK_SIZE}" ] ; then
            break;
        fi

        #NAND_INFO_PARAM="${ECC_TYPE} ${OOB_SIZE} ${PAGE_SIZE} ${BLK_SIZE}"
        #IMAGE_MODLE_NAME=${IMAGE_NAME}_${MODEL}
        echo MODEL="[${MODEL}]"
        echo PAGE_SIZE="[${PAGE_SIZE}]"
        echo NAND_INFO_PARAM="[${NAND_INFO_PARAM}]"

        if [ ! -d ${MODEL} ] ;then
            mkdir ${MODEL}
        else
            echo "${MODEL} directory exist!"
            rm -rf ${MODEL}/*
        fi

        list=`find ${ROOTDIR}/images -name "*${MODEL}-gpt-nandinfo.img"`
        echo "-- $list --"
        for i in "${list[@]}"
        do
            img_name=`basename $i | awk -F'.' '{ print $1"_raw.bin"}'`
            echo "${ROOTDIR}/tools/usr/bin/bin2nand ${i} --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} > ${img_name}" | tee ./bin2nand-${MODEL}.log
            #${ROOTDIR}/tools/usr/bin/bin2nand ${i} --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} > ${img_name}
            bin2nand_split ${i} ${MODEL}/${img_name}

            echo "${ROOTDIR}/tools/usr/bin/bin2nand ${ROOTDIR}/images/uboot-env.bin --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} > uboot-env_raw.bin"  | tee -a ./bin2nand-${MODEL}.log
            #${ROOTDIR}/tools/usr/bin/bin2nand ${ROOTDIR}/images/uboot-env.bin --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} > uboot-env_raw.bin
            bin2nand_split ${ROOTDIR}/images/${UBOOTENV_IMG} ${MODEL}/${UBOOTENV_RAW_IMG}

            echo "${ROOTDIR}/tools/usr/bin/bin2nand ${ROOTDIR}/images/G3_ubi_device.ubi --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} > G3_ubi_device_raw.ubi" | tee -a ./bin2nand-${MODEL}.log
            #${ROOTDIR}/tools/usr/bin/bin2nand ${ROOTDIR}/images/G3_ubi_device.ubi --ecc-layout=BCH16 --pagesize=${PAGE_SIZE} > G3_ubi_device_raw.ubi
            bin2nand_split ${ROOTDIR}/images/${UBI_DEVICE_IMG} ${MODEL}/${UBI_DEVICE_RAW_IMG}
        done
    done < ./.flash_support.lst
}
#######main #########################
BIN2NAND_TOOL="${ROOTDIR}/tools/usr/bin/bin2nand"

if [ ! -f ${BIN2NAND_TOOL} ];then
    echo "Can't find ${BIN2NAND_TOOL} !"
    exit 1
fi

if [ ! -d ${ROOTDIR}/images ];then
    echo "Can't find ${ROOTDIR}/images !"
    exit 1
fi

if [ ! -d ${ROOTDIR}/images/programmer_images ];then
    mkdir  ${ROOTDIR}/images/programmer_images
else
    rm -rf ${ROOTDIR}/images/programmer_images/*
fi

OPT="$1"
if [ x"${OPT}" = x ];then
    OPT="COMB"
fi

#echo "ONLY_BOOTLOADER=${ONLY_BOOTLOADER}"
if [ x"${ONLY_BOOTLOADER}" = x ];then
    ONLY_BOOTLOADER=0
fi
echo "ONLY_BOOTLOADER=${ONLY_BOOTLOADER}"
echo "OPT=${OPT}"
#echo ${ROOTDIR}

#1. cd luna_g3/images/programmer_images
cd ${ROOTDIR}/images/programmer_images
#2. Produce .flash_support.lst for flash setting
get_flash_setting
#3.
if [ "${OPT}" = "COMB" ];then
    gen_combined_img
fi
if [ "${OPT}" = "RAW_IMGS" ];then
    gen_raw_img
fi
#4. Remove temp file
rm ${ALL_ONE_FILE}

