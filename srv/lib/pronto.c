#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdbool.h>
#include "h/pronto.h"
#include "h/cluster_worker.h"
#include "h/config.h"
#include "h/node.h"
#include "h/job.h"

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

    // initializing the semaphone used by the handlers and workers
    sem_init(&instance->notify, 0, 0);

    // i also need to initialize the queue
    instance->job_queue = malloc(sizeof(prio_queue));
    prio_queue_init(instance->job_queue);

    instance->total_capacity = 0;
    instance->workers = workers;
    instance->workers_instances = malloc(sizeof(struct cluster_worker) * instance->workers);

    instance->http_server = malloc(sizeof(struct server));
    /* initialize workers */
    for(int i = 0; i < instance->workers; i++){
        cluster_worker_init(&instance->workers_instances[i]);
        instance->total_capacity += instance->workers_instances[i].max_resources;
    } 
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

}

bool pronto_is_schedulable(struct pronto *p, int value){

    return p->total_capacity >= value;

}

void pronto_add_job(struct pronto *p, unsigned int value){

    struct node_t *node = calloc(1, sizeof(struct node_t));
    job* j = calloc(1, sizeof(job));
    j->request = value;
    node->value = j;

    pthread_mutex_lock(&p->queue_mutex);
    
    prio_queue_enqueue(p->job_queue, node);

    pthread_mutex_unlock(&p->queue_mutex);

}