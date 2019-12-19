/******* bitCoin.h *******/
#ifndef _BITCOIN_H
#define _BITCOIN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct BitCoin BitCoin;
typedef struct TransTree TransTree;


#include "transaction.h"
#include "wallet.h"
#include "hashTable.h"


struct BitCoin{
	size_t ID;			//the ID of BitCoin
	size_t numOfTransactions;		//number of Transactions this BitCoin has been involved into
	TransTree *root;				//the root of the Transaction Tree
	BitCoin *prevBC;				//connect all BitCoins in a list, so as to free them only once
	BitCoin *nextBC;				//this could not be done otherwise, because a BitCoin can be owned by more than 1 wallets
};

struct TransTree{
	Wallet *walletID;		//the wallet that received BitCoin after a transaction (or the wallet that was left some BitCoin after a transaction)
	size_t amount;			//the amount transferred to (or remaining to) walletID

	//this is a pointer to a SenderHashTable->TransList element	(could be ReceiverHashTable, too, because when printing information
	//about the transactions a BitCoin has been involved into, we only need the ID and the amount (not exactly need the amount, but if we can avoid a subtraction, it is ok))
	TransList *transaction;	

	TransTree *parent;		//parent tree node
	TransTree *left;		//the left node always represents the amount of BitCoin remaining to parent (can be non-existent)
	TransTree *right;		//the right node always represents the amount of the BitCoin received by walletID
};

BitCoin *bitCoinInitialization(int id, Wallet *wallet, size_t value, BitCoin *previousBC);		//create a BitCoin (and its tree)
size_t makeBCTransaction(BitCoin *BC, size_t amount, Wallet *sender, Wallet *receiver, TransList *transaction);		//update data of a BitCoin after it's been involved into a transaction
void freeBitCoins(BitCoin *BC);		//free memory allocated for all BitCoins
void bitCoinStatus(size_t BCID, BitCoin *allBitCoins);			//command given by the user
void traceCoin(size_t BCID, BitCoin *allBitCoins);				//command given by the user

#endif
