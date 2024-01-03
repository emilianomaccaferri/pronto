#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "pronto.h"
#include "prio_queue.h"


struct cluster_worker {
    pthread_mutex_t worker_mutex; // this is the mutex the *server* (and the worker itself) will acquire to edit 
                                 // the resources of a given worker
    unsigned long max_resources;     // this represents the MAX resources available for a given worker
    unsigned long current_resources; // this represents the current AVAILABLE resources
    unsigned int scheduled_jobs;
    sem_t notify;
    pthread_t *thread;
    struct pronto *pronto;
    prio_queue* scheduler_jobs;  // this contains jobs that are scheduled by schedulers
    bool active;
    char* endpoint;              // actual worker's address - the job is sent via HTTP to this address 
    short port;                  // the remote port
    int worker_id;               // the worker id that will be used for the hostname
};

extern void cluster_worker_fetch(struct cluster_worker* cw);
extern void cluster_worker_init(struct cluster_worker* cw, struct pronto* p, int index, short port);
extern void cluster_worker_add_job(struct cluster_worker *cw, unsigned int value);