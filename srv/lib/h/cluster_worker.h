#pragma once

typedef struct {
    unsigned long max_resources;
    unsigned long current_resources;
    unsigned int scheduled_jobs;
} cluster_worker;