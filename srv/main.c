#define MAX_EVENTS 128
#define NUM_HANDLERS 1 // how many threads handle epolls
#define PORT 8080
#define MAX_EPOLL_HANDLER_QUEUE_SIZE 2048
#define REQUEST_BUFFER_SIZE 32368 // HTTP request chunk is 32k
#define MAX_REQUEST_SIZE 120000

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "lib/h/utils.h"
#include "lib/h/server.h"

int main(void){

    server* http_server = server_init(PORT, MAX_EVENTS, NUM_HANDLERS, MAX_EPOLL_HANDLER_QUEUE_SIZE, REQUEST_BUFFER_SIZE, MAX_REQUEST_SIZE);
    printf("\nserver is now listening on localhost:%d\n", PORT);

    server_loop(http_server);

    return 0;

}