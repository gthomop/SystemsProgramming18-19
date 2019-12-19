/****** use.h ******/
#ifndef _USE_H
#define _USE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//auxiliary struct for parsing program arguments
typedef struct CmdParams{
	int rightUsage;
	char *params[6];
} CmdParams;

CmdParams use(int argc, char **argv);
//free memory allocated for arguments' strings
void freeCmdParams(CmdParams *arguments);

#endif