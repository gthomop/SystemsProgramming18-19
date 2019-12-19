#Makefile
#Ioannis Thomopoulos
#SysPro Project 2

OCLIENT = clientSync.o dirControl.o main.o use.o
OSYNC = sync.o dirControl.o
OSEND = sync_send.o dirControl.o
ORECEIVE = sync_receive.o dirControl.o
CCLIENT = clientSync.c dirControl.c main.c use.c
CSYNC = sync.c dirControl.c
CSEND = sync_send.c dirControl.c
CRECEIVE = sync_receive.c dirControl.c
H = clientSync.h dirControl.h use.h
OUTCLIENT = mirror_client
OUTSYNC = sync
OUTSEND = sync_send
OUTRECEIVE = sync_receive
CC = gcc
FLAGS = -g -Wall -pedantic

all: $(OUTCLIENT) $(OUTSYNC) $(OUTSEND) $(OUTRECEIVE)


$(OUTCLIENT):	$(OCLIENT)
	$(CC) $(FLAGS) -o $(OUTCLIENT) $(OCLIENT)

$(OUTSYNC):	$(OSYNC)
	$(CC) $(FLAGS) -o $(OUTSYNC) $(OSYNC)

$(OUTSEND):	$(OSEND)
	$(CC) $(FLAGS) -o $(OUTSEND) $(OSEND)

$(OUTRECEIVE):	$(ORECEIVE)
	$(CC) $(FLAGS) -o $(OUTRECEIVE) $(ORECEIVE)

clientSync.o: clientSync.c
	$(CC) $(FLAGS) -c clientSync.c

dirControl.o: dirControl.c
	$(CC) $(FLAGS) -c dirControl.c

main.o: main.c
	$(CC) $(FLAGS) -c main.c

use.o: use.c
	$(CC) $(FLAGS) -c use.c

sync.o: sync.c
	$(CC) $(FLAGS) -c sync.c

sync_send.o: sync_send.c
	$(CC) $(FLAGS) -c sync_send.c

sync_receive.o: sync_receive.c
	$(CC) $(FLAGS) -c sync_receive.c

clean:
	rm -f $(OCLIENT) $(OSYNC) $(OSEND) $(ORECEIVE) $(OUTCLIENT) $(OUTSYNC) $(OUTSEND) $(OUTRECEIVE)
