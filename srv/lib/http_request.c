#include "h/http_request.h"
#include "h/picohttpparser.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>

void http_request_init(
        http_request* init,
        char* method,
        size_t method_len, // non-terminated
        char* path,
        size_t path_len, // non-terminated
        struct phr_header* headers,
        int num_headers
){
    // +1 = we terminate the string!
    init->method_len = method_len; 
    init->path_len = path_len;
    init->num_headers = num_headers;

    // +1 => we need to terminate the string
    init->method = malloc(sizeof(char) * init->method_len) + 1;
    init->path = malloc(sizeof(char) * init->path_len + 1);
    init->headers = malloc(sizeof(struct phr_header) * init->num_headers);
    bzero((void*) init->method, init->method_len + 1);
    bzero((void*) init->path, init->path_len + 1);

    memcpy((void*) init->method, method, init->method_len);
    memcpy((void*) init->path, path, init->path_len);
    memcpy((void*) init->headers, headers, sizeof(struct phr_header) * init->path_len);
}
void http_request_destroy(http_request* req){
    //free((void*) req->body);
    //free((void*) req->path);
    //free((void*) req->headers);
    free((void*) req);
}