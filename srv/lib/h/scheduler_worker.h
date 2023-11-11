#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "pronto.h"

struct scheduler_worker {
    pthread_t *thread;
    struct pronto *pronto;
    bool active;
};

void scheduler_worker_fetch(struct scheduler_worker* s);
void scheduler_worker_init(struct scheduler_worker* s, struct pronto* p);