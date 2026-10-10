ARP_SERV = https-dns-proxy
#ARP_LDFLAGS = $(LDFLAGS) -lev -lcurl -lcares
#ARP_CFLAGS = $(CFLAGS)

all: $(ARP_SERV)

OBJ = main.o dns_poller.o dns_server.o https_client.o logging.o options.o stat.o

$(ARP_SERV):$(OBJ)
	$(CC) $(LDFLAGS) -lev -lcurl -lcares $(OBJ) -o $(ARP_SERV)

main.o:main.c dns_poller.h dns_server.h https_client.h logging.h options.h stat.h
	$(CC) $(CFLAGS) -c $^

dns_poller.o:dns_poller.c dns_poller.h logging.h
	$(CC) $(CFLAGS) -c $^

dns_server.o:dns_server.c dns_server.h logging.h
	$(CC) $(CFLAGS) -c $^

https_client.o:https_client.c https_client.h options.h logging.h stat.h
	$(CC) $(CFLAGS) -c $^

logging.o:logging.c logging.h
	$(CC) $(CFLAGS) -c $^

options.o:options.c options.h logging.h
	$(CC) $(CFLAGS) -c $^

stat.o:stat.c stat.h logging.h
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o *.gch $(ARP_SERV)