#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdbool.h>
#include "h/pronto.h"
#include "h/cluster_worker.h"
#include "h/scheduler_worker.h"
#include "h/config.h"
#include "h/node.h"
#include "h/job.h"

void pronto_start_http(struct pronto* instance){
    server_loop(instance->http_server);
}

void pronto_init(
    struct pronto* instance,
    int workers,
    int schedulers
){

    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutex_init(&instance->queue_mutex, &a);
    sem_init(&instance->notify, 0, 0);

    instance->job_queue = malloc(sizeof(prio_queue));
    instance->waiting_queue = malloc(sizeof(prio_queue));

    prio_queue_init(instance->job_queue);
    prio_queue_init(instance->waiting_queue);

    instance->total_capacity = 0;
    instance->schedulers = schedulers;
    instance->workers = workers;
    instance->workers_instances = malloc(sizeof(struct cluster_worker) * instance->workers);
    instance->schedulers_instances = malloc(sizeof(struct scheduler_worker) * instance->schedulers);

    instance->http_server = malloc(sizeof(struct server));

    // initialize schedulers 
    for(int i = 0; i < instance->schedulers; i++){
        scheduler_worker_init(&instance->schedulers_instances[i], instance);
    }
    // initialize workers
    for(int i = 0; i < instance->workers; i++){
        cluster_worker_init(&instance->workers_instances[i], instance);
        instance->total_capacity += instance->workers_instances[i].max_resources; // this will be constant once set
    } 
    instance->current_capacity = instance->total_capacity;
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

    if(p->total_capacity <= value) return false;

    for(int i = 0; i < p->workers; i++){
        if(p->workers_instances[i].max_resources >= value) 
            return true;
    }
    return false;

}

/* 
    this determines if there is at least a worker ready to accept a job
    if there are no workers ready, this function returns -1, indicating that there's no worker able to schedule this particular job
    if there is more than one worker that can schedule the job, the "least-busy" worker is chosen
*/
int pronto_best_current_fit(struct pronto *p, unsigned int value){
    
    if(p->current_capacity < value)
        return -1;

    int capacity = -1;
    int worker_index = -1; 
    for(int i = 0; i < p->workers; i++){
        if(value <= p->workers_instances[i].current_resources){
            int i_th_capacity = p->workers_instances[i].current_resources;
            if(capacity < i_th_capacity){ // i need to find the leasy-busy worker
                capacity = i_th_capacity; // the capacity cannot be < 0, so this works fine
                worker_index = i;
            }
        }
    }

    return worker_index;

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

void pronto_add_waiting_job(struct pronto *p, unsigned int value){

    struct node_t *node = calloc(1, sizeof(struct node_t));
    job* j = calloc(1, sizeof(job));
    j->request = value;
    node->value = j;

    pthread_mutex_lock(&p->waiting_queue_mutex);
    prio_queue_enqueue(p->waiting_queue, node);
    pthread_mutex_unlock(&p->waiting_queue_mutex);

}

void pronto_decrease_current_capacity(struct pronto *p, unsigned int value){
    pthread_mutex_lock(&p->bookkeeper);
    p->current_capacity -= value;
    pthread_mutex_unlock(&p->bookkeeper);
}