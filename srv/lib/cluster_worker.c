#include "h/cluster_worker.h"
#include <pthread.h>

void cluster_worker_init(cluster_worker* cw){
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutex_init(&cw->worker_mutex, &a);
    cw->current_resources = 0;
    cw->max_resources = 15; // todo: from config?
    cw->scheduled_jobs = 0;
}