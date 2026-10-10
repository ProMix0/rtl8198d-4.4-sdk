

#CFLAGS += -Wall -Os -mips16 
#CFLAGS := -Wall -Os

CSV_PARSER_NAME    =  csv_parser
CSV_PARSER_PATH	   =  $(PWD)
CSV_PARSER_LIB     =  lib$(CSV_PARSER_NAME).a
CSV_PARSER_SO_LIB  =  lib$(CSV_PARSER_NAME).so

CFLAGS += -ICSV_PARSER_PATH

OBJS		= csv_api.o csv_parser.o
DEPENDENCY	= $(OBJS:.o=.d)

all: $(CSV_PARSER_LIB)
#lib: $(OBJ_DIR) $(CSV_PARSER_LIB) $(CSV_PARSER_SO_LIB)
#lib: $(OBJ_DIR) $(CSV_PARSER_LIB) 

#$(OBJ_DIR):
#	mkdir -p $@
	
$(CSV_PARSER_LIB): $(OBJS)
	$(AR) rcs $(CSV_PARSER_LIB) $^

$(CSV_PARSER_SO_LIB): $(OBJS)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< 
	
%.o: %.c
	$(CC) -DOPENWRT_BUILD -c -o $@ $(CFLAGS) $<
	$(CC) -M $(CFLAGS) $< | sed -e "s#^$(*F).o[ :]*#$(@D)\/$(*F).o : #" > $(@:.o=.d)
	
romfs:
	$(ROMFSINST) config_in_dram.csv /etc/config.csv
#	$(ROMFSINST) config_in_flash.csv /etc/config.csv
#	$(ROMFSINST) ./reference/call_log_ref.csv /etc/call_log_ref.csv

clean:
	-rm -rf $(CSV_PARSER_LIB) $(CSV_PARSER_SO_LIB) $(OBJS) $(DEPENDENCY)

-include $(DEPENDENCY)

.PHONY: all clean 