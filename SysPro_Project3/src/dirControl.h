/******* dirControl.h *******/
#ifndef DIRCONTROL_H
#define DIRCONTROL_H

#include "file_structs.h"

//to define macros such as S_IFMT and S_IFDIR
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int get_file_list(char *dir_path, file_list **list, file_list **last, int *num_of_files, short first_rec);
int is_dir(char *path);
char *find_version(int fd);
char *get_version(char *fileName);

#endif
