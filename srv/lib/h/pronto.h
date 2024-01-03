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
    int schedulers;
    struct scheduler_worker* schedulers_instances;
    struct cluster_worker* workers_instances;   // dispatcher threads
    struct rescheduler* rescheduler;            // rescheduler thread
    int total_capacity;                         // this is constant and represents the cluster's total "theoretical" capacity
    int current_capacity;                       // this is used to track the pronto's workers total capacity 
    prio_queue* job_queue;                      // this queue is used by the http handlers to store jobs
    prio_queue* waiting_queue;                  // this queue is used by schedulers if no ready worker is found
    pthread_mutex_t waiting_queue_mutex;        // this will be the mutex that every scheduler thread will acquire for waiting jobs
    pthread_mutex_t queue_mutex;                // this will be the mutex that every scheduler thread will acquire for scheduled jobs
                                                    // to fetch jobs from the queue
    pthread_mutex_t bookkeeper;                 // mutex used to change values (such as capacity) inside the main instance 
    sem_t notify;                               // semaphore used by the scheduler threads waiting for jobs on the job queue
    sem_t reschedule;                           // semaphore used by the rescheduler thread to wait for jobs that have been completed
} pronto;

extern void pronto_init(
    struct pronto* p,
    int workers,
    int schedulers
);
extern void pronto_start_http(struct pronto* p);
extern bool pronto_is_schedulable(struct pronto* p, int value);
extern void pronto_add_job(struct pronto* p, unsigned int value);
extern void pronto_add_waiting_job(struct pronto *p, unsigned int value);
extern int pronto_best_current_fit(struct pronto *p, unsigned int value); 
extern void pronto_decrease_current_capacity(struct pronto* p, unsigned int value);
extern void pronto_increase_current_capacity(struct pronto* p, unsigned int value);
extern void pronto_reserve_capacity(struct pronto* p, struct cluster_worker*, unsigned int qty);
extern void pronto_free_capacity(struct pronto* p, struct cluster_worker** worker, unsigned int qty);