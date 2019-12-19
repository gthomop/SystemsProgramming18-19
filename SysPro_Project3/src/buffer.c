/**** buffer.c ****/
#include "buffer.h"

void buffer_init(buffer_t *buff, size_t bufferSize) {
    buff->circ_buffer = malloc(sizeof(element_t) * bufferSize);
    buff->read = 0;
    buff->write = 0;
    buff->size = bufferSize;

    return;
}

short buffer_is_empty(buffer_t *buff) {
    if (buff->read == buff->write) {
        return 1;
    }

    return 0;
}

short buffer_is_full(buffer_t *buff) {
    size_t next_write = buff->write + 1;
    if (next_write >= buff->size) {
        next_write = 0;
    }
    if (buff->read == next_write) {
        return 1;
    }

    return 0;
}

void buffer_push(buffer_t *buff, element_t *elem) {
    memcpy(&(buff->circ_buffer)[buff->write], elem, sizeof(element_t));
    buff->write++;
    if (buff->write >= buff->size) {
        buff->write = 0;
    }

    return;
}

element_t *buffer_pop(buffer_t *buff) {
    element_t *ret = &buff->circ_buffer[buff->read];

    buff->read++;
    if (buff->read >= buff->size) {
        buff->read = 0;
    }

    return ret;
}