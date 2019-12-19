/**** file_structs.h ****/
#ifndef _FILE_STRUCTS_H
#define _FILE_STRUCTS_H

typedef struct file_tuple {
    char pathname[129];
    char version[33];
} file_tuple;

typedef struct file_list file_list;
struct file_list {
    file_tuple *this_tuple;
    file_list *next;
    file_list *previous;
};

#endif