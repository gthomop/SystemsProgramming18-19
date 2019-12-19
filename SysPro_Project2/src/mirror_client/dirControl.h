/******* dirControl.h *******/
#ifndef DIRCONTROL_H
#define DIRCONTROL_H

//to define macros such as S_IFMT and S_IFDIR
#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>


int remove_dir(char *dir_path);
int is_dir(char *path);
char *get_abs_path(char *cwd, char *path_to_append);
char *get_fifo_name(char *common_dir_path, char *id1, char *id2);
char *append_to_rel_path(char *dir, char *file);

#endif
