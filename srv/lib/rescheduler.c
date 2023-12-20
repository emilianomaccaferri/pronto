#include "h/rescheduler.h"
#include <semaphore.h>
#include <stdlib.h>

void *_rescheduler_loop(void* item){

    struct rescheduler *me = (struct rescheduler*) item;
    while(me->active){
        sem_wait(&me->pronto->reschedule);
        fprintf(stdout, "job rescheduled!\n");
    }

    pthread_exit(0);

}

void rescheduler_init(struct rescheduler* r, struct pronto* p){
    r->pronto = p;
    r->active = true;
    r->thread = (pthread_t *) malloc(sizeof(pthread_t));

    pthread_create(r->thread, NULL, _rescheduler_loop, (void *)r);
}