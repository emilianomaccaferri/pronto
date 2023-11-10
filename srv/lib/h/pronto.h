#pragma once
#include "server.h"
#include "cluster_worker.h"
#include "prio_queue.h"
#include <pthread.h>
#include <semaphore.h>

typedef struct pronto{
    struct server* http_server;
    int workers;
    struct cluster_worker* workers_instances;
    prio_queue* job_queue;
    pthread_mutex_t queue_mutex; // this will be the mutex that every scheduler thread will acquire
                                // to fetch jobs from the queue
    sem_t notify;               // semaphore used by consumers to wait for new jobs on the queue
} pronto;

extern void pronto_init(
    struct pronto* p,
    int workers
);
extern void pronto_start_http(struct pronto* p);