SRC_DIR:=`pwd`

all:modules

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC_DIR)/scfg $@

module_headers_install user_headers_install:
	-mkdir -p ${DEST}
	cp scfg/include/scfg.h ${DEST}
	cp scfg/include/scfg_defs.h ${DEST}
	cp scfg/include/not_scfg.h ${DEST}

iros_headers_install:
	-mkdir -p ${DEST}
	cp scfg/include/scfg.h ${DEST}

configure: .confingure

modules modules_install: prerequisites

prerequisites: scfg/include/scfg_defs.h  scfg/cfg_array.c

scfg/cfg_array.c: .configure
	python3 tools/cfg_cvt.py -f .configure -a -b > scfg/cfg_array.c

scfg/include/scfg_defs.h: .configure
	python3 tools/cfg_cvt.py -f .configure -d > scfg/include/scfg_defs.h

.confingure:
	cp config.customized/scfg-${TARGET}.txt .configure2
	cp config/scfg-${TARGET}.txt .configure

config_install:
	-mkdir -p ${DEST}
	cp .configure ${DEST}/default_scfg.txt
	-cp .configure2 ${DEST}/scfg.txt

.PHONY: all modules modules_install clean
