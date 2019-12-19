/****** use.h ******/
#ifndef _USE_H
#define _USE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashTable.h"
#include "wallet.h"
#include "transaction.h"
#include "bitCoin.h"

//auxiliary struct for parsing program arguments
typedef struct CmdParams{
	int rightUsage;
	char *params[6];
} CmdParams;

CmdParams use(int argc, char **argv);
//free memory allocated for arguments' strings
void freeCmdParams(CmdParams *arguments);

//begin reading commands from Command Line and carry them out
void takeCommands(Wallet **wallets, HashTable *senderHashTable, HashTable *receiverHashTable, BitCoin *allBitCoins);

#endif
