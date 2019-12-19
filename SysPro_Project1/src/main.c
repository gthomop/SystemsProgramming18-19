/******** main.c *********/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "use.h"
#include "hashTable.h"


void freeEverything(CmdParams *arguments, BitCoin *allBitCoins, Wallet *unordered, Wallet **wallets, HashTable *sndrHT, HashTable *rcvrHT){
	//free Command line parameters
	freeCmdParams(arguments);

	//free all structs from top to bottom (HashTable -> BitCoin)
	if (sndrHT != NULL){
		freeSenderHashTable(sndrHT);
		freeReceiverHashTable(rcvrHT);
	}
	if (wallets != NULL){
		freeWallets(unordered);
		free(wallets);
	}

	freeBitCoins(allBitCoins);

	return;
}

int main(int argc, char **argv){
	CmdParams arguments = use(argc,argv);

	if (!arguments.rightUsage)
		return 1;	//wrong arguments, terminate program

	//grab data input from Cmd Line

	size_t bitCoinValue = atoi(arguments.params[2]);
	size_t senderHTEntries = atoi(arguments.params[3]);
	size_t receiverHTEntries = atoi(arguments.params[4]);
	size_t bucketSize = atoi(arguments.params[5]);
	BitCoin *bitCoins = NULL;
	Wallet *unordered = NULL;
	Wallet **wallets;
	if ((wallets = walletsInitialization(&unordered, arguments.params[0], &bitCoins, bitCoinValue)) == NULL){
		freeEverything(&arguments, bitCoins, NULL, NULL, NULL, NULL);
		return 1;
	}

	HashTable *senderHashTable = initializeHashTable(senderHTEntries, bucketSize);
	HashTable *receiverHashTable = initializeHashTable(receiverHTEntries, bucketSize);

	if ((transactionsInitialization(arguments.params[1], wallets, senderHashTable, receiverHashTable))){
		freeEverything(&arguments, bitCoins, unordered, wallets, senderHashTable, receiverHashTable);
		return 1;
	}

	takeCommands(wallets, senderHashTable, receiverHashTable, bitCoins);

	freeEverything(&arguments, bitCoins, unordered, wallets, senderHashTable, receiverHashTable);

	return 0;
}
