/********** sync_receive.c *********/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/select.h>
#include <signal.h>
#include "dirControl.h"

int flag = 0;

void sig_h(int signo, siginfo_t *info, void *v){
  flag = 1;
  return;
}

char *signal_termination(char *id_to){
  char *buffer = malloc( sizeof(char) * ( strlen("From client ") + strlen(id_to) + strlen(": Failed due to a signal.\n") + 1) );
  strcpy(buffer, "From client ");
  strcat(buffer, id_to);
  strcat(buffer, ": Failed due to a signal.\n");

  return buffer;
}

char *pipe_termination(char *id_to){
  char *buffer = malloc( sizeof(char) * ( strlen("From client ") + strlen(id_to) + strlen(": Failed due to a problem with the pipe.\n") + 1) );
  strcpy(buffer, "From client ");
  strcat(buffer, id_to);
  strcat(buffer, ": Failed due to a problem with the pipe.\n");

  return buffer;
}

int read_file_path(int pipe_fd, short unsigned length, size_t max_buffsize, char **path);
int read_file(int pipe_fd, int file_fd, unsigned file_size, size_t max_buffsize);

int main(int argc, char **argv){
    close(0);     //inherited from parent process<-client process
    struct sigaction sigact = {0};
    sigact.sa_flags = SA_RESTART;
    sigact.sa_sigaction = sig_h;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGUSR1, &sigact, NULL);

    char *id_from = argv[1];
    char *id_to = argv[2];
    char *common_dir_path = argv[3];
    char *mirror_dir_path = argv[4];
    size_t max_bytes = strtol(argv[5], NULL, 10);

    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);

    char *fifo_name = get_fifo_name(common_dir_path, id_to, id_from);

    if ( (mkfifo(fifo_name, 0666)) == -1){
        if ( errno != EEXIST){
          char *log = pipe_termination(id_to);
          write(1, log, strlen(log));
          free(log);
          free(fifo_name);
          close(1);

          return 1;
        }
    }

    sigprocmask(SIG_UNBLOCK, &set, NULL);

    int fifo_fd = open(fifo_name, O_RDONLY );

    free(fifo_name);    //is not needed any more

    DIR *mirror_dir = opendir(mirror_dir_path);

    if ( mirror_dir == NULL ){
        close(fifo_fd);
        close(1);

        return 1;
    }

    chdir(mirror_dir_path);
    mkdir(id_to, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    chdir(id_to);

    if ( flag == 1 ){
      char *problem = signal_termination(id_to);
      write(1, problem, strlen(problem));
      free(problem);
      close(fifo_fd);
      closedir(mirror_dir);

      close(1);

      return 1;
    }

    struct timeval t;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(fifo_fd, &fdset);

    t.tv_sec = 30;
    t.tv_usec = 0;

    //wait 30 seconds for input in the pipe, else exit with failure
    int sel = select(fifo_fd + 1, &fdset, NULL, NULL, &t);

    if ( sel <= 0 ){
      char *problem = NULL;
      if (sel == -1){
        problem = signal_termination(id_to);
      }
      else{
        problem = pipe_termination(id_to);
      }
      write(1, problem, strlen(problem));
      free(problem);
      close(fifo_fd);
      closedir(mirror_dir);

      close(1);

      return 1;
    }

    if ( flag == 1 ){
      char *problem = signal_termination(id_to);
      write(1, problem, strlen(problem));
      free(problem);
      close(fifo_fd);
      closedir(mirror_dir);

      close(1);

      return 1;
    }

    short unsigned path_length = 0;
    unsigned file_size = 0;
    char *file_path = NULL;
    char *log = NULL;

    read(fifo_fd, &path_length, 2);

    sigprocmask(SIG_SETMASK, &set, NULL);
    //write to log file
    log = malloc(sizeof(char) * ( strlen("From client ") + strlen(id_to) + strlen(": Initiating copy of files.\n") + 1 ) );

    strcpy(log, "From client ");
    strcat(log, id_to);
    strcat(log, ": Initiating copy of files.\n");
    write(1, log, strlen(log));
    free(log);
    log = NULL;

    if ( path_length == 0 ){    //in case the other client doesn't have any files or directories

      log = malloc( sizeof(char) * ( strlen("From client ") + strlen(id_to) + strlen(": No files. File copy ends successfully.\n") + 1 ) );
      strcpy(log, "From client ");
      strcat(log, id_to);
      strcat(log, ": No files. File copy ends successfully.\n");
      write(1, log, strlen(log));
      free(log);

      close(fifo_fd);
      closedir(mirror_dir);

      close(1);

      return 0;
    }

    sigprocmask(SIG_UNBLOCK, &set, NULL);

    if (flag == 1){
      log = signal_termination(id_to);
      write(1, log, strlen(log));
      free(log);
      close(fifo_fd);
      closedir(mirror_dir);

      close(1);

      return 1;
    }

    errno = 0;

    //block signals while reading a file, because there are many commands that are not reentrant inside
    do{
        sigprocmask(SIG_SETMASK, &set, NULL);

        int file = 0;
        if ( ( file = read_file_path(fifo_fd, path_length, max_bytes, &file_path) ) < 0 ){
            log = pipe_termination(id_to);
            write(1, log, strlen(log));
            free(log);
            free(file_path);
            close(fifo_fd);
            closedir(mirror_dir);

            close(1);

            return 1;
        }

        if ( (read(fifo_fd, &file_size, 4)) == -1 ){
            log = pipe_termination(id_to);
            write(1, log, strlen(log));
            free(log);
            free(file_path);
            close(fifo_fd);
            closedir(mirror_dir);

            close(1);

            return 1;
        }

        if ( (read_file(fifo_fd, file, file_size, max_bytes)) ){
            log = pipe_termination(id_to);
            write(1, log, strlen(log));
            free(log);
            free(file_path);
            close(fifo_fd);
            closedir(mirror_dir);

            close(1);

            return 1;
        }

        //write to log file
        log = malloc(sizeof(char) * ( strlen("From client ") + strlen(id_to) + strlen(": ") + strlen(file_path) + strlen(" - ") + 32  + strlen(" bytes\n") + 1 ) );
        char bytes_read[32];
        sprintf(bytes_read, "%u", file_size);
        strcpy(log, "From client ");
        strcat(log, id_to);
        strcat(log, ": ");
        strcat(log, file_path);
        strcat(log, " - ");
        strcat(log, bytes_read);
        strcat(log, " bytes\n");
        write(1, log, strlen(log));
        free(log);
        log = NULL;
        free(file_path);

        sigprocmask(SIG_UNBLOCK, &set, NULL);
        sleep(1);

        if (flag == 1){
          log = signal_termination(id_to);
          write(1, log, strlen(log));
          free(log);
          free(fifo_name);
          free(file_path);
          close(fifo_fd);
          closedir(mirror_dir);

          close(1);

          return 1;
        }

        read(fifo_fd, &path_length, 2);
    } while (  path_length != 0 );

    closedir(mirror_dir);
    close(fifo_fd);

    log = malloc( sizeof(char) * ( strlen("From client ") + strlen(id_to) + strlen(": File copy ends sucessfully.\n") + 1 ) );
    strcpy(log, "From client ");
    strcat(log, id_to);
    strcat(log, ": File copy ends successfully.\n");
    write(1, log, strlen(log));
    free(log);

    close(1);

    return 0;     //synchronization ends successfully
}

//reads the path of the file and creates it
int read_file_path(int pipe_fd, short unsigned length, size_t max_buffsize, char **path){

    sigset_t set;
    sigfillset(&set);

    short unsigned remaining = length;
    char *buffer = malloc( sizeof(char) * ( max_buffsize + 1 ) );
    *path = malloc( sizeof(char) * ( length + 1 ) );

    size_t times = 0;

    while ( remaining >= max_buffsize ){
        if ( (read(pipe_fd, buffer, max_buffsize)) == -1){
            free(*path);
            free(buffer);
            return -1;
        }

        for (size_t i = 0; i < max_buffsize; i++){
            (*path)[i + max_buffsize * times] = buffer[i];
        }

        remaining -= max_buffsize;
        times++;
    }

    if ( (read(pipe_fd, buffer, remaining)) == -1 ){
        free(*path);
        free(buffer);
        return -1;
    }

    for (size_t i = 0; i < remaining; i++){
        (*path)[i + max_buffsize * times] = buffer[i];
    }
    (*path)[length] = 0;

    char *tokenized = malloc( sizeof(char) * ( length + 1 ) );
    strcpy(tokenized, *path);
    char *token = strtok(tokenized, "/");

    char *tree = malloc( sizeof(char) * ( length + 1 ) );


    if ( (strcmp(token, *path)) ){
        strcpy(tree, token);
        mkdir(tree, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

        while ( (token = strtok(NULL, "/")) != NULL ){
            strcat(tree, "/");
            strcat(tree, token);
            if ( !(strcmp(*path, tree)) )    break;
            mkdir(tree, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        }
    }

    free(tokenized);
    free(tree);
    if ( (*path)[strlen(*path) - 1] != '/'){
      int file = open(*path, O_CREAT | O_RDWR, 00666);

      return file;
    }
    else return 0;
}

//reads the file data and copies everything
int read_file(int pipe_fd, int file_fd, unsigned file_size, size_t max_buffsize){
    if ( file_size == 0 ){
      return 0;
    }
    unsigned remaining = file_size;
    char *buffer = malloc( sizeof(char) * (max_buffsize + 1) );
    size_t times = 0;

    while ( remaining >= max_buffsize ){
        if ( (read(pipe_fd, buffer, max_buffsize)) == -1 ){
            free(buffer);
            return 1;
        }
        write(file_fd, buffer, max_buffsize);

        times++;
        remaining -= max_buffsize;
    }

    if ( (read(pipe_fd, buffer, remaining)) == -1 ){
        free(buffer);
        return 1;
    }
    write(file_fd, buffer, remaining);

    close(file_fd);

    return 0;
}
