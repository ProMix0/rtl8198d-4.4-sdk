SHARE_LIB_NAME = libca-plat-user.so

SHARE_LIB_SOURCES	=	$(SUBDIRS)/osal/osal/linux_user/os_core.c \
			$(SUBDIRS)/osal/util/tlv_parser.c \
			$(SUBDIRS)/osal/util/util_bitmap.c \
			$(SUBDIRS)/osal/util/util_hash.c \
			$(SUBDIRS)/osal/util/util_list.c \
			$(SUBDIRS)/osal/util/util_misc.c \
			$(SUBDIRS)/osal/util/util_aes.c \
			$(SUBDIRS)/osal/osal/osal_cmn.c \
			$(SUBDIRS)/osal/osal/osal_file.c \
			$(SUBDIRS)/osal/osal/osal_mem.c \
			$(SUBDIRS)/osal/osal/osal_mutex.c \
			$(SUBDIRS)/osal/osal/osal_queue.c \
			$(SUBDIRS)/osal/osal/osal_sem.c \
			$(SUBDIRS)/osal/osal/osal_spinlock.c \
			$(SUBDIRS)/osal/osal/osal_thread.c \
			$(SUBDIRS)/osal/osal/osal_timer.c \
			$(SUBDIRS)/osal/osal/osal_workqueue.c \
			$(SUBDIRS)/osal/osal/osal_misc.c \
			$(SUBDIRS)/log/log.c \
			$(SUBDIRS)/event/ca_event.c  \
			$(SUBDIRS)/pal/pal_comm.c  \
			$(SUBDIRS)/event/event_core.c

#Realtek not use thise.
#These functions of img_upgrade.c are used by ca-omci
#The image format is different and it doesn't work for RTK PON

#SHARE_LIB_SOURCES += $(SUBDIRS)/peri/img_upgrade.c

APP_MOD_INCLDUES=-I$(SUBDIRS)/inc -I$(SUBDIRS)/osal/include -I$(SUBDIRS)/osal/osal/linux_user

LOCAL_CFLAGS += -Werror -O -fPIC -D_XOPEN_SOURCE -D_GNU_SOURCE  -g  -Wall $(APP_MOD_INCLDUES)
LDFLAGS += -lrt  -lpthread 

OBJ_DIR = $(SUBDIRS)/osal/osal/linux_user/

SHARE_LIB_OBJS = $(addprefix $(OBJ_DIR), $(SHARE_LIB_SOURCES:.c=.o))
SHARE_LIB_DEPS = $(addprefix $(OBJ_DIR), $(SHARE_LIB_SOURCES:.c=.d))

OBJS = $(SHARE_LIB_OBJS)
DEPS = $(SHARE_LIB_DEPS)

all: $(DEPS) $(OBJS) $(SHARE_LIB_NAME)

$(OBJ_DIR)%.d: %.c
	 mkdir -p $(dir $@)
	 $(CC)  $(CFLAGS) $(LOCAL_CFLAGS) -MM  -MT '$(basename $@).o'  $^ 1> $@

$(OBJ_DIR)%.o: %.c
	 echo [Compile $<]
	 mkdir -p $(dir $@)
	 $(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

$(SHARE_LIB_NAME):$(SHARE_LIB_OBJS)
	 $(CC)  -shared -o $@ $(SHARE_LIB_OBJS) $(LDFLAGS)


-include $(DEPS)





