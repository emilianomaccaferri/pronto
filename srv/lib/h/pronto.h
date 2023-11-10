#pragma once
#include "server.h"
#include "cluster_worker.h"
#include "prio_queue.h"
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct pronto{
    struct server* http_server;
    int workers;
    struct cluster_worker* workers_instances;
    int total_capacity;                   // this is constant and represents the cluster's total "theoretical" capacity
    int current_capacity;                 // this is used to track the pronto's workers total capacity 
    prio_queue* job_queue;
    pthread_mutex_t queue_mutex; // this will be the mutex that every scheduler thread will acquire
                                // to fetch jobs from the queue
    pthread_mutex_t bookkeeper; // mutex used to change values (such as capacity) inside the main instance 
    sem_t notify;               // semaphore used by consumers to wait for new jobs on the queue
} pronto;

extern void pronto_init(
    struct pronto* p,
    int workers
);
extern void pronto_start_http(struct pronto* p);
extern bool pronto_is_schedulable(struct pronto* p, int value);
extern void pronto_add_job(struct pronto* p, unsigned int value);