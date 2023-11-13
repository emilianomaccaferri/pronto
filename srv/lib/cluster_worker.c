#define _GNU_SOURCE
#include "h/cluster_worker.h"
#include "h/config.h"
#include "h/c_json.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

void *_cluster_loop(void *cw);

void cluster_worker_init(
        struct cluster_worker *cw, 
        struct pronto *instance, 
        int index,
        short port
    ){
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutex_init(&cw->worker_mutex, &a);
    cw->current_resources = MAX_RESOURCES; 
    cw->max_resources = MAX_RESOURCES; 
    cw->scheduled_jobs = 0;
    cw->thread = (pthread_t *)malloc(sizeof(pthread_t));
    cw->pronto = instance;
    cw->active = true;
    cw->scheduler_jobs = malloc(sizeof(prio_queue));
    cw->port = port;
    cw->worker_id = index;
    cw->curl = curl_easy_init();
    
    asprintf(&cw->endpoint, "http://srv-%d:%d", cw->worker_id, port);
    prio_queue_init(cw->scheduler_jobs);
    sem_init(&cw->notify, 0, 0);

    pthread_create(cw->thread, NULL, _cluster_loop, (void *)cw);

}

void *_cluster_loop(void *cw){

    struct cluster_worker *worker = (struct cluster_worker *)cw;
    fprintf(stdout, "cluster worker %d started\n", worker->worker_id);

    while(worker->active){
        sem_wait(&worker->notify);
        pthread_mutex_lock(&worker->worker_mutex);
        struct node_t* node = prio_queue_fifo_dequeue(worker->scheduler_jobs);
        pthread_mutex_unlock(&worker->worker_mutex);
        if(worker->curl){
            CURLcode res = CURLE_FAILED_INIT;
            cJSON* json_obj = cJSON_CreateObject();
            if(!json_obj){
                fprintf(stderr, "BUG: json obj is null\n");
                continue;
            }
            if(!cJSON_AddNumberToObject(json_obj, "value", node->value->request)) {
                fprintf(stderr, "err: failed to add value.\n");
                cJSON_Delete(json_obj); 
                continue;
            }
            if(!cJSON_AddNumberToObject(json_obj, "worker", worker->worker_id)) {
                fprintf(stderr, "err: failed to add worker id.\n");
                cJSON_Delete(json_obj);
                continue;
            }
            char *str_json = cJSON_PrintUnformatted(json_obj);
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Expect:");
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(worker->curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(worker->curl, CURLOPT_URL, worker->endpoint);
            curl_easy_setopt(worker->curl, CURLOPT_POST, 1L);
            curl_easy_setopt(worker->curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(worker->curl, CURLOPT_POSTFIELDS, str_json);
            res = curl_easy_perform(worker->curl);

            if(res != CURLE_OK){
                fprintf(stderr, "err: failed to post data to worker!\n");
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(worker->curl);
            cJSON_Delete(json_obj);
            free(str_json);
        }else{
            fprintf(stderr, "BUG: curl is NULL, cannot schedule job\n");
        }
    }

    pthread_exit(0);

}

void cluster_worker_add_job(struct cluster_worker *cw, unsigned int value){

    struct node_t *node = calloc(1, sizeof(struct node_t));
    job* j = calloc(1, sizeof(job));
    j->request = value;
    node->value = j;

    pthread_mutex_lock(&cw->worker_mutex);
    prio_queue_enqueue(cw->scheduler_jobs, node);
    cw->current_resources -= value;
    cw->scheduled_jobs++;
    pthread_mutex_unlock(&cw->worker_mutex);

}