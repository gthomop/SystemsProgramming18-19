/******* wallet.h ********/
#ifndef _WALLET_H
#define _WALLET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Wallet Wallet;
typedef struct BCList BCList;


#include "bitCoin.h"
#include "transaction.h"
#include "hashTable.h"



struct Wallet{
	char *walletID;		//self-explanatory
	size_t amount;		//amount of money owned by the wallet
	BCList *firstBCL;	//first item of the list containing all BitCoins owned by walletID
	BCList *lastBCL;	//last item -"-
};

//List of all BitCoins owned by a wallet (need this because a Bitcoin can be owned by more than one wallets)
struct BCList{
	BCList *previous;
	BCList *next;
	size_t amountOf;		//the amount of a BitCoin owned by the walletID owning this BCList
	BitCoin *thisBC;		//pointer to the BitCoin
};

extern size_t numOfWallets;		//number of all wallets added at the start of the program

//create all wallets read in bitCoinBalancesFile and create BitCoins, also
Wallet **walletsInitialization(Wallet **unordered, char *bitCoinBalancesFile, BitCoin **allBitCoins, size_t value);
//use Binary Search to find a wallet and return a pointer to it or NULL if it does not exist
Wallet *findWallet(Wallet **wallets, char *key);
void changeAmountsOfWallets(Wallet *sender, Wallet *receiver, size_t amount);	//adds and subtracts balance from wallets
int senderHasEnough(Wallet *senderID, size_t amount);							//returns 1 if Wallet with ID key has Wallet->amount >= amount

//update data of wallets after a transaction is made
void makeTransaction(Wallet *sndr, Wallet *rcvr, size_t amount, TransList *transaction);
void freeWallets(Wallet *wallets);		//free memory allocated for wallets
void walletStatus(Wallet **wallets, char *walletID);		//command given by the user
//free memory allocated for wallets if an error occurs when creating them (need this because some wallets can be unitialized when this happens and could lead to segfault)
void freeWalletsInitial(Wallet *wallets, size_t walletIndex);

#endif