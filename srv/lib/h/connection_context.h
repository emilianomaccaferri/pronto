#pragma once
#include <stdlib.h>
#include <strings.h>

typedef struct {
    int length; // as an example, max message size is 4GB (i won't check for overflows because i'm lazy and this is an example)
    char* data;
    int allocations;
    int buf_size; // defaults to this
    int fd;
} connection_context;

extern void context_init(connection_context* context, unsigned int request_size);
void context_write(connection_context* context, char* data, size_t received_bytes);
void context_destroy(connection_context* context);