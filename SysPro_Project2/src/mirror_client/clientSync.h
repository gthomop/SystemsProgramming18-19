/******** clientSync.h ********/
#ifndef CLIENTSYNC_H
#define CLIENTSYNC_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dirControl.h"

typedef struct client client;
typedef struct children_list children_list;
typedef int client_id;

struct client{
  char id[32];
  char *mirror_dir_path;
  char *input_dir_path;
  char *common_dir_path;
  char *log_file_path;
  children_list *first_child;
};

struct children_list{     //in order to make sure every child process has ended on exit
  pid_t last_pid;
  char *to_sync_with;
  int times;
  children_list *next;
  children_list *previous;
};

client_id check_common_dir(client *cl);
int cl_still_exists(client *cl, char *id);
client_id retry(client *cl, pid_t sent_usr1);
client_id synch(char *new_client_id, client *cl, int tries);
void add_pid(client *cl, pid_t pid, char *id);
int times_tried(client *cl, char *to_sync_with);
int check_for_deleted_clients(client *cl);
int already_synced(client *cl, char *new_id);
void remove_list_node(client *cl, children_list *curr_child);
int delete_client(char *id_to_delete, client *cl);
void free_client(client *cl);
void delete_on_exit(client *cl);
void free_on_exec(client *cl);
void kill_processes(client *cl);
void delete_entry(client *cl, char *id_to_delete);

#endif
