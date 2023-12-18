#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "picohttpparser.h"
#include "http_request.h"

extern void make_nonblocking(int fd);
extern int count_lines(char* bytes, size_t byte_len);
extern char** bytes_to_lines(char* bytes, size_t byte_len, int lines_num);
extern char* file_to_string(char* filename);
extern char* filename_to_mimetype_header(char* filename);
extern char* filename_to_extension(char* filename);
extern char* read_whole_file(int fd);
extern char* get_header(struct phr_header* headers, int len, char* wanted_header);
extern bool has_body(http_request *req);
extern void to_lowercase(char* string, int len);

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

#define ERROR(res, e, status, s) INTO_RESPONSE(res, "{\"success\": false, \"error\": " ## #e ## "}", status, s)