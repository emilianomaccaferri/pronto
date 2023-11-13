#include "h/cluster_worker.h"
#include "h/config.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void *_cluster_loop(void *cw);

void cluster_worker_init(struct cluster_worker *cw, struct pronto *instance){
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutex_init(&cw->worker_mutex, &a);
    cw->current_resources = MAX_RESOURCES; 
    cw->max_resources = MAX_RESOURCES; 
    cw->scheduled_jobs = 0;
    cw->thread = (pthread_t *)malloc(sizeof(pthread_t));
    cw->pronto = instance;
    cw->active = true;
    cw->scheduler_jobs = malloc(sizeof(prio_queue));
    prio_queue_init(cw->scheduler_jobs);
    sem_init(&cw->notify, 0, 0);

    pthread_create(cw->thread, NULL, _cluster_loop, (void *)cw);

}

void *_cluster_loop(void *cw){

    struct cluster_worker *worker = (struct cluster_worker *)cw;
    while(worker->active){
        sem_wait(&worker->notify);
        pthread_mutex_lock(&worker->worker_mutex);
        struct node_t* node = prio_queue_fifo_dequeue(worker->scheduler_jobs);
        pthread_mutex_unlock(&worker->worker_mutex);
    }

    pthread_exit(0);

}

void cluster_worker_add_job(struct cluster_worker *cw, unsigned int value){

    struct node_t *node = calloc(1, sizeof(struct node_t));
    job* j = calloc(1, sizeof(job));
    j->request = value;
    node->value = j;

    pthread_mutex_lock(&cw->worker_mutex);
    prio_queue_enqueue(cw->scheduler_jobs, node);
    cw->current_resources -= value;
    cw->scheduled_jobs++;
    pthread_mutex_unlock(&cw->worker_mutex);

}