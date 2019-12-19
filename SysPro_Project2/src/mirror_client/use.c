/****** use.c ******/
#include "use.h"

void printError(){	//write once the usage error message
	printf("Wrong use of ./mirror_client. Proper use:\n./mirror_client -n id -c common_dir -i input_dir -m mirror_dir -b buffer_size -l log_file\n");
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
		char *validParams[] = {"-n", "-c", "-i", "-m", "-b", "-l"};
		int flag = 0;
		int foundParams = 0;
		for (int i = 1; i < argc; i += 2){	//by incrementing by 2, I ensure that there are arguments after the options required
			for (int j = 0; j < 6; j++){
				if (!(strcmp(argv[i], validParams[j]))){
					foundParams++;
					if (!(strcmp(argv[i], "-n"))){
						args.params[0] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[0],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-c"))){
						args.params[1] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[1],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-i"))){
						args.params[2] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[2],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-m"))){
						args.params[3] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[3],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-b"))){
						args.params[4] = malloc(sizeof(char)*(1 + strlen(argv[i+1])));
						strcpy(args.params[4],argv[i+1]);
					}
					else if (!(strcmp(argv[i], "-l"))){
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