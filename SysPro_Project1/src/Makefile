#Makefile
#Ioannis Thomopoulos
#SysPro Project 1

O = bitCoin.o hashTable.o main.o mergesort.o transaction.o use.o wallet.o
C = bitCoin.c hashTable.c main.c mergesort.c transaction.c use.c wallet.c
H = bitCoin.h hashTable.h mergesort.h transaction.h use.h wallet.h
OUTPUT = bitCoin
CC = gcc
FLAGS = -g -Wall -pedantic

$(OUTPUT): $(O)
	$(CC) $(FLAGS) -o $(OUTPUT) $(O)

bitCoin.o: bitCoin.c
	$(CC) $(FLAGS) -c bitCoin.c

hashTable.o: hashTable.c
	$(CC) $(FLAGS) -c hashTable.c

main.o: main.c
	$(CC) $(FLAGS) -c main.c

mergesort.o: mergesort.c
	$(CC) $(FLAGS) -c mergesort.c

transaction.o: transaction.c
	$(CC) $(FLAGS) -c transaction.c

use.o: use.c
	$(CC) $(FLAGS) -c use.c

wallet.o: wallet.c
	$(CC) $(FLAGS) -c wallet.c

clean:
	rm -f $(O) $(OUTPUT)
