/**** client_list.h ****/
#ifndef _CLIENT_LIST_H
#define _CLIENT_LIST_H

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

typedef struct client_tuple{
    in_addr_t IP_Address;
    int portNum;
} client_tuple;

typedef struct client_list client_list;
struct client_list{
    client_tuple *this_tuple;
    client_list *next;
    client_list *previous;
};

int in_client_list(client_list *list, client_tuple *ct);
void client_list_add(client_list **list, client_tuple *ct);
void delete_from_client_list(client_list **list, client_tuple *ct);
void free_list(client_list *list);

#endif