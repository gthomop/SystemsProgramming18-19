/******* hashtable.h ********/
#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>

typedef struct HashTable HashTable;
typedef struct HashedItem HashedItem;
typedef struct TransList TransList;

#include "transaction.h"
#include "wallet.h"
#include "bitCoin.h"


struct HashTable{
	HashedItem **Table;			//this is the hashTable
	size_t numOfEntries;		//number of HashTable entries, need this because sndr/rcvr-HashTables can have different size
	size_t bucketSize;			//size of buckets
};

struct HashedItem{
	TransList *first;		//fast access to first item of Transaction list
	TransList *last;		//fast access to last item of Transaction list
	char *walletID;			//need this for collisions

	//this field has 2 roles. One is to know which items of a bucket have been filled, and , also,
	//to have a connection between buckets (last item of bucket 1 -> first item of bucket 2)
	HashedItem *nextItem;
};

//this is the struct that implements a list that contains all the transactions having been made by the wallet

struct TransList{
	TransList *previous;
	Transaction *thisTrans;		//the data of transaction, this memory location should exist in both hashTables
	TransList *next;
	Wallet *sourceDestWallet;	//this is the wallet entry of either the Sender (if in receiverHashTable) or Receiver (if in senderHashTable)
};

HashTable *initializeHashTable(size_t numOfEntries, size_t bucketSize);			//create a HashTable
//when adding a new Transaction, use this in order to update a hashTable
TransList *addTransaction(HashTable *senRecHashTable, Wallet *walletID, Wallet *senRecWalletID, Transaction *tmpTransaction);

//carry out findPayments and findEarnings commands given by the user
void findPayments(HashTable *HT, char *walletID, time_t start, time_t end);
void findEarnings(HashTable *HT, char *walletID, time_t start, time_t end);		//these 2 functions could be 1, however, we cannot know after the function call, which walletID to print first (sender/receiver)

//always use freeSenderHashTable first in order to free Transactions only once and avoid segfault
void freeSenderHashTable(HashTable *HT);
void freeReceiverHashTable(HashTable *HT);

#endif
