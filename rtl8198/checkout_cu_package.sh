#!/bin/bash

PACKAGE_SVN_PATH='http://cadinfo.realtek.com/svn/CN/cnsd5/trunk/adsl/SDK/packages'
# PACKAGE_CU_SVN_PATH='http://cadinfo.realtek.com/svn/CN/cnsd5/trunk/adsl/SDK/packages/cu'

dir_y=(cu)
# dir_cu_pkg=()

for i in ${dir_y[@]}; do  \
	if [ ! -d "user/$i" ]; then \
		svn co $PACKAGE_SVN_PATH/$i user/$i;\
	fi; \
done
# for i in ${dir_cu_pkg[@]}; do  \
	# if [ ! -d "user/$i" ]; then \
		# svn co $PACKAGE_CU_SVN_PATH/$i user/$i; \
	# fi; \
# done
