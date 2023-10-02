#include "h/http_request.h"

void http_request_empty(http_request* init){
    init->method = NULL;
    init->method_len = 0;
    init->path = NULL;
    init->path_len = 0;
    init->headers = NULL;
    init->num_headers = 0;
    init->body = NULL;
    init->parsed_body = NULL;
    init->body_len = 0;
}