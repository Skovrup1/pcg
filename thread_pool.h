#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct Task {
    void (*function)(void *arg);
    void *arg;
} Task;

typedef struct ThreadPool {
    pthread_t *threads;
    int thread_count;
    Task *queue;
    int queue_capacity;
    int queue_head;
    int queue_tail;
    int active_tasks;
    pthread_mutex_t mutex;
    pthread_cond_t cond_task;
    pthread_cond_t cond_done;
    bool shutdown;
} ThreadPool;

ThreadPool *thread_pool_create(int thread_count, int queue_capacity);
void thread_pool_destroy(ThreadPool *pool);
bool thread_pool_submit(ThreadPool *pool, void (*function)(void *), void *arg);
void thread_pool_wait(ThreadPool *pool);

#endif // THREAD_POOL_H
