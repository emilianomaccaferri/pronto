#pragma once
#include "server.h"
#include "cluster_worker.h"
#include "prio_queue.h"
#include <pthread.h>

typedef struct {
    server* http_server;
    int workers;
    cluster_worker* workers_instances;
    prio_queue* job_queue;
    pthread_mutex_t queue_mutex; // this will be the mutex that every scheduler thread will acquire
                                // to fetch jobs from the queue
} pronto;

extern void pronto_init(
    pronto* p,
    int workers
);
extern void pronto_start_http(pronto* p);