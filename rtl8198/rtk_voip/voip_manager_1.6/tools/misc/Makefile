ifeq ($(OPENWRT_BUILD), 1)
include ${DIR_LINUX}/.config

EXES-1-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += wait_dsp 
OBJS-1-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += wait_dsp.o
 
EXES-2-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_def_init
OBJS-2-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_def_init.o

CFLAGS += -g -Wall -DOPENWRT_BUILD
LDFLAGS += -lvoip_manager

EXES-1 = $(EXES-1-y)
OBJS-1 = $(OBJS-1-y)

EXES-2 = $(EXES-2-y)
OBJS-2 = $(OBJS-2-y)

DEPENDENCY-1	= $(OBJS-1:.o=.d)
DEPENDENCY-2	= $(OBJS-2:.o=.d)

all: $(EXES-1) $(EXES-2)

$(EXES-1): $(OBJS-1)
	$(CC) -DOPENWRT_BUILD -o $@ $(LDFLAGS) $^ 

$(EXES-2): $(OBJS-2)
	$(CC) -DOPENWRT_BUILD -o $@ $(LDFLAGS) $^ 

%.o: %.c
	$(CC) -DOPENWRT_BUILD -c -o $@ $(CFLAGS) $<
	$(CC) -M $(CFLAGS) $< | sed -e "s#^$(*F).o[ :]*#$(@D)\/$(*F).o : #" > $(@:.o=.d)

clean:
	rm -f $(LIBS) $(OBJS-1) $(OBJS-2) *.d	
	
else ifeq ($(YOCTO_BUILD), 1)
include ${PACKAGE_DIR}/.config

EXES-1-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += wait_dsp
OBJS-1-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += wait_dsp.o

EXES-2-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_def_init
OBJS-2-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_def_init.o

CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += -L $(LIBVOIP_DIR) -lvoip_manager
#LDFLAGS += -lvoip_manager

EXES-1 = $(EXES-1-y)
OBJS-1 = $(OBJS-1-y)

EXES-2 = $(EXES-2-y)
OBJS-2 = $(OBJS-2-y)

DEPENDENCY-1	= $(OBJS-1:.o=.d)
DEPENDENCY-2	= $(OBJS-2:.o=.d)

#all: $(EXES-1) $(EXES-2)
all:
	$(CC) wait_dsp.c        $(CFLAGS) $(LIBS) $(LDFLAGS) -o wait_dsp
	$(CC) voip_def_init.c   $(CFLAGS) $(LIBS) $(LDFLAGS) -o voip_def_init

$(EXES-1): $(OBJS-1) ${PACKAGE_DIR}/.config
	$(CC) $(LDFLAGS) $^ -o $@

$(EXES-2): $(OBJS-2) ${PACKAGE_DIR}/.config
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<
	$(CC) -M $(CFLAGS) $< | sed -e "s#^$(*F).o[ :]*#$(@D)\/$(*F).o : #" > $(@:.o=.d)

clean:
	rm -f $(LIBS) $(OBJS-1) $(OBJS-2) *.d

install:
	mkdir -p ${DEST}/usr/bin
	cp wait_dsp         ${DEST}/usr/bin/wait_dsp
	cp voip_def_init    ${DEST}/usr/bin/voip_def_init

else #else OPENWRT_BUILD

ifndef DIR_LINUX
DIR_LINUX=$(ROOTDIR)/$(LINUXDIR)
endif
include $(DIR_LINUX)/.config

EXES-1-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += wait_dsp 
OBJS-1-$(CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST) += wait_dsp.o
 
EXES-2-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_def_init
OBJS-2-$(CONFIG_RTK_VOIP_DEFER_SNDDEV_INIT) += voip_def_init.o

CFLAGS += -g -Wall -I../includes -I$(ROOTDIR)/rtk_voip/voip_manager_1.6/includes
LDFLAGS = -L$(ROOTDIR)/rtk_voip/voip_manager_1.6 -lvoip_manager 

EXES-1 = $(EXES-1-y)
OBJS-1 = $(OBJS-1-y)

EXES-2 = $(EXES-2-y)
OBJS-2 = $(OBJS-2-y)

DEPENDENCY-1	= $(OBJS-1:.o=.d)
DEPENDENCY-2	= $(OBJS-2:.o=.d)

all: $(EXES-1) $(EXES-2)

$(EXES-1): $(OBJS-1)
	$(CC) -o $@ $(LDFLAGS) $^ 

$(EXES-2): $(OBJS-2)
	$(CC) -o $@ $(LDFLAGS) $^ 

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<
	$(CC) -M $(CFLAGS) $< | sed -e "s#^$(*F).o[ :]*#$(@D)\/$(*F).o : #" > $(@:.o=.d)

romfs:
	[ ! -f wait_dsp ] || $(ROMFSINST) wait_dsp /bin/wait_dsp
	[ ! -f voip_def_init ] || $(ROMFSINST) voip_def_init /bin/voip_def_init
	[ ! -f rc_voip_module ] || $(ROMFSINST) rc_voip_module /etc/rc_voip_module

clean:
	rm -f $(LIBS) $(OBJS-1) $(OBJS-2) *.d
	rm -f wait_dsp voip_def_init

-include $(DEPENDENCY-1) $(DEPENDENCY-2)
endif 
