#!/bin/sh
IMAGEDIR=$1 
IMAGE=$2
WORKDIR=`pwd`
LZMA=${WORKDIR}/tools/usr/bin/lzma
if [ ! -d ${IMAGEDIR} ];then
	echo "IMAGEDIR Not Found!"
	exit 1
fi
if [ ! -f ${IMAGEDIR}/${IMAGE} ];then
	echo "Image: ${IMAGE} Not Found!"
	exit 1
fi
cd ${IMAGEDIR}; 
if [ -f Image ]; then
    echo ${LZMA}
   # echo "Execute \"lzma e Image Image.lzma...\""
    echo "Execute \"lzma -f -k Image\""
    ${LZMA} -f -k Image ; 
else 
    echo "error: Miss Image file"
    exit 1; 
fi

if [ ! -f Image.lzma ]; then
   echo "lzma fail."
   echo "Build Host must install lzma"
   exit 127
fi
