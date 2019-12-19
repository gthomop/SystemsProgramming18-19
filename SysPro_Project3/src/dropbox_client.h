/**** dropbox_client.h ****/
#ifndef _DROPBOX_CLIENT_H
#define _DROPBOX_CLIENT_H

#include "buffer.h"
#include "client_list.h"
#include "dirControl.h"
#include "file_structs.h"
#include "messages.h"
#include "use_client.h"

#include <arpa/inet.h>      //hton
#include <errno.h>
#include <fcntl.h>          //open
#include <netdb.h>
#include <netinet/in.h>     //socket structs
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif