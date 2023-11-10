#pragma once
#include <stddef.h>
#include "picohttpparser.h"

extern void make_nonblocking(int fd);
extern int count_lines(char* bytes, size_t byte_len);
extern char** bytes_to_lines(char* bytes, size_t byte_len, int lines_num);
extern char* file_to_string(char* filename);
extern char* filename_to_mimetype_header(char* filename);
extern char* filename_to_extension(char* filename);
extern char* read_whole_file(int fd);
extern char* get_header(struct phr_header* headers, int len, char* wanted_header);

#define INTO_RESPONSE(res, b, status, s) ({ \
    memcpy( \
        res, http_response_create( \
            status, \
            "Content-Type: application/json", \
            b, \
            s \
        ),  \
        sizeof(http_response) \
    ); \
})