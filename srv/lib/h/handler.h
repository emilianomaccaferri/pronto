#pragma once
#include <stdbool.h>
#include <pthread.h>
#include "http_request.h"
#include "http_response.h"
#include "pronto.h"

/*
    the handler will process every request it gets from the main thread (i.e the server).
    every handler is thread that has a fixed-size epoll queue where connection events will occur; 
    for every event, the handler will parse the request and do something with it.

*/
// function that will be called when a route is matched
typedef int (*handler_callback)(http_request* req, http_response* res, int socket);
typedef struct {
    char* method;
    char* path;
    handler_callback callback;
} handler_route_t;
typedef enum {
    GET, POST
} http_method;

struct handler{
    
    /*
        every handler has a fixed-size epoll queue.
        if more than "max_events" file descriptors are ready when
        epoll_wait() is called, then successive epoll_wait() calls will
        round robin through the set of ready file descriptors. (https://man7.org/linux/man-pages/man2/epoll_wait.2.html)
    */
    struct pronto* pronto; // this is needed because every handler needs to access the internal mutex
    int max_request_size;
    int max_body_size;
    int max_events; 
    int epoll_fd;
    int request_buffer_size;
    int routes_idx;
    char* request_buffer; // where the request will be written
    struct epoll_event* events;
    handler_route_t** routes;

    pthread_t* thread;
    bool active;


};

void handler_init(
    struct pronto* instance,
    struct handler* handler, 
    int max_events, 
    int buf_size, 
    int max_request_size,
    int max_body_size
);

int handler_route(
    struct handler* handler,
    http_method route_method,
    char* route_path,
    handler_callback cb
);

handler_callback handler_route_search(
    struct handler* handler,
    const char* route_method,
    const char* route_path,
    int* status
);

