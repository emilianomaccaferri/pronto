#include "h/rescheduler.h"
#include "h/node.h"
#include "h/pronto.h"
#include "h/prio_queue.h"

#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

void *_rescheduler_loop(void* item){

    struct rescheduler *me = (struct rescheduler*) item;

    // this worker will take jobs from the waiting queue and will try to re-schedule jobs that are placed in the waiting queue.

    while(me->active){
        sem_wait(&me->pronto->reschedule);
        fprintf(stdout, "[rescheduler]: triggered rescheduling\n");
        pthread_mutex_lock(&me->pronto->waiting_queue_mutex);
        // check if there's any waiting job that satisfies the current state of the cluster in terms of resources
        while(1){
            if(me->pronto->waiting_queue->head == NULL){
                fprintf(stdout, "[rescheduler]: no jobs to reschedule!\n"); 
                break;
            };

            int head_value = me->pronto->waiting_queue->head->value->request; // don't pop unless the job is schedulable (and we can remove it from the queue)
            // thread-safe best-fit search
            int b = pronto_best_current_fit(me->pronto, head_value);

            if(b != -1){
                // if the head is schedulable, we pop it!
                fprintf(stdout, "[rescheduler]: re-scheduling on worker %d\n", b); 

                struct node_t* node = prio_queue_fifo_dequeue(me->pronto->waiting_queue);
                struct cluster_worker* best_fit = &me->pronto->workers_instances[b];
                pronto_reserve_capacity(me->pronto, best_fit, node->value->request);
                cluster_worker_add_job(best_fit, node->value->request);

                sem_post(&best_fit->notify); // notify the worker (dispatch)

            }else{
                fprintf(stdout, "[rescheduler]: qty %d is not reschedulable. still waiting...\n", head_value);
                break;
            }; // this job is not schedulable, so we stop here!
        } 
        pthread_mutex_unlock(&me->pronto->waiting_queue_mutex);
    }

    pthread_exit(0);

}

void rescheduler_init(struct rescheduler* r, struct pronto* p){
    r->pronto = p;
    r->active = true;
    r->thread = (pthread_t *) malloc(sizeof(pthread_t));

    pthread_create(r->thread, NULL, _rescheduler_loop, (void *)r);
}