
files:=$(wildcard $(src)/*.c $(src)/generate/*.c)
cli-drv-objs := $(patsubst $(src)/%.c,%.o,$(files))
obj-m := cli-drv.o

ccflags-y += \
-DCLI_KERNEL_SPACE \
-DCLI_OS_LINUX \
-D__LITTLE_ENDIAN__ \
-I$(src)/../include \
-I$(src) \
-I$(AAL_H_DIR)\
-I$(AAL_H_DIR)/include
