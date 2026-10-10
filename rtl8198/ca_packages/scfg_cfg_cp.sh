#!/bin/sh
#  find $(VENDDIR)/conf44/$*/ -name 'scfg-*.txt' -exec cp {} $(ROOTDIR)/$(CA_SCFG_CFG) \;; \
#

CA_SCFG_DIR=ca-scfg/1.0-r0/ca-scfg-1.0

basedir=$(dirname "$0")
sourcedir=$1

dest_cfg=${basedir}/${CA_SCFG_DIR}/config/
dest_cfg_cust=${basedir}/${CA_SCFG_DIR}/config.customized/


#echo "$basedir"
#echo ${sourcedir}
#echo "$dest_cfg"
#echo "$dest_cfg_cust"

# default scfg config

find ${sourcedir} -name 'scfg-*.txt' -exec cp {} ${dest_cfg} \;

# customized scfg config
#find ${sourcedir} -name 'scfg-*.txt.customized' -exec cp {} ${dest_cfg_cust} \;

tmp=`find  ${sourcedir} -name "scfg-*.txt.customized"`

for scfg_file in ${tmp}; do
	if [ -f ${scfg_file} ] ; then
		dest_filename=`basename  ${scfg_file}`
		cp ${scfg_file} ${dest_cfg_cust}/${dest_filename%.*}
	fi
done;

