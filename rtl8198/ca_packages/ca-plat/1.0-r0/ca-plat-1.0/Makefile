SRC_DIR:=`pwd`

all: modules
modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC_DIR) $@

module_headers_install:
	-mkdir -p ${DEST}
	cp inc/*.h ${DEST}
	cp osal/include/*.h ${DEST}
	cp osal/osal/linux_kernel/*.h  ${DEST}
	cp peri/peri.h ${DEST}
	cp event/*.h ${DEST}
	cp log/*.h ${DEST}

user_headers_install:
	-mkdir -p ${DEST}
	cp inc/*.h ${DEST}
	cp osal/include/*.h ${DEST}
	cp osal/osal/linux_user/*.h ${DEST}
	cp peri/peri.h ${DEST}
	cp event/*.h ${DEST}
	cp log/*.h ${DEST}

user_libraries_install:
	-mkdir -p ${DEST}
	cp osal/osal/linux_user/libca-plat-user.so ${DEST}

iros_headers_install:
	-mkdir -p ${DEST}
	cp peri/peri.h ${DEST}

.PHONY: all modules modules_install clean
