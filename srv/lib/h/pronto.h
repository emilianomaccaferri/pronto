#pragma once
#include "server.h"
#include "prio_queue.h"

typedef struct {
    server* http_server;
    cluster_worker* workers;
    prio_queue* job_queue;
} pronto;