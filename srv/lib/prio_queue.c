#include "h/prio_queue.h"
#include <stdlib.h>
#include <assert.h>

void prio_queue_init(prio_queue *list){

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

}

void prio_queue_enqueue(prio_queue *list, struct node_t *el){

    if(list->size == 0){

        list->head = el;
        list->tail = el;
        list->tail->next = NULL;

    }else{

        struct node_t* curr_item = list->head;
        struct node_t* prev_of_curr_item = NULL;
        while(curr_item != NULL && (curr_item->value->request < el->value->request)){ 
            // keep going until we find a lower priority job
            prev_of_curr_item = curr_item;
            curr_item = curr_item->next;
        }

        if(prev_of_curr_item == NULL){
            // lower than the head: insert on top
            list->head = el;
            el->next = curr_item;  

            assert(list->head->value == el->value);
        }else if(curr_item == NULL){
            // higher than any element: insert at bottom
            el->next = NULL;
            list->tail->next = el;
            list->tail = el;
            assert(list->tail->value->request == el->value->request);
        }else{
            prev_of_curr_item->next = el;
            el->next = curr_item;  
        }

        assert(list->tail->next == NULL);

    }

    list->size++;

}

void prio_queue_fifo_dequeue(prio_queue *list){

    if(list->size == 0) return;

    struct node_t *old_head = list->head;

    list->head = list->head->next;
    list->size--;
    
    if(list->head == NULL) list->tail = NULL;

    free(old_head); 

}