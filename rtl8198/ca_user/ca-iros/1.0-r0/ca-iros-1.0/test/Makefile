APP_MOD_NAME = ca-iros
APP_MODE_SOURCES = $(wildcard *.c)  test_wrap.c

APP_MOD_FLAGS=
APP_MOD_INCLDUES= \
	-I../inc  \
	-I./  \
	-I$(SDK_HEADER_DIR)  \
	-I$(SDK_HEADER_DIR)/include

APP_MOD_LD_FLAGS= -L../user -lca-sdk

#Compile Flags
LOCAL_CFLAGS +=  \
	-include $(KCONFIG_FILE) \
	-D__LITTLE_ENDIAN__ \
	-D_XOPEN_SOURCE \
	-D_GNU_SOURCE \
	-g \
	-Wall \
	$(APP_MOD_FLAGS) \
	$(APP_MOD_INCLDUES)
CFLAGS += ${LOCAL_CFLAGS} ${EXTRA_CFLAGS}

APP_SRC_ROOT = $(addsuffix .root, $(basename $(APP_MODE_SOURCES)))

APP_OBJS = $(addprefix $(OBJ_DIR), $(APP_SRC_ROOT:.root=.o))
APP_DEPS = $(addprefix $(OBJ_DIR), $(APP_SRC_ROOT:.root=.d))

OBJS = $(APP_OBJS)
DEPS = $(APP_DEPS)

all : $(DEPS) $(OBJS) $(APP_MOD_NAME)

$(OBJ_DIR)%.d: %.c
	mkdir -p $(dir $@)
	$(CC) ${CFLAGS} -MM  -MT '$(basename $@).o'  $^ 1> $@

$(OBJ_DIR)%.o: %.c
	echo [Compile $<]
	mkdir -p $(dir $@)
	$(CC) ${CFLAGS} -c -o $@ $<

$(APP_MOD_NAME): $(APP_OBJS)
	$(AT) echo [Linking $@]
	$(AT) $(CC) ${EXTRA_LDFLAGS} $(APP_MOD_LD_FLAGS) $(LDFLAGS)  -o $@  $^

test_wrap.c:$(wildcard *.i)
	sed -i '/KERNEL CONFIG/r ${KCONFIG_FILE}' test.i
	swig -o $@ test.i

install:
	-mkdir -p ${DEST}/etc ${DEST}/usr/bin
	cp files/ca-iros.rc  ${DEST}/etc/
	cp files/iros ${DEST}/usr/bin
	cp ca-iros ${DEST}/usr/bin

-include $(DEPS)

clean:
	$(AT) rm -rf $(DEPS) $(OBJS) $(APP_MOD_NAME) $(OBJ_DIR) $(SHARE_LIB_NAME)




