/**** buffer.h ****/
#ifndef _BUFFER_H
#define _BUFFER_H

#include "file_structs.h"
#include "client_list.h"

#include <stdlib.h>
#include <string.h>

typedef struct element_t element_t;
typedef struct buffer_t buffer_t;

struct element_t{
    file_tuple f_tuple;
    client_tuple c_tuple;
};

struct buffer_t{
    element_t *circ_buffer;
    size_t size;
    size_t read;
    size_t write;
};

void buffer_init(buffer_t *buff, size_t bufferSize);
short buffer_is_empty(buffer_t *buff);
short buffer_is_full(buffer_t *buff);
void buffer_push(buffer_t *buff, element_t *elem);
element_t *buffer_pop(buffer_t *buff);

#endif