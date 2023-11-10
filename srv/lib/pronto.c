#include <pthread.h>
#include <stdlib.h>
#include "h/pronto.h"
#include "h/cluster_worker.h"
#include "h/config.h"

void pronto_start_http(struct pronto* instance){
    server_loop(instance->http_server);
}

void pronto_init(
    struct pronto* instance,
    int workers
){

    // initializing the queue mutex
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutex_init(&instance->queue_mutex, &a);

    // i also need to initialize the queue
    instance->job_queue = malloc(sizeof(prio_queue));
    prio_queue_init(instance->job_queue);

    instance->workers = workers;
    instance->workers_instances = malloc(sizeof(struct cluster_worker) * instance->workers);

    instance->http_server = malloc(sizeof(struct server));
    server_init(
        instance,
        instance->http_server,
        PORT, 
        MAX_EVENTS, 
        NUM_HANDLERS, 
        MAX_EPOLL_HANDLER_QUEUE_SIZE, 
        REQUEST_BUFFER_SIZE, 
        MAX_REQUEST_SIZE,
        MAX_BODY_SIZE
    );

    /* initialize handlers */
    // this MUST be done after the http_server has been initialized!
    

}