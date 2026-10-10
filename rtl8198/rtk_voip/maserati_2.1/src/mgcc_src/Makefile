include ../build/Makeparm.cfg
include ../build/Makeparm.def

EXEC = H248_carreraGT
OBJS		= mgcc.o mgcc_api.o $(CWMPEVT) uglib.o
ifdef CONFIG_CMCC
CFLAGS			+= -DHAVE_STRNDUP
endif
ifdef CONFIG_CU
CFLAGS			+= -DHAVE_STRNDUP
endif


all: built_time $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS) -I$(TR104_PATH) $(LDFLAGS) ../osip2/libosip2_release.a ../../flash/libvoip_flash_client.a

clean:
	rm -f $(EXEC) $(OBJS) built_time.h

built_time:
	echo '#define built_time "$(shell date --iso=seconds)"' > built_time.h


romfs:
	$(ROMFSINST) /bin/$(EXEC)	




	
