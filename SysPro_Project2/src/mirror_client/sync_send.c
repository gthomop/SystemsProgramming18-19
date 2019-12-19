/******* sync_send.c *******/
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
#include <signal.h>
#include "dirControl.h"

int flag = 0;

void sig_h(int signo, siginfo_t *info, void *v){
    flag = 1;
    return;
}

char *signal_termination(char *id_to){
  char *buffer = malloc( sizeof(char) * ( strlen("To client ") + strlen(id_to) + strlen(": Failed due to a signal.\n") + 1) );
  strcpy(buffer, "To client ");
  strcat(buffer, id_to);
  strcat(buffer, ": Failed due to a signal.\n");

  return buffer;
}

char *pipe_termination(char *id_to){
  char *buffer = malloc( sizeof(char) * ( strlen("To client ") + strlen(id_to) + strlen(": Failed due to a problem with the pipe.\n") + 1) );
  strcpy(buffer, "To client ");
  strcat(buffer, id_to);
  strcat(buffer, ": Failed due to a problem with the pipe.\n");

  return buffer;
}

int sync_send_dir(char *dir_path, int pipe_fd, size_t max_buffsize, char *id_to);
int sync_send_file(char *path, int pipe_fd, size_t max_buffsize, char *id_to);

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
    char *input_dir_path = argv[4];
    size_t max_bytes = strtol(argv[5], NULL, 10);

    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);

    char *fifo_name = get_fifo_name(common_dir_path, id_from, id_to);


    if (flag == 1){
      free(fifo_name);
      return 1;
    }

    sigprocmask(SIG_SETMASK, &set, NULL);

    if ( (mkfifo(fifo_name, 0666)) == -1){
        if ( errno != EEXIST ){
            char *log = pipe_termination(id_to);
            write(1, log, strlen(log));
            free(log);
            free(fifo_name);
            close(1);

            return 1;
        }
    }

    sigprocmask(SIG_UNBLOCK, &set, NULL);

    int fifo_fd = open(fifo_name, O_WRONLY );
    if ( fifo_fd == -1 ){
        char *log = pipe_termination(id_to);
        write(1, log, strlen(log));
        free(log);
        free(fifo_name);
        
        close(1);
        return 1;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fifo_fd, &fdset);
    struct timeval t;
    t.tv_sec = 40;
    t.tv_usec = 0;

    sigprocmask(SIG_UNBLOCK, &set, NULL);
    sleep(1);

    if ( flag == 1 ){
      char *log = signal_termination(id_to);
      write(1, log, strlen(log));
      free(log);
      close(fifo_fd);
      unlink(fifo_name);
      free(fifo_name);

      close(1);

      return 1;
    }
    //the process blocks here for 40 seconds until the read end has opened or a signal is sent
    int sel = select(fifo_fd + 1, NULL, &fdset, NULL, &t);

    if ( sel == -1 || sel == 0 ){
      close(fifo_fd);
      unlink(fifo_name);
      char *log = NULL;
      if ( sel == -1 ){
          log = signal_termination(id_to);
      }
      else{
          log = pipe_termination(id_to);
      }
      write(1, log, strlen(log));
      free(log);
      close(1);
      unlink(fifo_name);
      free(fifo_name);

      return 1;
    }

    DIR *input_dir = opendir(input_dir_path);
    if ( input_dir == NULL ){
        close(fifo_fd);
        unlink(fifo_name);
        free(fifo_name);
        close(1);

        return 1;
    }

    chdir(input_dir_path);

    struct dirent *dirent_input = NULL;
    char *log = NULL;

    if ( flag == 1 ){
        close(fifo_fd);
        closedir(input_dir);
        log = signal_termination(id_to);
        write(1, log, strlen(log));
        free(log);
        close(1);
        unlink(fifo_name);
        free(fifo_name);

        return 1;
    }

    log = malloc( sizeof(char) * ( strlen("To client ") + strlen(id_to) + strlen(": Initiating sending of files.\n") + 1 ) );
    strcpy(log, "To client ");
    strcat(log, id_to);
    strcat(log, ": Initiating sending of files.\n");
    write(1, log, strlen(log));
    free(log);

    while ( (dirent_input = readdir(input_dir)) != NULL ){
        sigprocmask(SIG_SETMASK, &set, NULL);
        if ( (strcmp(dirent_input->d_name, ".")) && (strcmp(dirent_input->d_name, "..")) ) {

            //send directory of file to pipe

            if ( (is_dir(dirent_input->d_name)) ){
                if ( (sync_send_dir(dirent_input->d_name, fifo_fd, max_bytes, id_to)) == 1 ){
                    close(fifo_fd);
                    closedir(input_dir);
                    log = pipe_termination(id_to);
                    write(1, log, strlen(log));
                    free(log);
                    close(1);
                    unlink(fifo_name);
                    free(fifo_name);

                    return 1;           //error occured
                }
            }
            else{
                if ( (sync_send_file(dirent_input->d_name, fifo_fd, max_bytes, id_to)) == 1 ){
                    close(fifo_fd);
                    closedir(input_dir);
                    log = pipe_termination(id_to);
                    write(1, log, strlen(log));
                    free(log);
                    close(1);
                    unlink(fifo_name);
                    free(fifo_name);

                    return 1;
                }
            }
        }
        sigprocmask(SIG_UNBLOCK, &set, NULL);
        sleep(1);

        if ( flag == 1 ){
            close(fifo_fd);
            closedir(input_dir);
            log = signal_termination(id_to);
            write(1, log, strlen(log));
            free(log);
            close(1);
            unlink(fifo_name);
            free(fifo_name);

            return 1;
        }
    }

    closedir(input_dir);
    //write 2 zero bytes that signal the reading end to stop reading
    short unsigned end = 0;
    if ( (write(fifo_fd, &end, 2)) == -1 ){
        close(fifo_fd);
        log = pipe_termination(id_to);
        write(1, log, strlen(log));
        free(log);
        close(1);
        unlink(fifo_name);
        free(fifo_name);

        return 1;
    }

    close(fifo_fd);

    /*
        unlink deletes the name of the fifo from the filesystem
        however, the space it was using will only be available to the filesystem
        only when the last file descriptor pointing to the fifo is closed
        this means that the process reading from the fifo will not stop reading.
        When it stops, the fifo will be completely deleted from the filesystem.
    */
    unlink(fifo_name);
    free(fifo_name);

    log = malloc( sizeof(char) * ( strlen("To client ") + strlen(id_to) + strlen(": File sending ends successfully.\n") + 1 ) );
    strcpy(log, "To client ");
    strcat(log, id_to);
    strcat(log, ": File sending ends successfully.\n");
    write(1, log, strlen(log));
    free(log);
    close(1);

    return 0;
}

int sync_send_dir(char *dir_path, int pipe_fd, size_t max_buffsize, char *id_to){
    DIR *dir = opendir(dir_path);

    if (dir == NULL){
        return 1;
    }

    struct dirent *dirp = NULL;
    //the absolute path of a file in the dir
    char *path_buf = NULL;
    int file_found = 0;

    while( (dirp = readdir(dir)) != NULL){
        if ( !(strcmp(dirp->d_name, ".")) || !(strcmp(dirp->d_name, ".."))){
            continue;
        }
        file_found = 1;

        path_buf = append_to_rel_path(dir_path, dirp->d_name);

        //if file is a directory
        if ( (is_dir(path_buf)) ){
            if ( (sync_send_dir(path_buf, pipe_fd, max_buffsize, id_to)) ){
                free(path_buf);
                closedir(dir);
                return 1;
            }
        }
        else{
            if ( (sync_send_file(path_buf, pipe_fd, max_buffsize, id_to)) ){
                free(path_buf);
                closedir(dir);
                return 1;
            }
        }

        free(path_buf);
        path_buf = NULL;
    }

    if ( !(file_found) ){
        path_buf = append_to_rel_path(dir_path, "");            //slash at the end of a filename(path) tells the reading end that this path is of an empty directory
        if ( (sync_send_file(path_buf, pipe_fd, max_buffsize, id_to)) ){
            free(path_buf);
            closedir(dir);
            return 1;
        }
        free(path_buf);
    }

    closedir(dir);

    return 0;
}

int sync_send_file(char *path, int pipe_fd, size_t max_buffsize, char *id_to){

    char *buffer = malloc( sizeof(char) * ( max_buffsize + 1 ) );

    short unsigned length = (short unsigned)strlen(path);

    /*********** write 2 bytes representing the size of path ********/

    if ( write(pipe_fd, &length, 2) == -1 ){
        free(buffer);
        return 1;
    }

    /****************************************************************/

    /********************** write file path ************************/
    int chars_left = (int)length;
    int times = 0;

    while (chars_left >= max_buffsize){
        for (size_t i = 0; i < max_buffsize; i++){
            buffer[i] = path[i + max_buffsize * times];
        }

        if ( (write(pipe_fd, buffer, max_buffsize)) == -1 ){
            free(buffer);
            return 1;
        }

        times++;
        chars_left -= max_buffsize;
    }

    for (size_t i = 0; i < chars_left; i++){
        buffer[i] = path[i + max_buffsize * times];
    }

    if ( (write(pipe_fd, buffer, chars_left)) == -1 ){
        free(buffer);
        return 1;
    }
    /***************************************************************/

    /******************** write size of file ***********************/
    unsigned file_size = 0;

    if ( (is_dir(path)) ){
        if ( (write(pipe_fd, &file_size, 4)) == -1 ){
            free(buffer);
            return 1;
        }
    }
    else{
        struct stat stat_buf = {0};

        stat(path, &stat_buf);

        file_size = (unsigned)stat_buf.st_size;

        if ( (write(pipe_fd, &file_size, 4)) == -1 ){
            free(buffer);
            return 1;
        }


    /***************************************************************/

    /******************* write contents of file ********************/
        if ( file_size != 0 ){
            unsigned remaining = file_size;
            int file = open(path, O_RDONLY);
            while ( remaining >= max_buffsize ){
                if ( (read(file, buffer, max_buffsize)) == -1 ){
                    free(buffer);
                    close(file);
                    return 1;
                }
                if ( (write(pipe_fd, buffer, max_buffsize)) == -1 ){
                    free(buffer);
                    close(file);
                    return 1;
                }

                remaining -= max_buffsize;
            }

            if ( (read(file, buffer, remaining)) == -1 ){
                free(buffer);
                close(file);
                return 1;
            }
            if ( (write (pipe_fd, buffer, remaining)) == -1 ){
                free(buffer);
                close(file);
                return 1;
            }
            close(file);
        }


    }

    char size[32];
    sprintf(size, "%u", file_size);
    char *log = malloc( sizeof(char) * ( strlen("To client ") + strlen(id_to) + strlen(": ") + strlen(path) + strlen(" - ") + strlen(size) + strlen(" bytes\n") + 1 ) );
    strcpy(log, "To client ");
    strcat(log, id_to);
    strcat(log, ": ");
    strcat(log, path);
    strcat(log, " - ");
    strcat(log, size);
    strcat(log, " bytes\n");
    write(1, log, strlen(log));
    free(buffer);

    return 0;
}
