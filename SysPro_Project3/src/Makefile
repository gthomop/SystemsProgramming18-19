#Makefile
#Ioannis Thomopoulos
#SysPro Project 3

OCLIENT = dropbox_client.o dirControl.o client_list.o use_client.o buffer.o
OSERVER = dropbox_server.o client_list.o
CCLIENT = dropbox_client.c dirControl.c client_list.c use_client.c buffer.c
CSERVER = dropbox_server.c client_list.c
H = dropbox_client.h dirControl.h client_list.h use_client.h buffer.h dropbox_server.h
OUTCLIENT = dropbox_client
OUTSERVER = dropbox_server
CC = gcc
FLAGS = -g -Wall -pedantic
ENDFLAGS = -lcrypto -lssl -pthread

all: $(OUTCLIENT) $(OUTSERVER)

$(OUTSERVER):	$(OSERVER)
	$(CC) $(FLAGS) -o $(OUTSERVER) $(OSERVER)

$(OUTCLIENT):	$(OCLIENT)
	$(CC) $(FLAGS) -o $(OUTCLIENT) $(OCLIENT) $(ENDFLAGS)

dropbox_client.o: dropbox_client.c
	$(CC) $(FLAGS) -c dropbox_client.c $(ENDFLAGS)

dirControl.o: dirControl.c
	$(CC) $(FLAGS) -c dirControl.c

client_list.o: client_list.c
	$(CC) $(FLAGS) -c client_list.c

use_client.o: use_client.c
	$(CC) $(FLAGS) -c use_client.c

buffer.o: buffer.c
	$(CC) $(FLAGS) -c buffer.c

dropbox_server.o: dropbox_server.c
	$(CC) $(FLAGS) -c dropbox_server.c

clean:
	rm -f $(OCLIENT) $(OUTCLIENT) $(OSERVER) $(OUTSERVER)
