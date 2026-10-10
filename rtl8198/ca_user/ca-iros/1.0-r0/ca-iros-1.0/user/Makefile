SHARE_LIB_NAME = libca-sdk.so
SHARE_LIB_SOURCES = $(wildcard *.c)

APP_MOD_INCLDUES= \
	-I../inc  \
	-I./  \
	-I$(SDK_HEADER_DIR)  \
	-I$(SDK_HEADER_DIR)/include

CFLAGS += -fPIC -D_XOPEN_SOURCE -D_GNU_SOURCE  -g  -Wall $(APP_MOD_INCLDUES) $(EXTRA_CFLAGS)


SHARE_LIB_SRC_ROOT = $(addsuffix .root, $(basename $(SHARE_LIB_SOURCES)))

SHARE_LIB_OBJS = $(addprefix $(OBJ_DIR), $(SHARE_LIB_SRC_ROOT:.root=.o))
SHARE_LIB_DEPS = $(addprefix $(OBJ_DIR), $(SHARE_LIB_SRC_ROOT:.root=.d))

OBJS = $(SHARE_LIB_OBJS)
DEPS = $(SHARE_LIB_DEPS)

all user_libraries: $(DEPS) $(OBJS) $(SHARE_LIB_NAME)

$(OBJ_DIR)%.d: %.c
	 mkdir -p $(dir $@)
	 $(CC)  $(CFLAGS) -MM  -MT '$(basename $@).o'  $^ 1> $@

$(OBJ_DIR)%.o: %.c
	 echo [Compile $<]
	 mkdir -p $(dir $@)
	 $(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c -o $@ $<

$(SHARE_LIB_NAME):$(SHARE_LIB_OBJS)
	 rm -rf $(SDK_LIB_DIR)/$(SHARE_LIB_NAME)
	 $(LD)  -shared -o $@ $(SHARE_LIB_OBJS)

user_libraries_install:
	-mkdir -p ${DEST}
	cp libca-sdk.so ${DEST}

-include $(DEPS)





