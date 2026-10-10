CA_IROS_DIR := .
MODULE_SRC := `pwd`/$(CA_IROS_DIR)/driver

SDK_GEN_ROOT := $(CA_IROS_DIR)
SDK_CODE_TOOL := $(CA_IROS_DIR)/tools/sdk_code_gen.py
DEMO_I := $(CA_IROS_DIR)/test/template.i
SDK_DEPENDS_FILES= $(SDK_CODE_TOOL) $(DEMO_I) $(SDK_API_FILES)
SDK_API_FILES:=$(filter-out %/cs_types.h %/ca_types.h,$(wildcard $(SDK_HEADER_DIR)/*.h))

md5_files_suffix = $(patsubst %,%.md5,$1)
demd5_files_suffix = $(patsubst %.md5,%,$1)
SDK_API_FILES_MD5 = $(call md5_files_suffix,$(SDK_API_FILES))

all: user_app kerne_modules

user_app : user_libraries
	$(MAKE) -C test

user_app_install:
	$(MAKE) -C test install

user_libraries : Codegen
user_libraries user_libraries_install:
	$(MAKE) -C user $@

clean :
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_SRC) SUBDIRS=$(MODULE_SRC) $@
	$(RM) Codegen

modules : Codegen
modules modules_install : Codegen
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_SRC) $@

Codegen: $(SDK_API_FILES_MD5)
	python $(SDK_CODE_TOOL) $(SDK_GEN_ROOT)/ $(DEMO_I) $(call demd5_files_suffix,$^)
	touch $@

%.md5:%
	$(if $(filter-out $(shell cat $@ 2>/dev/null), $(shell md5sum $*)),md5sum $* > $@)

