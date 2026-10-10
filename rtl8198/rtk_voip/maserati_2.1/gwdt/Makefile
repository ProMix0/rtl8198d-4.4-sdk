TARGET = voip_gwdt
OBJS = gwdt_main.o
CFLAGS = -Wall -Os -g -DGWDT_DEBUG
LIBS = 
LDFLAGS = 

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

romfs:
	$(ROMFSINST) /bin/$(TARGET)
	#$(ROMFSINST) app.gwdt /etc/app.gwdt
	$(ROMFSINST) app.gwdt.sip /etc/scripts/app.gwdt.sip
	$(ROMFSINST) app.gwdt.h248 /etc/scripts/app.gwdt.h248
	$(ROMFSINST) rc_voip /etc/rc_voip
	$(ROMFSINST) voip_mptest /bin/voip_mptest
	
clean:
	-rm -rf $(TARGET) $(OBJS)


	
