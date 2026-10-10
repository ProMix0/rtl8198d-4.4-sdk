#!/bin/sh
IMAGEDIR=$1 
IMAGE=$2
if [ ! -d ${IMAGEDIR} ];then
	echo "IMAGEDIR Not Found!"
	exit 1
fi
if [ ! -f ${IMAGEDIR}/${IMAGE} ];then
	echo "Image: ${IMAGE} Not Found!"
	exit 1
fi
rm -f ${IMAGE}.align
rm -f ${IMAGE}.ori
#echo "--------$IMAGE--------"
Image_size=`du -sb ${IMAGEDIR}/${IMAGE} | head -n 1 | awk '{ print $1 }'` ; 
#echo ${Image_size} ; 
Block=$(( $Image_size / 262144)) ; 
#echo $Block
Desired_Block=$(( $Block + 1 )) ;
DSIZE=$(( $Desired_Block * 262144)) ; 
PaddingSize=$(( $DSIZE - $Image_size )) ; 

cp ${IMAGEDIR}/${IMAGE} ${IMAGEDIR}/${IMAGE}.align; 
#echo "pading ${PaddingSize}" ;
dd if=/dev/zero bs=1 count=${PaddingSize}>> ${IMAGEDIR}/${IMAGE}.align;
if [ -f ${IMAGEDIR}/${IMAGE}.align ] ;then
	if [ ! -d ${IMAGEDIR}/.tmp ]; then
		mkdir -p ${IMAGEDIR}/.tmp
	fi	
	mv ${IMAGEDIR}/${IMAGE} ${IMAGEDIR}/.tmp/${IMAGE}.ori
	mv ${IMAGEDIR}/${IMAGE}.align ${IMAGEDIR}/${IMAGE}
fi
