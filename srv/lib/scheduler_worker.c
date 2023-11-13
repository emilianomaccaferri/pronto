#include "h/scheduler_worker.h"
#include "h/job.h"
#include "h/prio_queue.h"
#include "h/node.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void *_scheduler_loop(void *s);

void scheduler_worker_init(struct scheduler_worker *s, struct pronto *instance){
    s->pronto = instance;
    s->active = true;
    s->thread = (pthread_t *)malloc(sizeof(pthread_t));

    pthread_create(s->thread, NULL, _scheduler_loop, (void *)s);

}

void *_scheduler_loop(void *s){

    struct scheduler_worker *worker = (struct scheduler_worker *)s;
    while(worker->active){
        sem_wait(&worker->pronto->notify);
        pthread_mutex_lock(&worker->pronto->queue_mutex);
        struct node_t* node = prio_queue_fifo_dequeue(worker->pronto->job_queue);
        if(node == NULL){
            printf("bug: scheduler was notified but no job was placed in the queue\n");
        }else{
            int b = pronto_best_current_fit(worker->pronto, node->value->request);
            if(b == -1){
                // put the job in waiting queue since no best fit has been found yet
                pronto_add_waiting_job(worker->pronto, node->value->request);
            }else{
                // best-fit found: time to schedule the task
                struct cluster_worker* best_fit = &worker->pronto->workers_instances[b];
                cluster_worker_add_job(best_fit, node->value->request);
                pronto_decrease_current_capacity(worker->pronto, node->value->request);
                sem_post(&best_fit->notify); // notify the worker (dispatch)
                printf("[scheduler %ld]: scheduling on worker %d\n", pthread_self(), b); 
            }
            // dequeue doesn't free
            free(node); 
        }
        pthread_mutex_unlock(&worker->pronto->queue_mutex);
    }

    pthread_exit(0);

}