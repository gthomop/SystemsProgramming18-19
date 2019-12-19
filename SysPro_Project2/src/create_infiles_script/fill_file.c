#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char string[] = "1234567890AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";

int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr, "No file names specified.\n");
		return 1;
	}
	
	int len = strlen(string);	//length of string containing alphanumeric chars
	int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	srand(time(NULL));		//rand seed

	int num_of_kbs = rand() % 128 + 1;		//random number 1-128 (kilobytes)

	for (int i = 0; i < num_of_kbs; i++){
		for (int j = 0; j < 1024; j++){
			int pos = rand() % len;		//position in string to hold a char
			int w = write(fd, &string[pos], 1);	//write the char to file
		}
	}
	
	close(fd);
	return 0;
}
