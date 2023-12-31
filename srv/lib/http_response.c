#include "h/http_response.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

char* default_headers = "Connection: close\nServer: pronto-srv/0.0.1\n";

static char* stringify_status(int status);

void http_response_destroy(http_response* res){
    // free(res->body);
    free(res->headers);
    free(res->stringified);
    free(res);
}

http_response* http_response_create(int status, char* headers, char* body, int socket){

    /*
        status: the HTTP status
        body: a null-terminated string representing the response body
        headers: a null-terminated string representing the response headers
    */

    http_response* res = (http_response*) malloc(sizeof(http_response));
    char date_buf[255];

    time_t now = time(NULL);
    struct tm* gmt_time = gmtime(&now);
    strftime(date_buf, 255, "Date: %a, %d %b %Y %H:%M:%S %Z\n", gmt_time);

    res->socket = socket;
    res->stream_ptr = 0;
    res->content_length = strlen(body);
    res->date_header = date_buf;
    res->date_header_length = strlen(date_buf);

    int content_length_header_length = snprintf(NULL, 0, "%d", res->content_length);
    char* content_length_header = malloc(sizeof(char) * (content_length_header_length + 18)); // 17 is "Content-Length: " + terminator

    snprintf(content_length_header, content_length_header_length + 18, "Content-Length: %d\n", res->content_length);

    if(headers){
        res->headers_length = strlen(headers) + strlen(default_headers) + content_length_header_length + 18 + res->date_header_length;
    }else{
        res->headers_length = strlen(default_headers) + content_length_header_length + 18 + res->date_header_length;
    }
    
    res->headers = malloc(sizeof(char) * (res->headers_length + 1));
    res->body = malloc(sizeof(char) * (res->content_length + 1));
    res->status = status;

    strncpy(res->headers, default_headers, strlen(default_headers));
    strncpy(res->headers, res->date_header, res->date_header_length);
    strncat(res->headers, content_length_header, content_length_header_length);
    strncpy(res->body, body, res->content_length);

    if(headers) 
        strncpy(res->headers, headers, res->headers_length);
    
    res->headers[res->headers_length] = 0;
    res->body[res->content_length] = 0;

    res->stringified = http_response_stringify(res);
    res->full_length = strlen(res->stringified);
    
    return res;

}

char* http_response_stringify(http_response* res){
    
    char* status_line = stringify_status(res->status); 
    int status_line_length = strlen(status_line);
    int total_length = res->content_length + res->headers_length + status_line_length;
    char* response_string = malloc(sizeof(char) * total_length);

    bzero(response_string, total_length);
    strncat(response_string, status_line, status_line_length);
    strncat(response_string, res->headers, res->headers_length);
    strncat(response_string, "\n\r\n", 4);
    strncat(response_string, res->body, res->content_length);

    return response_string;

}

char* stringify_status(int status){

    // no time for a hashmap, sorry
    switch(status){
        case 200:
            return "HTTP/1.1 200 OK\n";
        case 400:
            return "HTTP/1.1 400 Bad Request\n";
        case 404:
            return "HTTP/1.1 404 Not Found\n";
        case 405:
            return "HTTP/1.1 405 Method Not Allowed\n";
        default:
            return "HTTP/1.1 500 Internal Server Error\n";
    }

}

http_response* http_response_bad_request(int socket_fd){
    
    return 
        http_response_create(400, "Content-Type: application/json", "{\"success\": false, \"error\": \"bad_request\"}", socket_fd);
    
}

http_response* http_response_uninmplemented_method(int socket_fd){
    
    return 
        http_response_create(501, "Content-Type: application/json", "{\"success\": false, \"error\": \"method_not_implemented\"}", socket_fd);
    
}

http_response* http_response_internal_server_error(int socket_fd){
    
    return 
        http_response_create(500, "Content-Type: application/json", "{\"success\": false, \"error\": \"internal_server_error\"}", socket_fd);
    
}

http_response* http_response_not_found(int socket_fd){
    
    return 
        http_response_create(404, "Content-Type: application/json", "{\"success\": false, \"error\": \"not_found\"}", socket_fd);
    
}
http_response* http_response_not_allowed(int socket_fd){
    
    return 
        http_response_create(405, "Content-Type: application/json", "{\"success\": false, \"error\": \"method_not_allowed\"}", socket_fd);
    
}