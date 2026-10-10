#!/bin/bash

if [ "${ROOTDIR}" = "" ] ; then
    echo "Error: ROOTDIR is not set!!"
    exit -1
fi

source ${ROOTDIR}/.config
source ${LINUX_CONFIG}


# echo "DSP+IPC=" $CONFIG_RTL8686_IPC_MEM_SIZE + $CONFIG_RTL8686_DSP_MEM_SIZE 
VOIP_DSP=$((${CONFIG_RTL8686_IPC_MEM_SIZE:=0} + ${CONFIG_RTL8686_DSP_MEM_SIZE:=0}))
echo VOIP_DSP=$VOIP_DSP bytes

tmp_str=`size ${LINUXDIR}/vmlinux|tail -n 1`
# echo $tmp_str
tmp_arr=($tmp_str)
# echo tmp_arr[3]=${tmp_arr[3]}
kernel_size=${tmp_arr[3]}
echo kernel_size=${kernel_size} bytes

run_time_spare=$((10*1024*1024))
echo run-time spare=${run_time_spare} bytes

PBO_size=$((${CONFIG_LUNA_PBO_DL_DRAM_SIZE:=0}+${CONFIG_LUNA_PBO_UL_DRAM_SIZE:=0}))
echo PBO=${PBO_size} bytes

total_mem=$((${run_time_spare}+${kernel_size}+${VOIP_DSP}+${PBO_size}))
printf "Total run-time memory requirement estimation=%s Bytes\n" $total_mem

total_mem=`echo "scale=2; ($total_mem/1024/1024)" | bc`
printf "Total run-time memory requirement estimation=%s MBytes\n" $total_mem
