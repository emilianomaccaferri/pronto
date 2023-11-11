#pragma once
#include "node.h"

typedef struct {
    int size;
    struct node_t *head, *tail;
} prio_queue;

extern void prio_queue_init(prio_queue *q);
extern void prio_queue_enqueue(prio_queue *q, struct node_t *el); 
extern struct node_t* prio_queue_fifo_dequeue(prio_queue *q);