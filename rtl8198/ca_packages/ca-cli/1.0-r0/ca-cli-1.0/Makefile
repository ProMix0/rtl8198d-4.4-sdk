AAL_CMN_H_FILE=./include/cli_aal.h
AAL_HEADER_FILES=$(wildcard $(AAL_H_DIR)/*.h)
CLI_NODE_FILE=./include/cli_product.h
CLI_REGISTER_KFILES=./cli-drv/cli_aal.c
CLI_REGISTER_FILES=app-cli/cli_aal.c

CLI_OUTPUT_KDIR=cli-drv/generate
CLI_OUTPUT_DIR=app-cli/generate

CLI_CODE_TOOL_AAL=./tools/cli_code_gen_aal.py
CLI_DEPENDS_FILES =$(CLI_CODE_TOOL_AAL) $(CLI_NODE_FILE) $(CLI_REGISTER_FILES_AAL) $(AAL_CMN_H_FILE) $(AAL_HEADER_FILES)

CFLAGS += $(shell grep CONFIG_ $(KERNEL_SRC)/.config |grep "=y" |sed 's/=y//' |sed 's/CONFIG_/\-DCONFIG_/')

all : app-cli/app-cli modules

.drvaal_codegen:$(AAL_HEADER_FILES) $(CLI_CODE_TOOL_AAL)
	cp src/cli_aal.c cli-drv/cli_aal.c
	cp src/cli_product.h include/cli_product.h 
	python $(CLI_CODE_TOOL_AAL) $(CLI_OUTPUT_KDIR) $(CLI_NODE_FILE) $(CLI_REGISTER_KFILES) $(AAL_CMN_H_FILE)  $(AAL_HEADER_FILES)
	touch .drvaal_codegen

.libaal_codegen: $(AAL_HEADER_FILES) $(CLI_CODE_TOOL_AAL)
	cp src/cli_aal.c app-cli/cli_aal.c
	cp src/cli_product.h include/cli_product.h 
	python $(CLI_CODE_TOOL_AAL) $(CLI_OUTPUT_DIR)  $(CLI_NODE_FILE) $(CLI_REGISTER_FILES) $(AAL_CMN_H_FILE)  $(AAL_HEADER_FILES)
	touch .libaal_codegen

PWD := $(shell pwd)

FORCE:


modules : .drvaal_codegen FORCE
modules modules_install :
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD)/cli-drv $@ 

user_app app-cli/app-cli : .libaal_codegen FORCE
	$(MAKE) -C app-cli

clean:
	$(MAKE) -C app-cli clean
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
	$(RM) .drvaal_codegen .libaal_codegen
	$(RM) -r $(CLI_OUTPUT_KDIR) $(CLI_OUTPUT_DIR)
