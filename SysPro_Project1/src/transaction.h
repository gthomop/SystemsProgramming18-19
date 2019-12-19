/******* transaction.h ******/
#ifndef _TRANSACTION_H
#define _TRANSACTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Transaction Transaction;

#include "bitCoin.h"
#include "wallet.h"
#include "hashTable.h"

struct Transaction{
	char *transactionID;	//ID of transaction
	size_t value;			//amount Transferred
	time_t dateTime;		//time and date transaction was carried out
};

extern char maxID[15];			//biggest transaction ID, in order to generate new unique IDs
extern time_t lastDateTime;		//dateTime of last (chronologically) transaction

int transactionsInitialization(char *transactionsFile, Wallet **wallets, HashTable *senderHT, HashTable *receiverHT);		//read transactionsFile and make transactions
//this function is used for requestTransaction(s) commands given by the user
void requestTransaction(char *sndrID, char *rcvrID, size_t amount, time_t dateTime, Wallet **wallets, HashTable *senderHashTable, HashTable *receiverHashTable);

void freeTransaction(Transaction *transaction);			//free memory allocated for a given transaction

#endif
