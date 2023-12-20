#pragma once
#include <pthread.h>
#include <stdbool.h>
#include "pronto.h"

struct rescheduler {
    struct pronto* pronto;
    bool active;
    pthread_t* thread;
};

extern void rescheduler_init(struct rescheduler* r, struct pronto* p);