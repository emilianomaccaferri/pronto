#pragma once

#define MAX_EVENTS 128
#define NUM_HANDLERS 4 // how many threads handle epolls
#define PORT 8080
#define MAX_EPOLL_HANDLER_QUEUE_SIZE 2048
#define REQUEST_BUFFER_SIZE 32368 // HTTP request chunk is 32k
#define MAX_REQUEST_SIZE 122880
#define MAX_BODY_SIZE 122880 // body is 128k too
#define CLUSTER_WORKERS 4