/******* hashtable.c *******/
#include "hashTable.h"

/* struct HashTable{
	HashedItem **Table;
	size_t numOfEntries;
	size_t bucketSize;
};

struct HashedItem{
	TransList *first;
	TransList *last;
	char *walletID;
	HashedItem *nextItem;
};

struct TransList{
	TransList *previous;
	Transaction *thisTrans;
	TransList *next;
	Wallet *sourceDestWallet;
}; */

//hash function by Dan J. Bernstein, returns a number between 0 and number of entries of a hashTable
size_t djb2Hash(char *ID, size_t numOfEntries){
	unsigned long hash = 5381;
	int c;
	while ((c = *ID++))
		hash = ((hash << 5) + hash) +c;

	return hash % numOfEntries;	//make hash value fit in [0-numOfEntries) range
}

HashTable *initializeHashTable(size_t numOfEntries, size_t bucketSize){
	HashTable *HT = malloc(sizeof(HashTable));
	HT->Table = malloc(numOfEntries*(sizeof(HashedItem*)));
	HT->numOfEntries = numOfEntries;
	HT->bucketSize = bucketSize;
	size_t index = 0;
	size_t numOfRecords = bucketSize/sizeof(HashedItem);
	for (size_t i = 0; i < HT->numOfEntries; i++){
		index = 0;
		HT->Table[i] = malloc(HT->bucketSize);
		do{
			HT->Table[i][index].first = NULL;
			HT->Table[i][index].last = NULL;
			HT->Table[i][index].walletID = NULL;
			if (index == numOfRecords - 1)
				HT->Table[i][index].nextItem = NULL;
			else
				HT->Table[i][index].nextItem = &HT->Table[i][index+1];
			index++;
		} while (index < numOfRecords);
	}
	return HT;
}

//return pointer to a new bucket of size bucketSize
HashedItem *newBucket(size_t bucketSize){
	size_t index = 0;
	size_t numOfRecords = bucketSize/sizeof(HashedItem);
	HashedItem *hi = malloc(bucketSize);
	do{
		hi[index].first = NULL;
		hi[index].last = NULL;
		hi[index].walletID = NULL;
		if (index == numOfRecords - 1)
			hi[index].nextItem = NULL;
		else
			hi[index].nextItem = &hi[index+1];
		index++;
	} while (index < numOfRecords);
	return hi;
}

TransList *addTransaction(HashTable *senRecHashTable, Wallet *walletID, Wallet *senRecWalletID, Transaction *tmpTransaction){
	size_t hash = djb2Hash(walletID->walletID, senRecHashTable->numOfEntries);
	HashedItem *tmpItem = &senRecHashTable->Table[hash][0];

	while (tmpItem->walletID != NULL){
		if (!(strcmp(tmpItem->walletID, walletID->walletID))){	//if tmpItem contains a wallet and it is the wallet we are looking for
			if (tmpItem->first == NULL){		//if there are no previous transactions of walletID
				//initialize new list of TransList items
				tmpItem->first = malloc(sizeof(TransList));
				tmpItem->last = tmpItem->first;
				//initialize data of new TransList
				tmpItem->first->previous = NULL;
				tmpItem->first->next = NULL;
				tmpItem->first->sourceDestWallet = senRecWalletID;
				//data of new Transaction
				tmpItem->first->thisTrans = tmpTransaction;
				return tmpItem->first;
			}
			else{		//if there are more transactions of walletID
				//temporary helping variable
				TransList *tmpTransList = tmpItem->last;
				//next of last is the new TransList item
				tmpTransList->next = malloc(sizeof(TransList));
				//previous of new TransList item is the until-before last item
				tmpTransList->next->previous = tmpTransList;

				tmpTransList = tmpTransList->next;
				tmpTransList->next = NULL;
				tmpTransList->sourceDestWallet = senRecWalletID;
				tmpItem->last = tmpTransList;
				//data of new Transaction
				tmpTransList->thisTrans = tmpTransaction;
				return tmpTransList;
			}
		}
		else{		//if the walletIDs do not match
			if (tmpItem->nextItem != NULL)		//if the bucket has not yet been searched completely
				tmpItem = tmpItem->nextItem;
			else{		//if the bucket is full and the walletID does not exist
				//create new Bucket
				tmpItem->nextItem = newBucket(senRecHashTable->bucketSize);
				//move pointer to next item of bucket
				tmpItem = tmpItem->nextItem;
				//initialize data for new item of Bucket
				tmpItem->walletID = walletID->walletID;
				tmpItem->first = malloc(sizeof(TransList));
				tmpItem->last = tmpItem->first;
				//initialize data for new TransList
				tmpItem->first->previous = NULL;
				tmpItem->first->next = NULL;
				tmpItem->first->sourceDestWallet = senRecWalletID;
				//data of new Transaction
				tmpItem->first->thisTrans = tmpTransaction;
				return tmpItem->first;
			}
		}
	}
	//we reach this point only if the first bucket of secRecHashTable[hash] is empty

	//initialize data for new item of Bucket
	tmpItem->walletID = walletID->walletID;
	tmpItem->first = malloc(sizeof(TransList));
	tmpItem->last = tmpItem->first;
	//initialize data for new TransList
	tmpItem->first->previous = NULL;
	tmpItem->first->next = NULL;
	tmpItem->first->sourceDestWallet = senRecWalletID;
	//data of new Transaction
	tmpItem->first->thisTrans = tmpTransaction;

	return tmpItem->first;
}

void findPayments(HashTable *HT, char *walletID, time_t start, time_t end){
	FILE *strings;
	char *transID;
	char *receiverID;
	size_t transAmount;
	struct tm transTime = {0};
	char dateTime[32];
	HashedItem *item;
	size_t hash;
	size_t sum = 0;

	hash = djb2Hash(walletID, HT->numOfEntries);
	item = HT->Table[hash];

	if (item->walletID == NULL){			//in case this hashtable entry is empty
		printf("WalletID does not exist.\n");
		return;
	}

	while (strcmp(item->walletID, walletID)){
		if (item->nextItem == NULL){
			printf("WalletID does not exist.\n");
			return;
		}
		item = item->nextItem;
	}
	TransList *currTrans = item->first;
	if (currTrans == NULL){			//walletID has not sent/received any BitCoin
		printf("Sum = 0.\n");
		return;
	}

	if ((strings = fopen("./.tmpStr", "w+")) == NULL){ 	//w+ -> create a file for reading and writing
		printf("Temporary file cannot be created.\n");
		return;
	}

	if (start == 0){		//start == end == 0
		while (currTrans != NULL){
			sum += currTrans->thisTrans->value;
			transID = currTrans->thisTrans->transactionID;
			receiverID = currTrans->sourceDestWallet->walletID;
			transAmount = currTrans->thisTrans->value;
			gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);
			strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
			fprintf(strings, "%s %s %s %lu %s\n", transID, walletID, receiverID, transAmount, dateTime);
			currTrans = currTrans->next;
		}
	}
	else{
		struct tm startTM;
		gmtime_r(&start, &startTM);
		struct tm endTM;
		gmtime_r(&end, &endTM);

		if (startTM.tm_mday == 1 && startTM.tm_mon == 0 && startTM.tm_year == 70){				//the date is default and, therefore, we only have time restriction

			if (start > end){					//if for example we have 23:42 - 06:24

				while (currTrans != NULL){
					gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);		//get struct tm for ONLY time comparison
					if ((transTime.tm_hour < startTM.tm_hour && transTime.tm_hour < endTM.tm_hour) || ((transTime.tm_hour == startTM.tm_hour && transTime.tm_min > startTM.tm_hour)\
					|| (transTime.tm_hour == endTM.tm_hour && transTime.tm_min < endTM.tm_min))){
						sum += currTrans->thisTrans->value;
						transID = currTrans->thisTrans->transactionID;
						receiverID = currTrans->sourceDestWallet->walletID;
						transAmount = currTrans->thisTrans->value;
						strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
						fprintf(strings, "%s %s %s %lu %s\n", transID, walletID, receiverID, transAmount, dateTime);
					}
					currTrans = currTrans->next;
				}
			}
			else if (start < end){		//for example 13:20 - 21:54

				while (currTrans != NULL){
					gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);		//get struct tm for ONLY time comparison
					if ((transTime.tm_hour > startTM.tm_hour && transTime.tm_hour < endTM.tm_hour) || ((transTime.tm_hour == startTM.tm_hour && transTime.tm_min > startTM.tm_hour)\
					|| (transTime.tm_hour == endTM.tm_hour && transTime.tm_min < endTM.tm_min))){
						sum += currTrans->thisTrans->value;
						transID = currTrans->thisTrans->transactionID;
						receiverID = currTrans->sourceDestWallet->walletID;
						transAmount = currTrans->thisTrans->value;
						strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
						fprintf(strings, "%s %s %s %lu %s\n", transID, walletID, receiverID, transAmount, dateTime);
					}
					currTrans = currTrans->next;
				}
			}
		}
		else{												//we have either both time and date restriction or only date restriction (with ommitted time restriction)

			while (currTrans != NULL){
				if (start < currTrans->thisTrans->dateTime && end > currTrans->thisTrans->dateTime){
					sum += currTrans->thisTrans->value;
					transID = currTrans->thisTrans->transactionID;
					receiverID = currTrans->sourceDestWallet->walletID;
					transAmount = currTrans->thisTrans->value;
					gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);
					strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
					fprintf(strings, "%s %s %s %lu %s\n", transID, walletID, receiverID, transAmount, dateTime);
				}
				currTrans = currTrans->next;
			}
		}
	}

	printf("%lu\n", sum);

	rewind(strings);
	char c;
	while ( (c = fgetc(strings)) != EOF)
		putchar(c);


	putchar('\n');
	fclose(strings);

	if ((remove("./.tmp")) == 0)
		printf("Unable to delete temporary file.\n");
}

void findEarnings(HashTable *HT, char *walletID, time_t start, time_t end){
	FILE *strings;
	char *transID;
	char *senderID;
	size_t transAmount;
	struct tm transTime = {0};
	char dateTime[32];
	HashedItem *item;
	size_t hash;
	size_t sum = 0;

	hash = djb2Hash(walletID, HT->numOfEntries);
	item = HT->Table[hash];

	if (item->walletID == NULL){			//in case this hashtable entry is empty
		printf("WalletID does not exist.\n");
		return;
	}

	while (strcmp(item->walletID, walletID)){
		if (item->nextItem == NULL){
			printf("WalletID does not exist.\n");
			return;
		}
		item = item->nextItem;
	}
	TransList *currTrans = item->first;
	if (currTrans == NULL){			//walletID has not sent/received any BitCoin
		printf("Sum = 0.\n");
		return;
	}

	if ((strings = fopen("./.tmpStr", "w+")) == NULL){ 	//w+ -> create a file for reading and writing
		printf("Temporary file cannot be created.\n");
		return;
	}

	if (start == 0){		//start == end == 0


		while (currTrans != NULL){
			sum += currTrans->thisTrans->value;
			transID = currTrans->thisTrans->transactionID;
			senderID = currTrans->sourceDestWallet->walletID;
			transAmount = currTrans->thisTrans->value;
			gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);
			strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
			fprintf(strings, "%s %s %s %lu %s\n", transID, senderID, walletID, transAmount, dateTime);
			currTrans = currTrans->next;
		}

	}
	else{

		struct tm startTM;
		gmtime_r(&start, &startTM);
		struct tm endTM;
		gmtime_r(&end, &endTM);

		if (startTM.tm_mday == 1 && startTM.tm_mon == 0 && startTM.tm_year == 70){				//the date is default and, therefore, we only have time restriction


			if (start > end){					//if for example we have 23:42 - 06:24

				while (currTrans != NULL){
					gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);		//get struct tm for ONLY time comparison
					if ((transTime.tm_hour < startTM.tm_hour && transTime.tm_hour < endTM.tm_hour) || ((transTime.tm_hour == startTM.tm_hour && transTime.tm_min > startTM.tm_hour)\
					|| (transTime.tm_hour == endTM.tm_hour && transTime.tm_min < endTM.tm_min))){
						sum += currTrans->thisTrans->value;
						transID = currTrans->thisTrans->transactionID;
						senderID = currTrans->sourceDestWallet->walletID;
						transAmount = currTrans->thisTrans->value;
						strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
						fprintf(strings, "%s %s %s %lu %s\n", transID, senderID, walletID, transAmount, dateTime);
					}
					currTrans = currTrans->next;
				}
			}
			else if (start < end){		//for example 13:20 - 21:54

				while (currTrans != NULL){
					gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);		//get struct tm for ONLY time comparison
					if ((transTime.tm_hour > startTM.tm_hour && transTime.tm_hour < endTM.tm_hour) || ((transTime.tm_hour == startTM.tm_hour && transTime.tm_min > startTM.tm_hour)\
					|| (transTime.tm_hour == endTM.tm_hour && transTime.tm_min < endTM.tm_min))){
						sum += currTrans->thisTrans->value;
						transID = currTrans->thisTrans->transactionID;
						senderID = currTrans->sourceDestWallet->walletID;
						transAmount = currTrans->thisTrans->value;
						strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
						fprintf(strings, "%s %s %s %lu %s\n", transID, senderID, walletID, transAmount, dateTime);
					}
					currTrans = currTrans->next;
				}
			}
		}
		else{												//we have either both time and date restriction or only date restriction (with ommitted time restriction)

			while (currTrans != NULL){
				if (start < currTrans->thisTrans->dateTime && end > currTrans->thisTrans->dateTime){
					sum += currTrans->thisTrans->value;
					transID = currTrans->thisTrans->transactionID;
					senderID = currTrans->sourceDestWallet->walletID;
					transAmount = currTrans->thisTrans->value;
					gmtime_r(&(currTrans->thisTrans->dateTime), &transTime);
					strftime(dateTime, 32, "%d-%m-%y %H:%M", &transTime);
					fprintf(strings, "%s %s %s %lu %s\n", transID, senderID, walletID,transAmount, dateTime);
				}
				currTrans = currTrans->next;
			}
		}
	}

	printf("%lu\n", sum);

	rewind(strings);
	char c;
	while ( (c = fgetc(strings)) != EOF)
		putchar(c);


	putchar('\n');

	fclose(strings);

	if ((remove("./.tmp")) == 0)
		printf("Unable to delete temporary file.\n");
}

//frees the memory allocated for an item of a bucket (different from receiver function in that it frees the transaction data, too)
void freeSenderHashedItem(HashedItem *item){
	if (item->first == NULL)
		return;

	TransList *currList = item->first;
	while (currList->next != NULL){
		currList = currList->next;
		freeTransaction(currList->previous->thisTrans);
		free(currList->previous);
	}
	freeTransaction(currList->thisTrans);
	free(currList);
	return;
}

void freeSenderHashTable(HashTable *HT){
	HashedItem *currItem;
	HashedItem *currBucket;
	HashedItem *nextBucket;
	HashedItem *prevItem;
	size_t numOfRecords = HT->bucketSize/sizeof(HashedItem);

	for (size_t i = 0; i < HT->numOfEntries; i++){
		currItem = HT->Table[i];
		prevItem = currItem;
		currBucket = HT->Table[i];
		nextBucket = currBucket[numOfRecords - 1].nextItem;

		while (currItem->nextItem != NULL){
				currItem = currItem->nextItem;
				freeSenderHashedItem(prevItem);
				prevItem = currItem;
		}
		freeSenderHashedItem(currItem);

		while (nextBucket != NULL){
			free(currBucket);
			currBucket = nextBucket;
			nextBucket = currBucket[numOfRecords - 1].nextItem;
		}
		free(currBucket);
	}

	free(HT->Table);
	free(HT);

	return;
}

//avoids double free (free of already freed memory) because the pointers of already free memory cannot be set to NULL (and there is no need)
void freeReceiverHashedItem(HashedItem *item){
	if (item->first == NULL)
		return;
	TransList *currList = item->first;
	while (currList->next != NULL){
		currList = currList->next;
		free(currList->previous);
	}

	free(currList);
	return;
}

void freeReceiverHashTable(HashTable *HT){
	HashedItem *currItem;
	HashedItem *currBucket;
	HashedItem *nextBucket;
	HashedItem *prevItem;
	size_t numOfRecords = HT->bucketSize/sizeof(HashedItem);

	for (size_t i = 0; i < HT->numOfEntries; i++){
		currItem = HT->Table[i];
		prevItem = currItem;
		currBucket = HT->Table[i];
		nextBucket = currBucket[numOfRecords - 1].nextItem;

		while (currItem->nextItem != NULL){
				currItem = currItem->nextItem;
				freeReceiverHashedItem(prevItem);
				prevItem = currItem;
		}

		freeReceiverHashedItem(currItem);

		while (nextBucket != NULL){
			free(currBucket);
			currBucket = nextBucket;
			nextBucket = currBucket[numOfRecords - 1].nextItem;
		}
		free(currBucket);
	}

	free(HT->Table);
	free(HT);

	return;	
}