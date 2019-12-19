/******* wallet.c *******/
#include "wallet.h"
#include "mergesort.h"

size_t numOfWallets = 0;

/* struct Wallet{
	char *walletID;
	size_t amount;
	BCList *firstBCL;
	BCList *lastBCL;
};

struct BCList{
	BCList *previous;
	BCList *next;
	size_t amountOf;
	BitCoin *thisBC;
}; */

//checks whether a BitCoin has already been added on initialization
int checkIfBCExists(Wallet *wallets, size_t id, size_t walletIndex){
	BCList *currBCL;
	for (size_t i = 0; i < walletIndex; i++){
		currBCL = wallets[i].firstBCL;	//first BitCoin of wallet
		while (currBCL != NULL)		//while the last BitCoin of wallet has not been checked
			if (currBCL->thisBC->ID == id) return 1;
			else currBCL = currBCL->next;
	}
	return 0;		//if the loop ends, the BitCoin with id==ID does not already exist
}

//checks whether a Wallet has already been added and initialized
int checkIfWalletExists(Wallet *wallets, char *id, size_t walletIndex){
	for (size_t i = 0; i < walletIndex; i++)
		if (!(strcmp(wallets[i].walletID, id)))	return 1;	//strcmp return 0 if the 2 strings are identical
	return 0;
}

//count how many lines the bitCoinBalancesFile has (so many wallets)
size_t findNumOfWallets(FILE *file){	//count wallets in file
	char c = 'A';
	size_t wallets = 0;
	int flag = 0;			//the flag states that at least one character was read, indicating that the file has at least one wallet. This is important in case the file only contains one wallet.

	while ((c = fgetc(file)) != EOF){
		flag = 1;
		if (c == '\n')
			wallets++;
	}
	rewind(file);
	
	//wallets++;			//on last line there is no \n

	if (!(flag))	return 0;
	else	return wallets;
}

//returns the number of BitCoins owned by a wallet
size_t findNumOfBCsOnLine(FILE *file){	//returns the number of BitCoin IDs on each line of file(wallet)
	char c = 'A';
	int offset = 1;		//helps to return file pointer to the value before call
						//starts from 1 because I found that fseek works that way :P
	size_t numOfBCsOnLine = 0;
	int flag = 0;

	while ((c = fgetc(file)) != '\n' && c != EOF){	//while we are on the same line
		offset++;			//each character read
		flag = 1;
		if (c == ' ')	numOfBCsOnLine++;	//BitCoin IDs are separated by spaces
	}
	if (offset != 1)	fseek(file, -offset, SEEK_CUR);		//return file pointer to previous stage

	if (numOfBCsOnLine == 0 && !(flag))	return 0;
	else	return (numOfBCsOnLine + 1);		//there are so many IDs as spaces on each line. However, we have skipped the space between the walletID and the first BitCoin ID
}

//sorts wallets using mergesort and returns an array of alphabetically sortered (pointers to) wallets
Wallet **sortWallets(Wallet *wallets){
	Wallet **scndWallets = malloc(sizeof(Wallet*) * numOfWallets);	//helping Wallet array
	Wallet **ordered = malloc(sizeof(Wallet*) * numOfWallets);
	for (size_t i = 0; i < numOfWallets; i++)
		ordered[i] = &wallets[i];

	mergeSort(ordered, 0, numOfWallets - 1, scndWallets);

	free(scndWallets);

	return ordered;
}

Wallet **walletsInitialization(Wallet **unordered, char *bitCoinBalancesFile, BitCoin **allBitCoins, size_t value){
	FILE *file = fopen(bitCoinBalancesFile, "r");

	if (file == NULL){
		printf("File: %s was not read properly. Program is terminated.\n",bitCoinBalancesFile);
		return NULL;
	}
	Wallet *wallets;
	if (!(numOfWallets = findNumOfWallets(file))){		//if no wallets were found in file
		printf("No wallet data found in file: %s, program is terminated\n",bitCoinBalancesFile);
		return NULL;
	}
	else	wallets = malloc(sizeof(Wallet) * numOfWallets);

	*unordered = wallets;
	char buffer[64];
	int tmpID = 0;
	int ownerExistsOnLine = 0;
	int numOfBCsOnLine = 0;
	//int numOfBitCoins = 0;	//global variable in bitCoin.h
	BitCoin *tmpBC = NULL;
	BCList *tmpBCL = NULL;
	size_t walletIndex = 0;

	do{

		if ((ownerExistsOnLine = fscanf(file, "%s ", buffer)) == -1)	break;	//fscanf returns -1 if EOF is met

		if ((checkIfWalletExists(wallets, buffer, walletIndex))){
			printf("Wallet %s currently being inserted already exists. Program is terminated.\n", buffer);
			freeWalletsInitial(wallets, walletIndex + 1);
			return NULL;
		}

		wallets[walletIndex].walletID = malloc(sizeof(char)*(1 + strlen(buffer)));
		strcpy(wallets[walletIndex].walletID, buffer);

		if (!(numOfBCsOnLine = findNumOfBCsOnLine(file))){
			wallets[walletIndex].firstBCL = NULL;
			wallets[walletIndex].lastBCL = NULL;
			wallets[walletIndex].amount = 0;
			walletIndex++;
			continue;
		}
		
		wallets[walletIndex].amount = numOfBCsOnLine*value;
		wallets[walletIndex].firstBCL = NULL;
		wallets[walletIndex].lastBCL = NULL;

		for (int i = 0; i < numOfBCsOnLine; i++){
			fscanf(file, "%d ", &tmpID);

			//new BitCoin Initialization
			tmpBC = bitCoinInitialization(tmpID, &(wallets[walletIndex]), value, tmpBC);
			if (*allBitCoins == NULL)
				*allBitCoins = tmpBC;


			if (wallets[walletIndex].firstBCL == NULL && 
			(!(walletIndex) || !(checkIfBCExists(wallets, tmpID, walletIndex)))){		//if the wallet has no BitCoins at all AND (it is the first wallet OR the BitCoin does not already exist)
				tmpBCL = malloc(sizeof(BCList));
				wallets[walletIndex].firstBCL = tmpBCL;
				tmpBCL->previous = NULL;
				tmpBCL->next = NULL;
				tmpBCL->thisBC = tmpBC;
				tmpBCL->amountOf = value;
				wallets[walletIndex].lastBCL = tmpBCL;
			}
			else if (!(checkIfBCExists(wallets, tmpID, walletIndex))){	//if wallet has already BitCoins and it is not the first wallet and the BitCoin does not already exist
				tmpBCL = malloc(sizeof(BCList));
				tmpBCL->previous = wallets[walletIndex].lastBCL;
				tmpBCL->next = NULL;
				tmpBCL->thisBC = tmpBC;
				tmpBCL->amountOf = value;
				wallets[walletIndex].lastBCL->next = tmpBCL;
				wallets[walletIndex].lastBCL = tmpBCL;
			}
			else{
				printf("BitCoin currently being inserted already exists. Program is terminated.\n");
				freeWalletsInitial(wallets, walletIndex + 1);
				return NULL;
			}
		}

		walletIndex++;

	} while (ownerExistsOnLine != -1);

	fclose(file);

	Wallet **orderedWalls = sortWallets(wallets);

	return orderedWalls;
}

Wallet *findWallet(Wallet **wallets, char *key){
	int leftBound = 0;
	int rightBound = numOfWallets - 1;
	int middle = 0;
	int cmp = 0;

	while (leftBound <= rightBound){
		middle = (leftBound + rightBound) / 2;

		cmp = strcmp(wallets[middle]->walletID, key);
		if (!(cmp))
		       return wallets[middle];
		else if (cmp < 0)
			leftBound = middle + 1;
		else
			rightBound = middle - 1;
	}

	return NULL;	//NULL is returned when the wallet is not found
}

void changeAmountsOfWallets(Wallet *sender, Wallet *receiver, size_t amount){	//adds and subtracts balance from wallets
	sender->amount -= amount;
	receiver->amount += amount;
	return;
}

int senderHasEnough(Wallet *senderID, size_t amount){
	if (senderID->amount >= amount)
		return 1;
	else
		return 0;
}

//adds a BitCoin to receiver Wallet (if they don't already own it)
void addToBCList(Wallet *receiver, BitCoin *bitCoin, size_t amountTransferred){
	if (receiver->firstBCL == NULL){
		receiver->firstBCL = malloc(sizeof(BCList));
		receiver->lastBCL = receiver->firstBCL;
		receiver->firstBCL->previous = NULL;
		receiver->firstBCL->next = NULL;
		receiver->firstBCL->amountOf = amountTransferred;
		receiver->firstBCL->thisBC = bitCoin;

		return;
	}

	//search for bitCoin and if found make necessary changes
	BCList *list = receiver->firstBCL;
	while(list != NULL){
		if (list->thisBC->ID == bitCoin->ID){
			list->amountOf += amountTransferred;
			return;
		}
		
		list = list->next;
	}

	//if any portion of bitCoin is not already owned by receiver
	receiver->lastBCL->next = malloc(sizeof(BCList));
	receiver->lastBCL->next->previous = receiver->lastBCL;
	receiver->lastBCL = receiver->lastBCL->next;
	receiver->lastBCL->next = NULL;
	receiver->lastBCL->amountOf = amountTransferred;
	receiver->lastBCL->thisBC = bitCoin;

	return;
}

//removes a BitCoin from sender's wallet if the amount becomes 0
void removeFromBCList(Wallet *sender, BCList *bcList){
	if (sender->firstBCL == bcList && sender->firstBCL == sender->lastBCL){		//if sender has only one bitCoin
		sender->firstBCL = NULL;
		sender->lastBCL = NULL;
	}
	else if (sender->firstBCL == bcList){		//if sender has only one bitCoin and it is this BCList
		sender->firstBCL = bcList->next;
		bcList->next->previous = NULL;
	}
	else if (sender->lastBCL == bcList){		//if it is the last BCList
		sender->lastBCL = bcList->previous;
		bcList->previous->next = NULL;
	}
	else{
		bcList->next->previous = bcList->previous;
		bcList->previous->next = bcList->next;
	}

	free(bcList);

	return;
}

void makeTransaction(Wallet *sndr, Wallet *rcvr, size_t amount, TransList *transaction){
	size_t tmpAmount = amount;
	size_t amountTransferred = 0;


	BCList *bitCoin = sndr->firstBCL;
	while(tmpAmount > 0){

		if (bitCoin->amountOf >= tmpAmount){
			amountTransferred = makeBCTransaction(bitCoin->thisBC, tmpAmount, sndr, rcvr, transaction);
			tmpAmount -= amountTransferred;

			addToBCList(rcvr, bitCoin->thisBC, amountTransferred);	//this function adds the bitcoin to bcList only if it does not exist
			bitCoin->amountOf -= amountTransferred;
			if (bitCoin->previous != NULL && bitCoin->previous->amountOf == 0)
				removeFromBCList(sndr, bitCoin);
		}
		else{			//amount of bitCoin owned by sndr is less than the needed amount for transaction
			amountTransferred = makeBCTransaction(bitCoin->thisBC, bitCoin->amountOf, sndr, rcvr, transaction);
			tmpAmount -= amountTransferred;
			addToBCList(rcvr, bitCoin->thisBC, amountTransferred);

			bitCoin->amountOf -= amountTransferred;

			bitCoin->thisBC->numOfTransactions++;
			bitCoin = bitCoin->next;
			removeFromBCList(sndr, bitCoin->previous);
		}
	}

	changeAmountsOfWallets(sndr, rcvr, amount);

	return;
}

void freeWallets(Wallet *wallets){
	BCList *currBCL = NULL;
	for (size_t i = 0; i < numOfWallets; i++){
		free(wallets[i].walletID);

		currBCL = wallets[i].firstBCL;
		while (currBCL->next != NULL){
			currBCL = currBCL->next;
			free(currBCL->previous);
		}
		free(currBCL);
	}

	free(wallets);
	
	return;
}

void walletStatus(Wallet **wallets, char *walletID){
	Wallet *wallet;
	if ((wallet = findWallet(wallets, walletID)) == NULL){
		printf("WalletID specified does not exist.\n");
		return;
	}

	printf("Wallet with ID %s has %lu $.\n", wallet->walletID, wallet->amount);

	return;
}

void freeWalletsInitial(Wallet *wallets, size_t walletIndex){
	BCList *currBCL = NULL;
	for (size_t i = 0; i < walletIndex; i++){
		free(wallets[i].walletID);

		currBCL = wallets[i].firstBCL;
		while (currBCL != NULL){
			free(currBCL);
			currBCL = currBCL->next;
		}
	}
	free(wallets);
	return;
}