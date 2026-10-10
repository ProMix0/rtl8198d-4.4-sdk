LIB = libomcivoip

CFLAGS += -I$(APOLLODIR)/sdk/src/app/omci_v1/API -I$(ROOTDIR)/user/rtk_voip/includes/

LDFLAGS += -L$(APOLLODIR)/object/src/app/lib \
		-L$(ROOTDIR)/lib/librtk \
		$(LIBMIB) $(LIBVOIP)

libomcivoip.o: libomcivoip.c
	$(CC) -Os -c -o $@ $(CFLAGS) -fPIC $?

all: libomcivoip.o
	$(CC) -shared -o $(LIB).so -L$(ROOTDIR)/lib/librtk -L$(APOLLODIR)/object/src/app/lib $(LIBMIB) $(LIBVOIP) $?

clean:
	rm -rf $(LIB).so
	rm -rf *.o

romfs:
	$(ROMFSINST) /lib/features/libomcivoip.so
