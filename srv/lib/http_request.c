#include "h/http_request.h"
#include "h/picohttpparser.h"
#include "h/c_json.h"
#include "h/utils.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

void http_request_from(
    http_request* empty,
    char* method,
    char* path,
    struct phr_header* headers,
    int method_len,
    int path_len,
    int num_headers
){
    char actual_method[method_len + 1];
    char actual_path[path_len + 1];

    strncpy(actual_method, method, (int) method_len);
    strncpy(actual_path, path, (int) path_len);

    actual_method[method_len] = 0;
    actual_path[path_len] = 0; // terminate these strings!
    empty->method_len = method_len;
    empty->path_len = path_len;
    empty->num_headers = num_headers;

    empty->method = malloc(sizeof(char) * (1 + empty->method_len));
    empty->path = malloc(sizeof(char) * (1 + empty->path_len));
    empty->headers = malloc(sizeof(struct phr_header) * empty->num_headers);

    bzero((void*) empty->method, empty->method_len + 1);
    bzero((void*) empty->path, empty->path_len + 1);

    strncpy((char*) empty->method, actual_method, method_len + 1);
    strncpy((char*) empty->path, actual_path, path_len + 1);
    memcpy((void*) empty->headers, headers, sizeof(struct phr_header) * num_headers);
    
}
void http_request_destroy(http_request* req){
    //free((void*) req->body);
    //free((void*) req->path);
    //free((void*) req->headers);
    free((void*) req);
}