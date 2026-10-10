FILES = $(shell ls script_dbg)
all:

romfs:
	for i in $(FILES) ; do cp script_dbg/$$i $(ROMFSDIR)/bin/$$i ; done
