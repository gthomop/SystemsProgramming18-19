/****** transaction.c ******/
#include "transaction.h"

char maxID[15] = "";
time_t lastDateTime = 0;

/* struct Transaction{
	char *transactionID;
	size_t value;
	time_t dateTime;
}; */

//return 1 if transaction with ID transID has already been added
int checkIfTransExists(char **transIDs, size_t numOfTransactions, char *transID){

	for (size_t i = 0; i < numOfTransactions; i++)
		if (transIDs[i] != NULL){
			if (!(strcmp(transIDs[i], transID)))
				return 1;	//already exists
		}
	return 0;
}

//returns the number of lines in transactionsFile (so many transactions)
size_t findNumOfTransactions(FILE *file){	//count transactions in file
	char c = 'A';
	size_t transactions = 0;
	int flag = 0;			//the flag states that at least one character was read, indicating that the file has at least one transaction. This is important in case the file only contains one line.

	while ((c = fgetc(file)) != EOF){
		flag = 1;
		if (c == '\n')
			transactions++;
	}
	rewind(file);

	if (!(flag))	return 0;
	else if (flag && !(transactions))	return 1;
	return ++transactions;
}

int transactionsInitialization(char *transactionsFile, Wallet **wallets, HashTable *senderHT, HashTable *receiverHT){
	FILE *file = fopen(transactionsFile, "r");
	if (file == NULL){
		printf("File: %s was not read properly. Program is terminated.\n",transactionsFile);
		return 1;
	}


	size_t numOfTransactions = findNumOfTransactions(file);
	if (!(numOfTransactions)){
		printf("No transactions found on transactionsFile. Program is terminated.\n");
		return 1;
	}

	char **transIDs = malloc(sizeof(char*) * numOfTransactions);
	for (size_t i = 0; i < numOfTransactions; i++)
		transIDs[i] = NULL;

	int transIDExists = 0;
	char tmpID[64];
	char sender[64];
	char receiver[64];
	size_t tmpValue = 0;
	struct tm tmpDateTimeTM = {0};
	time_t tmpDateTime = 0;
	TransList *sentTransaction = NULL;
	Wallet *senderWalletID = NULL;
	Wallet *receiverWalletID = NULL;
	char c = 'A';
	Transaction *tmpTransaction = NULL;

	for(size_t i = 0; i < numOfTransactions; i++){
		if ((transIDExists = fscanf(file, "%s ",tmpID)) == -1)	break;

		fscanf(file, "%s ", sender);

		if ((senderWalletID = findWallet(wallets, sender)) == NULL){
			printf("senderWalletID %s on line %lu does not exist\n",sender, i + 1);
			while ((c = fgetc(file)) != '\n' && c != EOF){;}
			continue;
		}

		if (!(checkIfTransExists(transIDs, i, tmpID))){
			fscanf(file, "%s ", receiver);

			if ((receiverWalletID = findWallet(wallets, receiver)) == NULL){
				printf("receiverWalletID on line %lu does not exist\n", i + 1);
				while ((c = fgetc(file)) != '\n' && c != EOF){;}
				continue;
			}

			if ((strcmp(sender, receiver))){
				transIDs[i] = malloc(sizeof(char) * (1 + strlen(tmpID)));
				strcpy(transIDs[i], tmpID);

				fscanf(file, "%lu ", &tmpValue);
				if(senderHasEnough(senderWalletID, tmpValue)){
					fscanf(file, "%d-", &tmpDateTimeTM.tm_mday);	//tm_day saves the day of the month [1-31]
					fscanf(file, "%d-", &tmpDateTimeTM.tm_mon);
					tmpDateTimeTM.tm_mon -= 1;		//tm_mon saves months since January [0-12]
					fscanf(file, "%d ", &tmpDateTimeTM.tm_year);
					tmpDateTimeTM.tm_year -= 1900;	//tm_year saves how many years have passed since 1900
					fscanf(file, "%d:", &tmpDateTimeTM.tm_hour);	//tm_hour saves hours [0-23]
					fscanf(file, "%d ", &tmpDateTimeTM.tm_min);	//tm_min saves minutes [0-59]

					tmpDateTime = timegm(&tmpDateTimeTM);

					tmpTransaction = malloc(sizeof(Transaction));
					tmpTransaction->dateTime = tmpDateTime;
					tmpTransaction->transactionID = transIDs[i];
					tmpTransaction->value = tmpValue;

					sentTransaction = addTransaction(senderHT, senderWalletID, receiverWalletID, tmpTransaction);
					addTransaction(receiverHT, receiverWalletID, senderWalletID, tmpTransaction);
					makeTransaction(senderWalletID, receiverWalletID, tmpValue, sentTransaction);
				}
				else{
					printf("Sender %s does not have enough credits to make transaction %s.\n", sender, tmpID);
					while ((c = fgetc(file)) != '\n' && c != EOF){;}
					free(transIDs[i]);
					transIDs[i] = NULL;
					continue;
				}
			}
			else{
				printf("Transaction on line %lu is not valid and will be skipped.\n", i+1);
				while ((c = fgetc(file)) != '\n' && c != EOF){;}
				continue;
			}
		}
		else{
			printf("Transaction on line %lu is not valid and will be skipped.\n", i+1);
			while ((c = fgetc(file)) != '\n' && c != EOF){;}
			continue;
		}

		if ((strcmp(maxID, tmpID)) < 0)
			strcpy(maxID, tmpID);

	}
	lastDateTime = tmpDateTime;	//transactionsFile is in ascending date/time order

	free(transIDs);

	fclose(file);

	return 0;
}

//gives a new ID for requestTransaction
char *newID(){
	size_t index = 0;
	char *new = malloc(sizeof(char) * 15);
	strcpy(new, maxID);
	do{
		if (new[index] < 48)		//if the new[index] character is zero
			new[index] = 48;
		else if (new[index] < 126)	//if the new[index] character is not the last ascii character (~)
			new[index]++;
		else if (index < 14){		//if the new[index] character is (~), go to next character
			index++;
			continue;
		}
		else{				//if we have reached the limit of maximum IDs
			free(new);		//return NULL pointer
			return NULL;
		}

	} while (!(strcmp(maxID,new)));	//do this while we have have not a new unique ID

	strcpy(maxID, new);
	return new;
}

void requestTransaction(char *sndrID, char *rcvrID, size_t amount, time_t dateTime, Wallet **wallets, HashTable *senderHashTable, HashTable *receiverHashTable){
	if (!(strcmp(sndrID, rcvrID))){
		printf("Transactions from and to the same Wallet ID cannot be completed.\n");
		return;
	}

	if (dateTime && (difftime(dateTime, lastDateTime)) < 0){
		printf("Invalid transaction date/time\n");
		return;
	}
	else if (!(dateTime)){
		dateTime = time(NULL) + 7200;		//time(NULL) returns gmt time, however, we assume that all hours are utc+2 (2 hours x 3600 seconds)
	}

	Wallet *sndr;
	if ((sndr = findWallet(wallets, sndrID)) == NULL){
		printf("SenderWalletID does not exist.\n");
		return;
	}
	Wallet *rcvr;
	if((rcvr = findWallet(wallets, rcvrID)) == NULL){
		printf("ReceiverWalletID does not exist.\n");
		return;
	}

	if ((!senderHasEnough(sndr, amount))){
		printf("SenderWalletID does not have enough credits.\n");
		return;
	}

	char *transID = newID();
	if (transID == NULL){
		printf("Cannot add more transactions due to ID limitations.\n");
		return;
	}

	Transaction *tmpTransaction = malloc(sizeof(Transaction));
	tmpTransaction->transactionID = transID;
	tmpTransaction->dateTime = dateTime;
	tmpTransaction->value = amount;

	TransList *sentTransaction = addTransaction(senderHashTable, sndr, rcvr, tmpTransaction);
	addTransaction(receiverHashTable, rcvr, sndr, tmpTransaction);

	makeTransaction(sndr, rcvr, amount, sentTransaction);

	struct tm tmpTime = {0};
	gmtime_r(&dateTime, &tmpTime);
	char timeString[32];
	strftime(timeString, 32, "%d-%m-%y %H:%M", &tmpTime);

	printf("Successfuly transferred %lu $ from %s to %s on %s.\n", amount, sndr->walletID, rcvr->walletID, timeString);

	lastDateTime = dateTime;

	return;
}

void freeTransaction(Transaction *transaction){
	free(transaction->transactionID);
	free(transaction);
	return;
}