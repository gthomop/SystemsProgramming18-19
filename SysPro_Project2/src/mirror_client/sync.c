/***** sync.c *******/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "dirControl.h"

int flag = 0;

void sig_h(int signo){
    signal(signo, sig_h);
    if (signo == SIGINT || signo == SIGQUIT){       //if an outsider factor wants to end the process
        flag = 1;
    }
    else if (signo == SIGUSR1){         //whenever a SIGUSR1 is received, it means that the client is trying to quit
        flag = 2;                       //so there is no need to send the other client's id and a signal
    }
}

int main(int argc, char **argv){
    signal(SIGINT, sig_h);
    signal(SIGQUIT, sig_h);
    signal(SIGUSR1, sig_h);

    char *this_id = argv[1];
    char *new_id = argv[2];
    char *common_dir_path = argv[3];
    char *input_dir_path = argv[4];
    char *mirror_dir_path = argv[5];
    char *max_buffsize = argv[6];

    sigset_t set;
    sigfillset(&set);

    if ( flag ){
        sigprocmask(SIG_SETMASK, &set, NULL);
        if ( flag == 1 ){
            int id_new = (int)strtol(new_id, NULL, 10);
            pid_t thispid = getpid();
            write(0, &thispid, sizeof(pid_t));
            kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
            close(0);

            return id_new;
        }
        else{
            close(0);

            return 0;
        }        
    }

    pid_t receive_pid = 0;
    pid_t send_pid = fork();

    if ( send_pid == -1 ){      //fork error
        int id_new = (int)strtol(new_id, NULL, 10);
        pid_t thispid = getpid();
        write(0, &thispid, sizeof(pid_t));
        kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
        close(0);

        return id_new;
    }
    else if ( send_pid == 0 ){      //in child process
        sigprocmask(SIG_UNBLOCK, &set, NULL);
        execl("./sync_send", "sync_send", this_id, new_id, common_dir_path, input_dir_path, max_buffsize, NULL);

        close(0);

        return -1;
    }
    else{       //in parent process
        sigprocmask(SIG_SETMASK, &set, NULL);

        receive_pid = fork();
        if ( receive_pid == -1 ){       //fork error
            int id_new = (int)strtol(new_id, NULL, 10);
            kill(send_pid, SIGUSR1);        //send signal to sender child process to make it end with error
            pid_t thispid = getpid();
            write(0, &thispid, sizeof(pid_t));
            kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
            close(0);

            return id_new;
        }
        else if ( receive_pid == 0 ){       //in child process
            sigprocmask(SIG_UNBLOCK, &set, NULL);
            execl("./sync_receive", "sync_receive", this_id, new_id, common_dir_path, mirror_dir_path,  max_buffsize, NULL);

            close(0);

            return -1;
        }
        else{       //in parent process
            sigprocmask(SIG_UNBLOCK, &set, NULL);
            sleep(1);       //sleep 1 second to be sure that if any signal was received, the flag will be 1
            if ( flag ){
                sigprocmask(SIG_SETMASK, &set, NULL);
                kill(send_pid, SIGUSR1);
                kill(receive_pid, SIGUSR1);
                sleep(1);   //wait 1 second until the 2 child processes end as they should

                if ( flag == 1 ){
                    int id_new = (int)strtol(new_id, NULL, 10);
                    char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                    remove_dir(tmp_path);
                    free(tmp_path);
                    pid_t thispid = getpid();
                    write(0, &thispid, sizeof(pid_t));
                    kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                    close(0);

                    return id_new;
                }
                else{
                    close(0);

                    return 0;
                }
            }

            int send_status;
            int receive_status;
            int w_ret1 = 0;
            int w_ret2 = 0;

            do{         //check whether any one of the 2 child processes has ended and returned
                send_status = 0;
                receive_status = 0;
                w_ret1 = waitpid(receive_pid, &receive_status, WNOHANG);
                w_ret2 = waitpid(send_pid, &send_status, WNOHANG);

                if ( flag ){
                    sigprocmask(SIG_SETMASK, &set, NULL);
                    kill(send_pid, SIGUSR1);
                    kill(receive_pid, SIGUSR1);

                    if ( flag == 1 ){
                        int id_new = (int)strtol(new_id, NULL, 10);
                        char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                        remove_dir(tmp_path);
                        free(tmp_path);
                        pid_t thispid = getpid();
                        write(0, &thispid, sizeof(pid_t));
                        kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                        close(0);

                        return id_new;
                    }
                    else{
                        close(0);

                        return 0;
                    }
                }

            } while ( w_ret1 <= 0 && w_ret2 <= 0 );

            //send a signal to the process that has not terminated and end it before returning
            //to the parent(client) the id of the client that this process was trying to sync with
            if ( w_ret1 > 0 ){        //if the receiver process ended
                if ( WEXITSTATUS(receive_status) != 0 ){        //receiver ended with error
                    char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                    remove_dir(tmp_path);
                    free(tmp_path);
                    kill(send_pid, SIGUSR1);        //send signal to end sending child process
                    sleep(1);
                    int id_new = (int)strtol(new_id, NULL, 10);
                    pid_t thispid = getpid();
                    write(0, &thispid, sizeof(pid_t));
                    kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                    close(0);

                    return id_new;
                }
                else{       //without error
                    if ( flag ){
                        sigprocmask(SIG_SETMASK, &set, NULL);
                        kill(send_pid, SIGUSR1);
                        sleep(1);

                        if ( flag == 1 ){
                            int id_new = (int)strtol(new_id, NULL, 10);
                            char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                            remove_dir(tmp_path);
                            free(tmp_path);
                            pid_t thispid = getpid();
                            write(0, &thispid, sizeof(pid_t));
                            kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                            close(0);

                            return id_new;
                        }
                        else{
                            close(0);

                            return 0;
                        }
                    }

                    w_ret2 = waitpid(send_pid, &send_status, 0);         //block until the other process ends
                    if ( WEXITSTATUS(send_status) != 0 ){       //sender process ended with error
                        char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                        remove_dir(tmp_path);
                        free(tmp_path);
                        int id_new = (int)strtol(new_id, NULL, 10);
                        pid_t thispid = getpid();
                        write(0, &thispid, sizeof(pid_t));
                        kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                        close(0);

                        return id_new;
                    }
                }
            }
            else if ( w_ret2 > 0 ){                 //if sender child process ended but the receiver process did not
                if ( WEXITSTATUS(send_status) != 0 ){        //with error
                    kill(receive_pid, SIGUSR1);        //send signal to end receiver child process
                    sleep(1);
                    char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                    remove_dir(tmp_path);
                    free(tmp_path);
                    int id_new = (int)strtol(new_id, NULL, 10);
                    pid_t thispid = getpid();
                    write(0, &thispid, sizeof(pid_t));
                    kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                    close(0);

                    return id_new;
                }
                else{       //without error
                    if ( flag ){
                        sigprocmask(SIG_SETMASK, &set, NULL);
                        kill(receive_pid, SIGUSR1);
                        sleep(1);

                        if ( flag == 1 ){
                            int id_new = (int)strtol(new_id, NULL, 10);
                            char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                            remove_dir(tmp_path);
                            free(tmp_path);
                            pid_t thispid = getpid();
                            write(0, &thispid, sizeof(pid_t));
                            kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                            close(0);

                            return id_new;
                        }
                        else{
                            close (0);

                            return 0;
                        }
                    }

                    w_ret2 = waitpid(receive_pid, &receive_status, 0);  //block until the other process ends
                    if ( WEXITSTATUS(receive_status) != 0 ){
                        char *tmp_path = append_to_rel_path(mirror_dir_path, new_id);
                        remove_dir(tmp_path);
                        free(tmp_path);
                        int id_new = (int)strtol(new_id, NULL, 10);
                        pid_t thispid = getpid();
                        write(0, &thispid, sizeof(pid_t));
                        kill(getppid(), SIGUSR1);       //send signal to parent process to indicate error
                        close(0);

                        return id_new;
                    }
                }
            }

            close(0);

            //if everything went right, return 0 to parent
            return 0;
        }
    }
}
