#pragma once
#include <pthread.h>

typedef struct {
    pthread_mutex_t worker_mutex; // this is the mutex the *server* will acquire to edit 
                                 // the resources of a given worker
    unsigned long max_resources;
    unsigned long current_resources;
    unsigned int scheduled_jobs;
} cluster_worker;