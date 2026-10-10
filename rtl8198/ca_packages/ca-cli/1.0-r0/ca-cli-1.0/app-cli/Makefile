APP_BIN  = app-cli 
LIB_BIN  = libca-cli.so
APP_SRCS = $(wildcard *.c generate/*.c)
LIB_SRCS = $(filter-out main.c, $(APP_SRCS))
ifdef CONFIG_CA_CLI_HAS_TELNETD
CFLAGS  += -DCLI_OS_LINUX -D__LITTLE_ENDIAN__ -DCLI_HAS_TELNETD -fPIC -DPIC
else
CFLAGS  += -DCLI_OS_LINUX -D__LITTLE_ENDIAN__ -fPIC -DPIC
endif

CFLAGS  += -I../include $(if $(OSAL_DIR),-I $(OSAL_DIR)) $(if $(OSAL_USER_DIR),-I $(OSAL_USER_DIR))
CFLAGS  += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

OUTPUT_DIR = .output

#Makefile share part
APP_OBGS = $(addprefix $(OUTPUT_DIR)/,$(patsubst %.c,%.o,$(APP_SRCS)))
APP_DEPS = $(addprefix $(OUTPUT_DIR)/,$(patsubst %.c,%.d,$(APP_SRCS)))

LIB_OBJS  = $(addprefix $(OUTPUT_DIR)/,$(patsubst %.c,%.o,$(LIB_SRCS)))
LIB_DEPS  = $(addprefix $(OUTPUT_DIR)/,$(patsubst %.c,%.d,$(LIB_SRCS)))


sinclude $(LINUX_CONFIG)
ifeq ($(shell [ $(CONFIG_ARCH_CORTINA_VENUS) = y ] || [ $(CONFIG_ARCH_REALTEK_TAURUS) = y ] && echo true),true)
ifdef CONFIG_ARCH_CORTINA_VENUS
REG_FILE = ${KERNEL_DIR}/include/soc/cortina/ca8277b_registers.h
endif
ifdef CONFIG_ARCH_REALTEK_TAURUS
REG_FILE = ${KERNEL_DIR}/include/soc/cortina/rtl8277c_registers.h
endif

all: $(APP_BIN) $(LIB_BIN) $(APP_DEPS) $(LIB_DEPS) reg_addr

reg_addr:
	cp ${REG_FILE} ${OUTPUT_DIR}/register.h
	cat ${OUTPUT_DIR}/register.h | grep "#define" | grep "0x" | grep -v "_dft" > ${OUTPUT_DIR}/tmp.txt
	sed 's/#define //g' ${OUTPUT_DIR}/tmp.txt > reg.txt
	cp reg.txt ${ROOTDIR}/sysroot-destdir/etc/

else
all: $(APP_BIN) $(LIB_BIN) $(APP_DEPS) $(LIB_DEPS)
endif

$(OUTPUT_DIR)/%.o:%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUTPUT_DIR)/%.d:%.c
	mkdir -p $(dir $@)
	$(CC)  $(CFLAGS) -MM  -MT '$(basename $@).o'  $^ 1> $@


$(APP_BIN):$(APP_OBGS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(LIB_BIN):$(LIB_OBJS)
	$(CC) -shared  -fPIC -DPIC $^ $(LDFLAGS)  -O2   -Wl,-soname -Wl,$@ -o $@

-include $(APP_DEPS) $(LIB_DEPS) 

clean:
	$(RM) -r $(APP_BIN) $(LIB_BIN) $(OUTPUT_DIR)
