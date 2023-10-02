#pragma once
#include "c_json.h"
#include "picohttpparser.h"

typedef struct {
    const char* method;
    const char* path;
    struct phr_header* headers;
    const char* body;
    cJSON* parsed_body;

    size_t method_len;
    size_t path_len;
    size_t num_headers;
    size_t body_len;
} http_request;

void http_request_empty(http_request* init);