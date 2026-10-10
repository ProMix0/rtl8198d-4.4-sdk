sdk_prep:
	cd $(SDK_ROOT_DIR) && ./preconfig.sh

sdk_tp_config:
	cp -arf $(CONFIG_DIR)/sdk_config/. $(SDK_ROOT_DIR)/

sdk_build:
	cd $(SDK_ROOT_DIR) && $(MAKE) all V=s

sdk_clean:
	cd $(SDK_ROOT_DIR) && $(MAKE) clean V=s

sdk: sdk_prep sdk_tp_config sdk_build
	@echo "build sdk done"


