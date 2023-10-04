#pragma once
#include "job.h"

struct node_t {
    job* value;
    struct node_t *next;
};

extern void node_init(struct node_t *node, job value);