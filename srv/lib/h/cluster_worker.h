#pragma once
#include <pthread.h>

struct cluster_worker {
    pthread_mutex_t worker_mutex; // this is the mutex the *server* (and the worker itself) will acquire to edit 
                                 // the resources of a given worker
    unsigned long max_resources;
    unsigned long current_resources;
    unsigned int scheduled_jobs;
};

void cluster_worker_fetch(struct cluster_worker* cw);
void cluster_worker_init(struct cluster_worker* cw);