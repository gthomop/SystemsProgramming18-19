/****** use.c ******/
#include "use.h"
#include <errno.h>

void printError(){	//write once the usage error message
	printf("Wrong use of ./bitcoin. Proper use:\n./bitcoin -a bitCoinBalancesFile -t transactionsFile -v bitCoinValue -h1 senderHashTableNumOfEntries -h2 receiverHastTableNumOfEntries -b bucketSize\n");
}

CmdParams use(int argc, char **argv){
	CmdParams args;
	for (int i = 0; i < 6; i++)
		args.params[i] = NULL;

	if (argc == 1){		//only one argument means that there are no arguments apart from the programme name
		printError();
		args.rightUsage = 0;
		return args;	//returns False for wrong use
	}
	else{ 	//checks whether there are all needed parameters on program execution
		char *validParams[] = {"-a", "-t", "-v", "-h1", "-h2", "-b"};
		int flag = 0;
		int foundParams = 0;
		for (int i = 1; i < argc; i += 2){	//by incrementing by 2, I ensure that there are arguments after the options required
			for (int j = 0; j < 6; j++){
				if (!(strcmp(argv[i], validParams[j]))){
					foundParams++;
					if (!(strcmp(argv[i], "-a"))){
						args.params[0] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[0],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-t"))){
						args.params[1] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[1],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-v"))){
						args.params[2] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[2],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-h1"))){
						args.params[3] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[3],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-h2"))){
						args.params[4] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[4],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-b"))){
						args.params[5] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[5],argv[i+1]);
					}
					flag = 1;
					break;
				}
				if (argc - 1 == i){	//no arguments after last option
					printError();
					args.rightUsage = 0;
					freeCmdParams(&args);
					return args;
				}
			}
			if (flag == 0){		//argument found is invalid
				printError();
				args.rightUsage = 0;
				freeCmdParams(&args);
				return args;
			}
			else			//check next argument
				flag = 0;
		}
		if (foundParams != 6){		//less arguments than required
			printError();
			args.rightUsage = 0;
			freeCmdParams(&args);
			return args;
		}
		args.rightUsage = 1;
		return args;	//no errors found
	}
}

void freeCmdParams(CmdParams *arguments){		//free any allocated memory
	for (int i = 0; i < 6; i++)
		free(arguments->params[i]);
}

void takeCommands(Wallet **wallets, HashTable *senderHashTable, HashTable *receiverHashTable, BitCoin *allBitCoins){
	//errno is used here to distinguish 0 returned by strtol between valid and invalid (return 0 if strtol read nothing)
	errno = 0;
	//getline reallocs as much memory as it needs in order to read from stream (stdin)
	char *cmd = NULL;
	size_t nullBuff = 0;
	size_t len = 0;

	char *commandString = NULL;
	//variable that help keep track of where in the cmd string we are after strtok and strtol
	char *str1 = NULL;

	while (1){
		if (commandString == NULL){
			do{
				printf("Give command:\n");
				cmd = NULL;
				nullBuff = 0;
				len = getline(&cmd, &nullBuff, stdin);
				if (len < 1){			//len = 0 if endline was read instantly
					free(cmd);
					continue;
				}
				//getline read the '\n' character and puts it in the string
				cmd[len-1] = 0;
				len--;
				str1 = cmd;	//need to remember the location of memory reallocated by getline in order to free it
				commandString = strtok(str1, " ");
			} while (commandString == NULL);		//this check exists so as to bypass empty commands (" ") given by the user
		}

		if (!(strcmp(commandString, "exit"))){		//EXIT
			printf("Program exits.\n");
			free(cmd);
			return;
		}
		else if (!(strcmp(commandString, "requestTransaction"))){
			char *sndrID = strtok(NULL, " ");
			if (sndrID == NULL){
				printf("No senderWalletID read.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}
			char *rcvrID = strtok(NULL, " ");
			if (rcvrID == NULL){
				printf("No receiverWalletID read.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}
			if ((str1 = strtok(NULL, " ")) == NULL){		//move str1 to amount entry
				printf("Invalid transaction entered.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}

			size_t amount = strtol(str1, &str1, 10);

			if (!amount){		//strtol return 0 if it did not read any value
				printf("Invalid amount.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}

			struct tm dateTimeTM = {0};
			time_t dateTime = 0;

			str1++;		//skip ' ' after amount
			if (!(dateTimeTM.tm_mday = strtol(str1, &str1, 10))){
				dateTime = 0;
			}
			else{
				str1++;
				if ((dateTimeTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
					printf("Month was not specified.\n");
					free(cmd);
					commandString = NULL;
					continue;
				}
				str1++;
				if ((dateTimeTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
					printf("Year was not specified.\n");
					free(cmd);
					commandString = NULL;
					continue;
				}
				str1++;
				errno = 0;
				if (!(dateTimeTM.tm_hour = strtol(str1, &str1, 10))){
					if (errno){
						printf("Hour was not specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
				}
				str1++;
				if (!(dateTimeTM.tm_min = strtol(str1, &str1, 10))){
					if (errno){
						printf("No minutes specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
				}

				dateTime = timegm(&dateTimeTM);
			}

			requestTransaction(sndrID, rcvrID, amount, dateTime, wallets, senderHashTable, receiverHashTable);
			free(cmd);
			commandString = NULL;
			continue;
		}
		else if (!(strcmp(commandString, "requestTransactions"))){
			if (cmd[len - 1] != ';'){					//FROM FILE
				char *filename = strtok(NULL, " ");
				if (filename == NULL){
					printf("No filename specified.\n");
					free(cmd);
					commandString = NULL;
					continue;
				}
				FILE *file = fopen(filename, "r");
				if (file == NULL){
					printf("File %s was not read properly.\n", filename);
					free(cmd);
					commandString = NULL;
					continue;
				}

				free(cmd);
				cmd = NULL;
				nullBuff = 0;
				char *sndrID = NULL;
				char *rcvrID = NULL;
				size_t amount = 0;
				struct tm dateTimeTM = {0};
				time_t dateTime = 0;

				while ((len = getline(&cmd, &nullBuff, file)) != -1){	//-1 indicates EOF
					if (len < 1){		//empty line
						free(cmd);
						cmd = NULL;
						nullBuff = 0;
						continue;
					}
					cmd[len - 1] = 0;
					len--;
					str1 = cmd;

					sndrID = strtok(str1, " ");
					if (sndrID == NULL){
						printf("No senderWalletID read.\n");
						free(cmd);
						cmd = NULL;
						nullBuff = 0;
						continue;
					}
					rcvrID = strtok(NULL, " ");
					if (rcvrID == NULL){
						printf("No receiverWalletID read.\n");
						free(cmd);
						cmd = NULL;
						nullBuff = 0;
						continue;
					}
					if ((str1 = strtok(NULL, " ")) == NULL){		//move str1 to amount entry
						printf("Invalid transaction entered.\n");
						free(cmd);
						cmd = NULL;
						nullBuff = 0;
						continue;
					}
					if (!(amount = strtol(str1, &str1, 10))){		//strtol return 0 if it did not read any value
						printf("Invalid amount.\n");
						free(cmd);
						cmd = NULL;
						nullBuff = 0;
						continue;
					}

					str1++;		//skip ' ' after amount
					if ((dateTimeTM.tm_mday = strtol(str1, &str1, 10) == 0)){
						dateTime = 0;
					}
					else{
						str1++;
						if ((dateTimeTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
							printf("Month was not specified.\n");
							free(cmd);
							cmd = NULL;
							nullBuff = 0;
							continue;
						}
						str1++;
						if ((dateTimeTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
							printf("Year was not specified.\n");
							free(cmd);
							cmd = NULL;
							nullBuff = 0;
							continue;
						}
						str1++;
						errno = 0;
						if ((dateTimeTM.tm_hour = strtol(str1, &str1, 10)) == 0){
							if (errno){
								printf("Hour was not specified.\n");
								free(cmd);
								cmd = NULL;
								nullBuff = 0;
								continue;
							}
						}
						str1++;
						if ((dateTimeTM.tm_min = strtol(str1, &str1, 10)) == 0){
							if (errno){
								printf("No minutes specified.\n");
								free(cmd);
								cmd = NULL;
								nullBuff = 0;
								continue;
							}
						}
						dateTime = timegm(&dateTimeTM);
					}

					requestTransaction(sndrID, rcvrID, amount, dateTime, wallets, senderHashTable, receiverHashTable);
					free(cmd);
					cmd = NULL;
					nullBuff = 0;
				}
				fclose(file);
				free(cmd);
				cmd = NULL;
				commandString = NULL;
				continue;
			}
			else{										//MULTIPLE TRANSACTIONS
				char *sndrID = strtok(NULL, " ");
				char *rcvrID = strtok(NULL, " ");
				while (!(strcmp(rcvrID,";")) || rcvrID == NULL){
					printf("No receiverWalletID read.\n");
					free(cmd);
					do{
						cmd = NULL;
						nullBuff = 0;
						len = getline(&cmd, &nullBuff, stdin);
						if (len < 1){
							free(cmd);
							continue;
						}
						cmd[len-1] = 0;
						len--;
						str1 = cmd;
						sndrID = strtok(str1, " ");
					} while (commandString == NULL);

					rcvrID = strtok(NULL, " ");
				}
				size_t amount = 0;
				struct tm dateTimeTM = {0};
				time_t dateTime = 0;
				while (1){
					if ((str1 = strtok(NULL, " ")) == NULL){
						printf("Invalid transaction entered.\n");
						free(cmd);
						commandString = NULL;
					}
					else{
						if (!(amount = strtol(str1, &str1, 10))){		//strtol return 0 if it did not read any value
							printf("Invalid amount.\n");
							free(cmd);
							commandString = NULL;
						}
						else{
							str1++;
							if (!(dateTimeTM.tm_mday = (strtol(str1, &str1, 10)))){
								dateTime = 0;
								requestTransaction(sndrID, rcvrID, amount, dateTime, wallets, senderHashTable, receiverHashTable);

							}
							else{
								str1++;
								if ((dateTimeTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
									printf("Month was not specified.\n");
									free(cmd);
									commandString = NULL;
								}
								else{
									str1++;
									if ((dateTimeTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
										printf("Year was not specified.\n");
										free(cmd);
										commandString = NULL;
									}
									else{

										str1++;
										errno = 0;
										if ((dateTimeTM.tm_hour = strtol(str1, &str1, 10)) == 0 && errno){
											printf("Hour was not specified.\n");
											free(cmd);
											commandString = NULL;
										}
										else{
											str1++;
											if ((dateTimeTM.tm_min = strtol(str1, &str1, 10)) == 0 && errno){
												printf("No minutes specified.\n");
												free(cmd);
												commandString = NULL;
											}
											else{
												dateTime = timegm(&dateTimeTM);
												requestTransaction(sndrID, rcvrID, amount, dateTime, wallets, senderHashTable, receiverHashTable);
											}
										}
									}
								}
							}
						}
						free(cmd);
					}


					do{
						printf("Enter new transaction or command: ");
						cmd = NULL;
						nullBuff = 0;
						len = getline(&cmd, &nullBuff, stdin);
						if (len < 1){
							free(cmd);
							continue;
						}
						cmd[len - 1] = 0;
						len--;
						str1 = cmd;
						commandString = strtok(str1," ");
					} while (commandString == NULL);

					if (cmd[len - 1] == ';'){
						sndrID = commandString;
						rcvrID = strtok(NULL, " ");
					}
					else{
						break;		//break this while and continue the bigger while executing commands
					}
				}
				continue;
			}
		}
		else if (!(strcmp(commandString, "findEarnings"))){
			char *walletID = strtok(NULL, " ");
			if (walletID == NULL){
				printf("No wallet specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}
			int dateFlag = 0;

			time_t startTime = 0;
			time_t endTime = 0;

			struct tm startTM = {0};
			struct tm endTM = {0};

			//if date is default (1 January 1970), then show all earnings made on a certain time range
			//else if date is not default but hours:minutes are 00:00(start) and 23:59(end), then show all earnings made between these 2 dates
			//else if neither date nor time is specified, show all earnings

			if ((str1 = strtok(NULL, " ")) == NULL){
				startTime = 0;
				endTime = 0;
			}
			else{
				//str1 now is either starting time or starting date

				//str1 is starting time
				if (str1[2] == ':'){
					errno = 0;
					if (!(startTM.tm_hour = strtol(str1, &str1, 10))){
						if (errno){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					str1++;			//skip ":"
					if (!(startTM.tm_min = strtol(str1, &str1, 10))){
						if (errno){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					str1++;			//skip " "

					if ((str1 = strtok(NULL," ")) == NULL){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}


					//str1 is now either starting date or end time

					//str1 is starting date
					if (str1[2] == '-'){
						dateFlag = 1;
						if (!(startTM.tm_mday = strtol(str1, &str1, 10))){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;			//skip '-' of date
						if ((startTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
							printf("Wrong time specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;
						if ((startTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}

						if ((str1 = strtok(NULL, " ")) == NULL){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}

					//str1 is end time
					errno = 0;
					if (!(endTM.tm_hour = strtol(str1, &str1, 10))){
						if (errno){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					str1++;
					if(!(endTM.tm_min = strtol(str1, &str1, 10))){
						if (errno){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					str1++;

					if ((str1 = strtok(NULL, " ")) == NULL && !(dateFlag)){		//expect end date, but there is not
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					else if (dateFlag){
						if (!(endTM.tm_mday = strtol(str1, &str1, 10))){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;
						if ((endTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;
						if ((endTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					else{
						startTM.tm_mday = 1;
						startTM.tm_year = 1970 - 1900;

						endTM.tm_mday = 1;
						endTM.tm_year = 1970 - 1900;
					}

					startTime = timegm(&startTM);
					endTime = timegm(&endTM);

					if (startTime > endTime && dateFlag){				//if there are no dates specified, it is ok if we have greater StartTime
						printf("Invalid time range.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
				}
				//str1 is starting date and we expect only starting and end date
				else if (str1[2] == '-'){
					if (!(startTM.tm_mday = strtol(str1, &str1, 10))){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((startTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((startTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;

					if ((str1 = strtok(NULL, " ")) == NULL){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}

					if (!(endTM.tm_mday = strtol(str1, &str1, 10))){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((endTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((endTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}

					startTM.tm_hour = 0;
					startTM.tm_min = 0;
					endTM.tm_hour = 23;
					endTM.tm_min = 59;

					startTime = timegm(&startTM);
					endTime = timegm(&endTM);

					if (startTime > endTime){
						printf("Invalid time range.\n");
						free(cmd);
						commandString = 0;
						continue;
					}
				}
			}
			findEarnings(receiverHashTable, walletID, startTime, endTime);
			free(cmd);
			commandString = 0;
			continue;
		}
		else if (!(strcmp(commandString, "findPayments"))){
			char *walletID = strtok(NULL, " ");
			if (walletID == NULL){
				printf("No wallet specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}
			int dateFlag = 0;

			time_t startTime = 0;
			time_t endTime = 0;

			struct tm startTM = {0};
			struct tm endTM = {0};


			//if both dates are default (1 January 1970) but time isn't, then show all payments made on this time range
			//else if date is not default but hours:minutes are 00:00(start) and 23:59(end), then show all payments made between these 2 dates
			//else if neither date nor time is specified, show all payments

			if ((str1 = strtok(NULL, " ")) == NULL){
				startTime = 0;
				endTime = 0;
			}
			else{
				//str1 now is either starting time or starting date

				//str1 is starting time
				if (str1[2] == ':'){
					errno = 0;
					if (!(startTM.tm_hour = strtol(str1, &str1, 10))){
						if (errno){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					str1++;			//skip ":"
					if (!(startTM.tm_min = strtol(str1, &str1, 10))){
						if (errno){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					str1++;			//skip " "

					if ((str1 = strtok(NULL," ")) == NULL){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}


					//str1 is now either starting date or end time

					//str1 is starting date
					if (str1[2] == '-'){
						dateFlag = 1;
						if (!(startTM.tm_mday = strtol(str1, &str1, 10))){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;			//skip '-' of date
						if ((startTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
							printf("Wrong time specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;
						if ((startTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}

						if ((str1 = strtok(NULL, " ")) == NULL){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}

					//str1 is end time
					if (!(endTM.tm_hour = strtol(str1, &str1, 10))){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if(!(endTM.tm_min = strtol(str1, &str1, 10))){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;

					if ((str1 = strtok(NULL, " ")) == NULL && !(dateFlag)){		//expect end date, but there is not
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					else if (dateFlag){
						if (!(endTM.tm_mday = strtol(str1, &str1, 10))){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;
						if ((endTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
						str1++;
						if ((endTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
							printf("Wrong time range specified.\n");
							free(cmd);
							commandString = NULL;
							continue;
						}
					}
					else{
						startTM.tm_mday = 1;
						startTM.tm_year = 1970 - 1900;

						endTM.tm_mday = 1;
						endTM.tm_year = 1970 - 1900;
					}

					startTime = timegm(&startTM);
					endTime = timegm(&endTM);

					if (startTime > endTime && dateFlag){			//if no date has been specified, then it is ok to have greater startTime
						printf("Invalid time range.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
				}
				//str1 is starting date and we expect only starting and end date
				else if (str1[2] == '-'){
					if (!(startTM.tm_mday = strtol(str1, &str1, 10))){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((startTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((startTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;

					if ((str1 = strtok(NULL, " ")) == NULL){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}

					if (!(endTM.tm_mday = strtol(str1, &str1, 10))){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((endTM.tm_mon = strtol(str1, &str1, 10) - 1) == -1){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}
					str1++;
					if ((endTM.tm_year = strtol(str1, &str1, 10) - 1900) == -1900){
						printf("Wrong time range specified.\n");
						free(cmd);
						commandString = NULL;
						continue;
					}

					startTM.tm_hour = 0;
					startTM.tm_min = 0;
					endTM.tm_hour = 23;
					endTM.tm_min = 59;

					startTime = timegm(&startTM);
					endTime = timegm(&endTM);

					if (startTime > endTime){
						printf("Invalid time range.\n");
						free(cmd);
						commandString = 0;
						continue;
					}
				}
			}
			findPayments(senderHashTable, walletID, startTime, endTime);
			free(cmd);
			commandString = 0;
			continue;
		}
		else if (!(strcmp(commandString, "walletStatus"))){
			char *walletID = strtok(NULL, " ");
			if (walletID == NULL){
				printf("No walletID specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}
			walletStatus(wallets, walletID);
			free(cmd);
			commandString = NULL;
			continue;
		}
		else if (!(strcmp(commandString, "bitCoinStatus"))){
			str1 = strtok(NULL," ");
			if (str1 == NULL){
				printf("No BitCoinID specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}
			size_t BCID = (size_t)(strtol(str1, &str1, 10));
			errno = 0;
			if (!BCID && errno){
				printf("No BitCoinID specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}

			bitCoinStatus(BCID, allBitCoins);
			free(cmd);
			commandString = NULL;
			continue;
		}
		else if (!(strcmp(commandString, "traceCoin"))){
			str1 = strtok(NULL, " ");
			if (str1 == NULL){
				printf("No BitCoinID specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}

			errno = 0;
			size_t BCID = strtol(str1, &str1, 10);

			if (!BCID && errno){
				printf("No BitCoinID specified.\n");
				free(cmd);
				commandString = NULL;
				continue;
			}

			traceCoin(BCID, allBitCoins);
			free(cmd);
			commandString = NULL;
			continue;
		}
		else{		//no arguments after the first command which is not exit
			printf("Wrong command, try:\n/requestTransaction senderWalletID receiverWalletID amount date time\n/requestTransactions senderWalletID receiverWalletID amount date time;\nsenderWalletID2 receiverWalletID2 amount2 date2 time2;\n...\nsenderWalletIDn receiverWalletIDn amountn daten timen;\n/requestTransactions inputFile\n/findEarnings walletID [time1][year1][time2][year2]\n/findPayments walletID [time1][year1][time2][year2]\n/walletStatus walletID\n/bitCoinStatus bitCoinID\n/traceCoin bitCoinID\n/exit\n");
			free(cmd);
			cmd = NULL;
			commandString = NULL;
			continue;	//read again
		}
	}

	return;
}
