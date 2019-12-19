/******* clientSync.c ******/
#include "clientSync.h"

int sync_flag = 0;
int pipefd[2];

void usr_handler(int sig, siginfo_t *info, void *v){    //luckily, sigaction blocks a signal received in its handler
  if (sig == SIGUSR1){
    sync_flag = 2;
  }
  else if (sig == SIGALRM){
    sync_flag = -1;
  }
  else{
    sync_flag = 1;
  }
  return;
}

/*
-opens a pipe in order to keep track of the SIGUSR1 signals received by the child processes
-sets the signal handler for SIGUSR1 with sigaction in order to receive info about which process sent the signal
-sets the signal handler for SIGINT/SIGQUIT in order to exit normally and for SIGALRM in order to pause for 3 seconds exactly and not be interrupted by any other signal
-while SIGINT/SIQUIT have not been received
-check serially all files in common_dir
-if a new .id file has appeared, and if this client has not already tried 3 times syncing with this client, start syncing
-after syncing (or trying to) with all clients, see which failed and then check if any synced-with client has been deleted (and do as needed)
-after waiting for 3 seconds, while catching signals and acting as needed for them, repeat
*/
client_id check_common_dir(client *cl){
  pipe2(pipefd, O_NONBLOCK);
  int p_read = dup(pipefd[0]);
  dup2(pipefd[1],0);

  fd_set fdset;
  FD_ZERO(&fdset);
  FD_SET(p_read, &fdset);
  struct timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;

  struct sigaction sigact = {0};
  sigact.sa_flags = SA_RESTART;
  sigact.sa_sigaction = usr_handler;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGQUIT, &sigact, NULL);
  sigaction(SIGUSR1, &sigact, NULL);
  sigaction(SIGALRM, &sigact, NULL);


  DIR *common_dir = opendir(cl->common_dir_path);
  if (common_dir == NULL){
    perror("Check Common Dir - Opendir()");
    close(pipefd[0]);
    close(pipefd[1]);
    close(0);

    return -1;
  }

  //print into log file that the client has started
  char *log = malloc( sizeof(char) * ( strlen("Client ") + strlen(cl->id) + strlen(": Connection successful.\n") + 1 ) );
  strcpy(log, "Client ");
  strcat(log, cl->id);
  strcat(log, ": Connection successful.\n");
  write(1, log, strlen(log));
  free(log);

  struct dirent *dirp = NULL;
  char *tokenized = NULL;
  sigset_t block_signals;
  sigfillset(&block_signals);

  while ( sync_flag != 1 ){       //while SIGINT/SIQUIT has not been received

    while ( (dirp = readdir(common_dir)) != NULL && sync_flag == 0 ){
      sigprocmask(SIG_SETMASK, &block_signals, NULL);

      if ( (strcmp(dirp->d_name, ".")) && (strcmp(dirp->d_name, ".."))){

        if ( dirp->d_name[strlen(dirp->d_name) - 1] == 'd' ){      //if it was not a .fifo file
          tokenized = malloc(sizeof(char) * ( strlen(dirp->d_name) + 1) );
          strcpy(tokenized, dirp->d_name);
          strtok(tokenized, ".id");
          if ( (strcmp(tokenized, cl->id)) ){
            if ( !(already_synced(cl, tokenized)) ) {     //if it is the first try to sync with this client
              client_id new_id = synch(tokenized, cl, 0);
              if ( new_id != -1 ){     //if sync returns -1, it means that we are in a copy of the process of the client and it needs to exit
                closedir(common_dir);
                free(tokenized);

                return new_id;
              }
            }
          }

          free(tokenized);
          tokenized = NULL;

        }

      }
      sigprocmask(SIG_UNBLOCK, &block_signals, NULL);
      sleep(1);     //sleep 1 second in order to receive signals before checking the flag

    }

    if ( sync_flag == 1 ){
      break;
    }

    //blocks signals while reading from self-pipe for pids that sent SIGUSR1
    sigprocmask(SIG_SETMASK, &block_signals, NULL);
    if ( sync_flag == 2 ){
      int sel;
      FD_ZERO(&fdset);
      FD_SET(p_read, &fdset);
      t.tv_sec = 1;
      t.tv_usec = 0;
      //when sel is equal to -1 something went wrong with the pipe, so try again
      while ( (sel = select(p_read + 1, &fdset, NULL, NULL, &t)) == -1 ){}   //wait 1 sec. If no signal has been sent, leave
      //when it is equal to 0, no one has written into the pipe
      if ( sel != 0 ){       //if the self-pipe contains nothing to read
        pid_t sent_usr1 = 0;
        while ( (read(p_read, &sent_usr1, sizeof(pid_t))) != -1 ){      //-1 means reached EOF
          client_id new_id = retry(cl, sent_usr1);
          if ( new_id != -1 ){        //going to exec()
            closedir(common_dir);

            return new_id;
          }
        }
      }
      sync_flag = 0;
    }

    //unblocks signals
    sigprocmask(SIG_UNBLOCK, &block_signals, NULL);

    alarm(3);
    while ( !sync_flag ){      //waits 3 seconds until it checks for new and deleted clients
      pause();                 //while still catching SIGUSR1/SIGINT/SIQUIT
    }
    if ( sync_flag == -1 ){
      sync_flag = 0;
    }
    else if ( sync_flag == 1 ){
      break;
    }

    int c = check_for_deleted_clients(cl);
    if ( c == -1 ){
      closedir(common_dir);

      return -2;
    }
    rewinddir(common_dir);

  }

  closedir(common_dir);
  close(0);
  close(p_read);
  close(pipefd[0]);
  close(pipefd[1]);
  return -1;
}

//checks whether a client that this client tried to sync with, still exists
int cl_still_exists(client *cl, char *id){
  DIR *common_dir = opendir(cl->common_dir_path);
  if ( common_dir == NULL ){
    perror("cl_still_exists - opendir()");
    return -1;
  }

  struct dirent *dirp = NULL;

  while ( (dirp = readdir(common_dir)) != NULL ){
    if ( dirp->d_name[strlen(dirp->d_name) - 1 ] == 'd' ){
      char *tokenized = malloc( sizeof(char) * ( strlen(dirp->d_name) + 1 ) );
      strcpy(tokenized, dirp->d_name);
      strtok(tokenized, ".id");
      if ( !(strcmp(tokenized, id)) ){
        return 1;
      }
    }
  }

  return 0;
}

client_id retry(client *cl, pid_t sent_usr1){

  int status;
  //I expect the other process to have ended here, so if it hangs, it is either an error of mine or I am so lucky that
  //a new process created in the while loop on line 76 took the same process id as a process that had already failed
  //I think I cannot have any control over such an occasion and it is of very low probability, however, of an existent one
  waitpid(sent_usr1, &status, 0 /* | WNOHANG */);
  client_id id_that_failed = WEXITSTATUS(status);
  char id[32];
  sprintf(id, "%d", id_that_failed);

  if ( cl_still_exists(cl, id) == 0 ){ 
    char *buffer = malloc( sizeof(char) * ( strlen("Synching client ") + strlen(cl->id) + strlen(" with client ") + strlen(id) + strlen(" failed, because it stopped.\n") + 1 ) );
    strcpy(buffer, "Synching client ");
    strcat(buffer, cl->id);
    strcat(buffer, " with client ");
    strcat(buffer, id);
    strcat(buffer, " failed, because it stopped.\n");
    write(1, buffer, strlen(buffer));
    free(buffer);
    delete_entry(cl, id);
    return -1; 
  }

  int tries = 0;
  if ( ( tries = times_tried(cl, id) ) <= 2 ){
    char *buffer = malloc( sizeof(char) * ( strlen("Synching client ") + strlen(cl->id) + strlen(" with client ") + strlen(id) + strlen(" failed. Going to retry.\n") + 1 ) );
    strcpy(buffer, "Synching client ");
    strcat(buffer, cl->id);
    strcat(buffer, " with client ");
    strcat(buffer, id);
    strcat(buffer, " failed. Going to retry.\n");
    write(1, buffer, strlen(buffer));
    free(buffer);
    return synch(id, cl, tries);
  }
  else{
    char *buffer = malloc( sizeof(char) * ( strlen("Client ") + strlen(cl->id) + strlen(" already tried 3 times to sync with client ") + strlen(id) + strlen(".\n") + 1 ) );
    strcpy(buffer, "Client ");
    strcat(buffer, cl->id);
    strcat(buffer, " already tried 3 times to sync with client ");
    strcat(buffer, id);
    strcat(buffer, ".\n");
    write(1, buffer, strlen(buffer));
    free(buffer);
    return -1;
  }
}

client_id synch(char *new_client_id, client *cl, int tries){

  tries++;
  char tries_string[2];
  sprintf(tries_string, "%d", tries);
  char *buffer = malloc( sizeof(char) * ( strlen("Try #") + strlen(tries_string) + strlen(" for client ") + strlen(cl->id) + strlen(" to sync with client ") + strlen(new_client_id) + strlen(".\n") + 1 ) );
  strcpy(buffer, "Try #");
  strcat(buffer, tries_string);
  strcat(buffer, " for client ");
  strcat(buffer, cl->id);
  strcat(buffer, " to sync with client ");
  strcat(buffer, new_client_id);
  strcat(buffer, ".\n");
  write(1, buffer, strlen(buffer));
  free(buffer);

  pid_t sync_proc = fork();
  if (sync_proc == -1){
    perror("Synch - fork()");
    return -1;
  }
  else if (sync_proc != 0){
    add_pid(cl, sync_proc, new_client_id);

    return -1;
  }
  else{
    client_id id = strtol(new_client_id, NULL, 10);

    return id;
   }
}

void add_pid(client *cl, pid_t pid, char *id){
  //if it is the first child process
  if (cl->first_child == NULL){
    cl->first_child = malloc(sizeof(children_list));
    cl->first_child->to_sync_with = malloc( sizeof(char) * ( strlen(id) + 1 ) );
    strcpy(cl->first_child->to_sync_with, id);
    cl->first_child->last_pid = pid;
    cl->first_child->times = 1;
    cl->first_child->next = NULL;
    cl->first_child->previous = NULL;

    return;
  }

  children_list *curr_child = cl->first_child;

  while ( curr_child->next != NULL ){
    if ( !(strcmp(curr_child->to_sync_with, id)) ){
      curr_child->times++;
      curr_child->last_pid = pid;

      return;
    }

    curr_child = curr_child->next;
  }
  /******* for the last child ***********/
  if ( !(strcmp(curr_child->to_sync_with, id)) ){
    curr_child->times++;
    curr_child->last_pid = pid;

    return;
  }
  /****************************************/

  curr_child->next = malloc(sizeof(children_list));
  curr_child->next->previous = curr_child;

  curr_child = curr_child->next;
  curr_child->times = 1;
  curr_child->last_pid = pid;
  curr_child->next = NULL;
  curr_child->to_sync_with = malloc( sizeof(char) * ( strlen(id) + 1 ) );
  strcpy(curr_child->to_sync_with, id);

  return;
}

int times_tried(client *cl, char *to_sync_with){
  if ( cl->first_child == NULL ) return 0;

  children_list *curr_child = cl->first_child;

  while ( curr_child->next != NULL ){
    if ( !(strcmp(curr_child->to_sync_with, to_sync_with) ) ){
      return curr_child->times;
    }

    curr_child = curr_child->next;
  }

  if ( !(strcmp(curr_child->to_sync_with, to_sync_with) ) ){
    return curr_child->times;
  }

  return 0;
}

int already_synced(client *cl, char *new_id){
  if (cl->first_child == NULL) return 0;

  children_list *curr_child = cl->first_child;

  while ( curr_child->next != NULL ){
    if ( !(strcmp(curr_child->to_sync_with, new_id)) ){
      return 1;
    }
  }
  if ( !(strcmp(curr_child->to_sync_with, new_id)) ){
    return 1;
  }

  return 0;
}

void remove_list_node(client *cl, children_list *curr_child){

  if (curr_child->previous != NULL ){
    curr_child->previous->next = curr_child->next;
    if (curr_child->next != NULL){
      curr_child->next->previous = curr_child->previous;
    }
  }
  else if (curr_child->next != NULL){   //this node having NULL as previous means that it is the first child
    curr_child->next->previous = NULL;
    cl->first_child = curr_child->next;
  }
  else{
    cl->first_child = NULL;
  }

  free(curr_child->to_sync_with);
  free(curr_child);

  return;
}

int check_for_deleted_clients(client *cl){
  DIR *common_dir = opendir(cl->common_dir_path);
  DIR *mirror_dir = opendir(cl->mirror_dir_path);

  if (common_dir == NULL){
    perror("Check for deleted clients - Opendir(common_dir)");
    return 1;
  }
  if (mirror_dir == NULL){
    perror("Check for deleted clients - Opendir(mirror_dir)");
    return 1;
  }

  struct dirent *dirent_common = NULL;
  struct dirent *dirent_mirror = NULL;
  int del_flag = 0;

  //detect all clients that have been synced and for every client,
  //check if there is a .id file with its id
  //if not, delete this client
  while ( (dirent_mirror = readdir(mirror_dir)) != NULL){

    if ( (strcmp(dirent_mirror->d_name, ".")) && (strcmp(dirent_mirror->d_name, "..")) ){

      while ( (dirent_common = readdir(common_dir)) != NULL){
        if ( (strcmp(dirent_common->d_name, ".")) && (strcmp(dirent_common->d_name, "..")) ){

          if ( dirent_common->d_name[strlen(dirent_common->d_name) - 1] == 'd' ){      //if it's not a .fifo file
            char *tokenized = malloc( sizeof(char) * ( strlen(dirent_common->d_name) + 1 ) );
            strcpy(tokenized, dirent_common->d_name);
            strtok(tokenized, ".id");

            if ( !(strcmp(tokenized, dirent_mirror->d_name)) ){    //if the client has not been deleted
              del_flag = 1;       //skip deletion
              free(tokenized);
              break;
            }
            free(tokenized);
            tokenized = NULL;
          }
        }
      }

      if (del_flag == 0){
        int d = delete_client(dirent_mirror->d_name, cl);
        if ( d == -1 ){
          closedir(mirror_dir);
          closedir(common_dir);
          return -1;
        }
      }
      else del_flag = 0;

      rewinddir(common_dir);    //rewind DIR stream for next client in mirror_dir
    }

  }

  closedir(mirror_dir);
  closedir(common_dir);

  return 0;
}

//delete previously synced client's folder
int delete_client(char *id_to_delete, client *cl){
  pid_t child = fork();

  if ( child == -1 ){
    perror("delete_client - fork()");
    return 1;
  }
  else if ( child != 0 ){
    delete_entry(cl, id_to_delete);
    return 0;
  }

  char *path_to_delete = append_to_rel_path(cl->mirror_dir_path, id_to_delete);

  remove_dir(path_to_delete);

  free(path_to_delete);

  return -1;
}

void free_client(client *cl){

  free(cl->mirror_dir_path);
  free(cl->input_dir_path);
  free(cl->common_dir_path);
  free(cl->log_file_path);

  if (cl->first_child != NULL){
    free_on_exec(cl);
  }

  return;
}

void delete_on_exit(client *cl){
  remove_dir(cl->mirror_dir_path);

  char *id_file_path = malloc(sizeof(char) * (strlen(cl->common_dir_path) + strlen(cl->id) + 5));
  strcpy(id_file_path, cl->common_dir_path);
  strcat(id_file_path, "/");
  strcat(id_file_path, cl->id);
  strcat(id_file_path, ".id");

  if ( (unlink(id_file_path)) ){
    if (errno != ENOENT){       //the function could be used when this file has not been created
      perror("delete_directories - unlink()");
    }
  }

  free(id_file_path);

  return;
}

//frees the children_list structure from the client
//it's not only used before an exec, but its initial purpose was this
void free_on_exec(client *cl){
  if (cl->first_child == NULL) return;
  children_list *curr_child = cl->first_child;

  while (curr_child->next != NULL){
    free(curr_child->to_sync_with);
    curr_child = curr_child->next;
    free(curr_child->previous);
  }

  free(curr_child->to_sync_with);
  free(curr_child);

  return;
}

void kill_processes(client *cl){
  if ( cl->first_child == NULL )  return;

  children_list *curr_child = cl->first_child;

  while ( curr_child->next != NULL ){
    kill(curr_child->last_pid, SIGUSR1);
    sleep(1);
  }

  kill(curr_child->last_pid, SIGUSR1);
  sleep(1);

  return;
}

void delete_entry(client *cl, char *id_to_delete){
  //expect the first child to be non-NULL when this function is called
  children_list *curr_child = cl->first_child;

  while ( curr_child->next != NULL ){
    if ( !(strcmp(id_to_delete, curr_child->to_sync_with)) ){
      remove_list_node(cl, curr_child);

      return;
    }
  }

  if ( !(strcmp(id_to_delete, curr_child->to_sync_with)) ){
    remove_list_node(cl, curr_child);

    return;
  }
}