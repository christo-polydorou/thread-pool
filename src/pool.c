#include "../include/cqueue.h"
#include "../include/pool.h"
#include "../include/rc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void* thread_fun(void* arg){
    thread_arg_t* the_arg = (thread_arg_t*) arg;
    rc_t* rc = malloc(sizeof(rc_t));    
    uint32_t size;
    int isNull = 1;

    while (isNull){
        pool_result_t* result;
        pool_work_t* work;
        *rc = cqueue_dequeue(the_arg->work_queue, sizeof(pool_work_t), (void**)work, &size, NULL);
        if (*rc != Success) {
            fprintf(stderr, "There was an error dequeueing, error value was %d\n", *rc);
            return rc;
        }
        
        if (work->fun_ptr == NULL){
            isNull = 0;
        } else{
            *rc = work->fun_ptr(work->arg_ptr, result->result);
            if (*rc != Success){
                fprintf(stderr, "Fun execution was not successful");
                return rc;
            }
            result->id = work->id;
            result->rc = *rc;
            *rc = cqueue_enqueue(the_arg->results_queue, (void*)result, size, NULL);
            if (*rc != Success){
                fprintf(stderr, "There was an error enqueueing to results queue, error value was %d\n", *rc);
                return rc;
            }
        }
    }
  
    return rc;
}

rc_t pool_create(thread_pool_t* pool, int pool_size){
    rc_t rc;
    thread_arg_t the_arg;
    cqueue_attr_t work_attrs;
    cqueue_attr_t results_attrs;

    if (pool_size < 1){
        fprintf(stderr, "Size of pool needs to be larger than 0");
        return Error;
    } else {
        pool->size = pool_size;
    }

    if (pool == NULL || &pool_size == NULL){
        fprintf(stderr, "The inputed pool and pool_size cannot be NULL\n");
        return Error;
    }

    //Make work queue
    rc = cqueue_attr_init(&work_attrs);
    if (rc != Success) {
        fprintf(stderr, "Error calling attr init.\n");
        return Error;
    }

    work_attrs.block_size = sizeof(pool_work_t);
    work_attrs.num_blocks = 32;

    rc = cqueue_create(&pool->work_queue, &work_attrs);
    if (rc != Success) {
        fprintf(stderr, "Error calling cqueue create.\n");
        return Error;
    }

    //Make results
    rc = cqueue_attr_init(&results_attrs);
    if (rc != Success) {
        fprintf(stderr, "Error calling attr init.\n");
        return Error;
    }

    results_attrs.block_size = sizeof(pool_result_t);
    results_attrs.num_blocks = 32;

    rc = cqueue_create(&pool->results_queue, &results_attrs);
    if (rc != Success) {
        fprintf(stderr, "Error calling cqueue create.\n");
        return Error;
    }
    
    pool->threads = malloc(sizeof(pthread_t)*pool_size);
    if (pool->threads == NULL){
        fprintf(stderr, "Memory allocation for %d threads was not possible", pool_size);
        return OutOfMemory;
    } 

    the_arg.work_queue = &pool->work_queue;
    the_arg.results_queue = &pool->results_queue;

    fprintf(stderr, "Creating %d threads\n", pool->size);
    for (int k=0; k<pool->size; k++) {
        int prc = pthread_create(&pool->threads[k], NULL, &thread_fun, &the_arg);
        if (prc != 0) {
            fprintf(stderr, "There was a problem during pthread create with error=%d\n", prc);
            return Error;
        }
    }
    
    return Success;
}

rc_t pool_map(thread_pool_t* pool, pool_fun_t fun, int arg_count, void* args[], void* results[]){
    rc_t* rc;
    uint32_t size; 

    if (pool == NULL){
        fprintf(stderr, "Pool cannot be null\n");
        return Error;
    }

    if (fun == NULL){
        fprintf(stderr, "Function cannot be null\n");
        return Error;
    }
    
    for (int i=0;i<arg_count;i++){
        pool_work_t req;
        if (args != NULL){
            req.arg_ptr = args[i];
        }
        req.id = i;
        req.fun_ptr = fun;
        *rc = cqueue_enqueue(&pool->work_queue, (void*)&req, sizeof(pool_work_t), NULL);
        if (*rc != Success) {
            fprintf(stderr, "There was an error enqueueing to work queue, error value was %d\n", *rc);
            return *rc;
        }
        fprintf(stderr, "Succesfully enqueued the request with id:%dt\n", i);
    }

    for (int k=0;k<arg_count;k++){
        pool_result_t* result;
        *rc = cqueue_dequeue(&pool->results_queue, sizeof(pool_result_t), (void**)&result, &size, NULL);
        if (*rc != Success){
            fprintf(stderr, "There was an error dequeueing from results queue, error value was %d\n", *rc);
            return *rc;
        }
        if (result->rc != Success){
            fprintf(stderr, "Result with an id of %d was not successful in computation", result->id);
            return result->rc;
        }
        if (results != NULL && result != NULL){
            results[result->id] = result->result;
        } 
    }

    return Success;
}

rc_t pool_destroy(thread_pool_t* pool){
    rc_t* rc;
    for (int k=0; k<pool->size; k++) {
        pool_work_t req;
        req.fun_ptr = NULL;
        *rc = cqueue_enqueue(&pool->work_queue, (void*)&req, sizeof(pool_work_t), NULL);
        if (*rc != Success) {
            fprintf(stderr, "There was an error enqueueing to work queue, error value was %d\n", *rc);
            return *rc;
        }
    }
    fprintf(stderr, "Joining %d threads\n", pool->size);
    for (int k=0; k<pool->size; k++) {
        int* trc;
        int prc = pthread_join(pool->threads[k], (void**)&trc);
        
        if (prc != 0) {
            printf("There was a problem during pthread join with error=%d\n", prc);
            return -1;  
        }

        if (trc == NULL) {
            fprintf(stderr, "Got NULL back for thread return code.\n");
            return -1;
        }
        if (*trc != Success) {
            fprintf(stderr, "There was a problem with the thread return code=%d\n", *trc);
            return -1; 
        }
        fprintf(stderr, "Successfully joined thread %d\n", k);

        free(trc);
    }


    //destroy work_queue
    cqueue_destroy(&pool->work_queue);

    //destroy results_queue
    cqueue_destroy(&pool->results_queue);
    //free the pools threads
    free(pool->threads);

    return Success;
}
