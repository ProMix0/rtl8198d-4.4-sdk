ccflags-y += \
	-I$(src)/../inc  \
	-I$(src) \
	-I$(SDK_HEADER_DIR) \
	-I$(SDK_HEADER_DIR)/include

KMOD_OBJS=$(foreach file,$(basename $(notdir $(wildcard $(M)/*.c))),$(file).o)

ca-sdk-cb-objs := $(KMOD_OBJS)

obj-m := ca-sdk-cb.o
