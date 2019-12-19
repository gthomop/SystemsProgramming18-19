/******* main.c ******/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "use.h"
#include "dirControl.h"
#include "clientSync.h"

int flag = 0;

void main_handler(int sig);
void freepaths(char *path1, char *path2, char *path3, char *path4);

int main(int argc, char **argv){

    signal(SIGINT, main_handler);
    signal(SIGQUIT, main_handler);

    CmdParams arguments = use(argc,argv);

	if (!arguments.rightUsage){
        freeCmdParams(&arguments);
	    return 1;	//wrong arguments, terminate program
    }

    client cl = {0};
    cl.first_child = NULL;
	//grab data input from Cmd Line
    strcpy(cl.id, arguments.params[0]);

    //id == 0 is not acceptable, because a child process that fails returns the ID of the client
    //with which the child process tried to synchronize, or 0 on success
    if ( (strtol(arguments.params[0], NULL, 10)) == 0 ){
        fprintf(stderr, "A client cannot have id equal to zero\n");
        freeCmdParams(&arguments);
        return 1;
    }

    //realpath returns the absolute path given a relative path
    //the second argument is NULL which means that it mallocs a string in order to store the absolute path

    /****** check if common dir exists and, if needed, create it ******************/
    if ( (cl.common_dir_path = realpath(arguments.params[1], NULL)) == NULL){       //if common_dir does not exist, create it
        mkdir(arguments.params[1], S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        cl.common_dir_path = realpath(arguments.params[1], NULL);
    }
    /******************************************************************************/

    /********** check if input dir exists and, if not, exit with error ************/
    if ( (cl.input_dir_path = realpath(arguments.params[2], NULL)) == NULL){        //if input_dir does not exist, exit
        perror("input_dir_path - realpath()");
        free_client(&cl);
        freeCmdParams(&arguments);

        return 1;
    }
    /******************************************************************************/

    /****** check if mirror dir exists and, if it does, exit with error ***********/
    if ( (cl.mirror_dir_path = realpath(arguments.params[3], NULL)) != NULL){       //if mirror_dir exists, exit
        fprintf(stderr, "Mirror directory (%s) already exists.\n", arguments.params[3]);
        free_client(&cl);
        freeCmdParams(&arguments);

        return 1;
    }
    /******************************************************************************/

    /****************************** create mirror dir *****************************/
    mkdir(arguments.params[3], S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    cl.mirror_dir_path = realpath(arguments.params[3], NULL);
    /******************************************************************************/

    /***************************** save maximum buffer size ***********************/
    char max_buffsize[32];
    strcpy(max_buffsize, arguments.params[4]);
    /******************************************************************************/

    /************* create log file if it doesn't already exist ********************/
    int log_file = 0;

    if ( (log_file = open(arguments.params[5], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ) == -1 ){
        perror("Creating_log_file - open()");
        free_client(&cl);
        freeCmdParams(&arguments);
        delete_on_exit(&cl);

        return 1;
    }

    cl.log_file_path = realpath(arguments.params[5], NULL);

    dup2(log_file, 1);      //for ease of use
    close(log_file);
    /******************************************************************************/

    freeCmdParams(&arguments);

    int id_file_fd = 0;

    //allocate a string that fits exactly the ID of the client, the file extension ".id" and the null byte
    char *tmp_path = malloc( sizeof(char) * ( strlen(cl.id) + 4 ) );
    strcpy(tmp_path, cl.id);
    strcat(tmp_path, ".id");

    char *id_file_path = append_to_rel_path(cl.common_dir_path, tmp_path);
    free(tmp_path);

    struct stat buf = {0};
    if ( !(stat(id_file_path, &buf)) ){          //if the .id file already exists
        fprintf(stderr, "A client with the same ID is already running.\n");        
        free(id_file_path);
        free_client(&cl);
        delete_on_exit(&cl);
        close(1);

        return 1;
    }

    //create the file and open it int write-only mode
    id_file_fd = open(id_file_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    //write the process id to the file
    pid_t pid = getpid();
    if ( (write(id_file_fd, &pid, sizeof(pid_t))) == -1){
        perror("if_file_fd - write()");
        free(id_file_path);
        free_client(&cl);
        close(id_file_fd);
        delete_on_exit(&cl);
        close(1);

      return 1;
    }
    free(id_file_path);
    close(id_file_fd);

    if (flag == 1){
        free_client(&cl);
        delete_on_exit(&cl);
        close(1);

        return 1;
    }

    client_id new_id = 0;
    if ( (new_id = check_common_dir(&cl)) > 0 ){      //returns -1 on success, else it is the id of the client to sync_with
        char id_of_this_client[32];
        char id_of_new_client[32];
        strcpy(id_of_this_client, cl.id);
        sprintf(id_of_new_client, "%d", new_id);
        free_on_exec(&cl);
        free(cl.log_file_path);

        execl("./sync", "sync", id_of_this_client, id_of_new_client, cl.common_dir_path, cl.input_dir_path, cl.mirror_dir_path, max_buffsize, NULL);

        //these commands will be executed only if execl() fails
        kill(getppid(), SIGUSR1);

        return 1;
    }
    else if ( new_id == -2 ){            //this is the child responsible for deleting a directory from the mirror directory
        free_client(&cl);
        close(1);

        return 0;
    }

    kill_processes(&cl);
    delete_on_exit(&cl);
    free_client(&cl);
    char *buffer = malloc( sizeof(char) * ( strlen("Exiting client\n") + 1 ) );
    strcpy(buffer, "Exiting client\n");
    write(1, buffer, strlen(buffer));
    free(buffer);
    close(1);

    return 0;
}

void main_handler(int sig){
    if (sig == SIGINT || sig == SIGQUIT){
        flag = 1;
    }
}
