#!/bin/sh
TOOLS_DIR=`pwd`
CC=gcc
AS=as
CXX=g++
AR=ar
LD=ld 
OBJCOPY=objcopy
RANLIB=ranlib
STRIPTOOL=strip
STRIP=strip
CFLAGS=
LDFLAGS=
CROSS=

do_build_automake(){
cd ${TOOLS_DIR}
if [ ! -f ./usr/bin/automake-1.15 ];then
echo "Running do_build_automake ......."
tar -zxvf automake-1.15.tar.gz
cd automake-1.15
./configure --prefix=${TOOLS_DIR}/usr
make all install
fi

}
do_build_libtool(){
cd ${TOOLS_DIR}
if [ ! -f ./usr/bin/libtool ];then
echo "Running do_build_libtool ................"
tar -zxvf libtool-2.4.6.tar.gz
cd libtool-2.4.6
./configure --prefix=${TOOLS_DIR}/usr
make all install
fi
}


do_build_lzma(){
cd ${TOOLS_DIR}
if [ ! -f ./usr/bin/lzma ];then
echo "Running do_build_lzma ................"
cd lzma-4.32.7
./configure --prefix=${TOOLS_DIR}/usr
make all install
fi
}


do_build_bin2nand(){
cd ${TOOLS_DIR}
if [ ! -f ./usr/bin/bin2nand ];then
 if [ -f bin2nand-20080107.xz ];then
   echo "Running do_build_bin2nand ................"
    tar -Jxvf bin2nand-20080107.xz
    cd bin2nand-20080107/ubi-utils
    make bin2nand
    if [ -f bin2nand ];then 
     cp bin2nand  ${TOOLS_DIR}/usr/bin/ ;
    fi
 fi #bin2nand-20080107.xz
fi
}
##### Main #####
do_build_automake
do_build_libtool
do_build_lzma
do_build_bin2nand


