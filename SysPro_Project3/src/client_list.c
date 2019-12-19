/**** client_list.c ****/
#include "client_list.h"

int in_client_list(client_list *list, client_tuple *ct) {
    client_list *curr = list;
    if (curr == NULL) return 0;

    while (curr->next != NULL) {
        if (ct->IP_Address == curr->this_tuple->IP_Address && ct->portNum == curr->this_tuple->portNum) {
            return 1;
        }
        curr = curr->next;
    }
    if (ct->IP_Address == curr->this_tuple->IP_Address && ct->portNum == curr->this_tuple->portNum) {
        return 1;
    }

    return 0;
}

void client_list_add(client_list **list, client_tuple *ct) {
    client_list *curr = *list;
    if (curr == NULL) {
        *list = malloc(sizeof(client_list));
        curr = *list;
        curr->previous = NULL;
    }
    else {
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = malloc(sizeof(client_list));
        curr->next->previous = curr;
        curr = curr->next;
    }
    curr->next = NULL;
    curr->this_tuple = malloc(sizeof(client_tuple));
    memcpy(curr->this_tuple, ct, sizeof(client_tuple)); 

    return;
}

void delete_from_client_list(client_list **list, client_tuple *ct) {
    client_list *curr = *list;
    if (curr == NULL) return;

    while (curr->next != NULL) {
        if (curr->this_tuple->IP_Address == ct->IP_Address && curr->this_tuple->portNum == ct->portNum) {
            if (curr->next != NULL) {
                curr->next->previous = curr->previous;
            }
            if (curr->previous != NULL) {
                curr->previous->next = curr->next;
            }
            if (curr == (*list)) {
                (*list) = curr->next;
            }
            free(curr->this_tuple);
            free(curr);

            return;
        }

        curr = curr->next;
    }

    if (curr->this_tuple->IP_Address == ct->IP_Address && curr->this_tuple->portNum == ct->portNum) {
        if (curr->next != NULL) {
            curr->next->previous = curr->previous;
        }
        if (curr->previous != NULL) {
            curr->previous->next = curr->next;
        }

        if (curr == (*list)) {
            (*list) = curr->next;
        }
        free(curr->this_tuple);
        free(curr);

        return;
    }

    return;
}

void free_list(client_list *list) {
    client_list *curr = list;

    if (curr == NULL) {
        return;
    }

    while (curr->next != NULL) {
        curr = curr->next;
        free(curr->previous->this_tuple);
        free(curr->previous);
    }

    free(curr->this_tuple);
    free(curr);

    return;
}