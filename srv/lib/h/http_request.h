#pragma once
#include "c_json.h"
#include "picohttpparser.h"

typedef struct {
    const char* method;
    const char* path;
    struct phr_header* headers;
    const char* body;
    cJSON* json_body;

    size_t method_len;
    size_t path_len;
    size_t num_headers;
    size_t body_len;
} http_request;

extern void http_request_destroy(http_request* req);
extern void http_request_from(
    http_request* empty,
    char* method,
    char* path,
    struct phr_header* headers,
    int method_len,
    int path_len,
    int num_headers
);
extern int http_request_parse_body(
    http_request* req, 
    char* body,
    int content_length,
    int socket,
    int sampling_buf_size
);