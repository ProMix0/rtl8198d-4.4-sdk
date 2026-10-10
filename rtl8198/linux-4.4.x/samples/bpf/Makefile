# kbuild trick to avoid linker error. Can be omitted if a module is built.
obj- := dummy.o

# List of programs to build
hostprogs-y := test_verifier test_maps
hostprogs-y += sock_example
hostprogs-y += fds_example
hostprogs-y += sockex1
hostprogs-y += sockex2
hostprogs-y += sockex3
hostprogs-y += tracex1
hostprogs-y += tracex2
hostprogs-y += tracex3
hostprogs-y += tracex4
hostprogs-y += tracex5
hostprogs-y += tracex6
hostprogs-y += trace_output
hostprogs-y += lathist
hostprogs-y += xdp1

EXEC = hostprogs-y

test_verifier-objs := test_verifier.o libbpf.o
test_maps-objs := test_maps.o libbpf.o
sock_example-objs := sock_example.o libbpf.o
fds_example-objs := bpf_load.o libbpf.o fds_example.o
sockex1-objs := bpf_load.o libbpf.o sockex1_user.o
sockex2-objs := bpf_load.o libbpf.o sockex2_user.o
sockex3-objs := bpf_load.o libbpf.o sockex3_user.o
tracex1-objs := bpf_load.o libbpf.o tracex1_user.o
tracex2-objs := bpf_load.o libbpf.o tracex2_user.o
tracex3-objs := bpf_load.o libbpf.o tracex3_user.o
tracex4-objs := bpf_load.o libbpf.o tracex4_user.o
tracex5-objs := bpf_load.o libbpf.o tracex5_user.o
tracex6-objs := bpf_load.o libbpf.o tracex6_user.o
trace_output-objs := bpf_load.o libbpf.o trace_output_user.o
lathist-objs := bpf_load.o libbpf.o lathist_user.o
xdp1-objs    := bpf_load.o libbpf.o xdp1_user.o

# Tell kbuild to always build the programs
always := $(hostprogs-y)
always += sockex1_kern.o
always += sockex2_kern.o
always += sockex3_kern.o
always += tracex1_kern.o
always += tracex2_kern.o
always += tracex3_kern.o
always += tracex4_kern.o
always += tracex5_kern.o
always += tracex6_kern.o
always += trace_output_kern.o
always += tcbpf1_kern.o
always += lathist_kern.o
always += xdp1_kern.o
always += test.o

HOSTCFLAGS += -I$(objtree)/usr/include -I/home/realsil/sarah_shi/rock/rtl819x-SDK-usdk-v1.2.0-full-package/rtl819x-v2/rtl819x/user/libelf/libelf
#HOSTCFLAGS += -I$(srctree)/tools/lib/
#HOSTCFLAGS += -I$(srctree)/tools/testing/selftests/bpf/
#HOSTCFLAGS += -I$(srctree)/tools/lib/ -I$(srctree)/tools/include
#HOSTCFLAGS += -I$(srctree)/tools/perf
#LIBELFDIR = $(ROOTDIR)/user/libelf/libelf
ROOTDIR = /home/realsil/sarah_shi/rock/rtl819x-SDK-usdk-v1.2.0-full-package/rtl819x-v2/rtl819x
LIBELFLAGS = -L$(ROOTDIR)/lib/zlib/zlib-1.2.11 -L$(ROOTDIR)/user/gettext-0.18.3.1/gettext-runtime/intl/.libs/ -L$(ROOTDIR)/user/lib/lib/gettext -L$(ROOTDIR)/lib/libiconv/libiconv-1.15/tmp/lib -lz -luargp -liconv -lintl
LIBELFDIR += $(LIBELFLAGS)
LIBELFDIR += -L/home/realsil/sarah_shi/rock/rtl819x-SDK-usdk-v1.2.0-full-package/rtl819x-v2/rtl819x/user/libelf/libelf
HOSTCFLAGS_bpf_load.o += -I$(objtree)/usr/include -Wno-unused-variable
HOSTLOADLIBES_fds_example += $(LIBELFDIR) -lelf
HOSTLOADLIBES_sockex1 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_sockex2 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_sockex3 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_tracex1 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_tracex2 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_tracex3 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_tracex4 += $(LIBELFDIR) -lelf -lrt
HOSTLOADLIBES_tracex5 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_tracex6 += $(LIBELFDIR) -lelf
HOSTLOADLIBES_trace_output += $(LIBELFDIR) -lelf -lrt
HOSTLOADLIBES_lathist += $(LIBELFDIR) -lelf
HOSTLOADLIBES_xdp1    += $(LIBELFDIR) -lelf

# point this to your LLVM backend with bpf support
#LLC=$(srctree)/tools/bpf/llvm/bld/Debug+Asserts/bin/llc
LLC ?= llc
CLANG ?= clang

all:
	$(MAKE) -C ../../ $(CURDIR)/

clean:
	$(MAKE) -C ../../ M=$(CURDIR) clean
	@rm -f *~

# Detect that we're cross compiling and use the cross compiler
ifdef CROSS_COMPILE
HOSTCC = $(CROSS_COMPILE)gcc
CLANG_ARCH_ARGS = -target $(ARCH)
endif
#EXTRA_CFLAGS += -I$(ROOTDIR)/user/libelf/libelf
EXTRA_CFLAGS += -I$(ROOTDIR)/user/libelf/libelf -L$(ROOTDIR)/user/libelf/libelf -lelf

# asm/sysreg.h inline assmbly used by it is incompatible with llvm.
# But, ehere is not easy way to fix it, so just exclude it since it is
# useless for BPF samples.
$(obj)/%.o: $(src)/%.c
	$(CLANG) $(NOSTDINC_FLAGS) $(LINUXINCLUDE) $(EXTRA_CFLAGS) -I$(obj) \
		-I$(srctree)/tools/testing/selftests/bpf/ \
		-D__KERNEL__ -Wno-unused-value -Wno-pointer-sign \
		-D__TARGET_ARCH_$(ARCH) -Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end \
		-Wno-address-of-packed-member -Wno-tautological-compare \
		-Wno-unknown-warning-option $(CLANG_ARCH_ARGS) \
		-O2 -emit-llvm -c $< -o -| $(LLC) -march=bpfeb -filetype=obj -o $@

