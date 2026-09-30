#include "thread_pool.h"
#include <stdio.h>

void *worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    while (true) {
        pthread_mutex_lock(&pool->mutex);
        while (pool->active_tasks == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->cond_task, &pool->mutex);
        }
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }
        Task task = pool->queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_capacity;
        pool->active_tasks--;
        pthread_mutex_unlock(&pool->mutex);
        task.function(task.arg);
    }
    return NULL;
}

ThreadPool *thread_pool_create(int thread_count, int queue_capacity) {
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->thread_count = thread_count;
    pool->queue = (Task *)malloc(sizeof(Task) * queue_capacity);
    pool->queue_capacity = queue_capacity;
    pool->queue_head = 0;
    pool->queue_tail = 0;
    pool->active_tasks = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond_task, NULL);
    pthread_cond_init(&pool->cond_done, NULL);
    pool->shutdown = false;

    for (int i = 0; i < thread_count; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    return pool;
}

void thread_pool_destroy(ThreadPool *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->cond_task);
    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    free(pool->queue);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond_task);
    pthread_cond_destroy(&pool->cond_done);
    free(pool);
}

bool thread_pool_submit(ThreadPool *pool, void (*function)(void *), void *arg) {
    pthread_mutex_lock(&pool->mutex);
    if (pool->active_tasks == pool->queue_capacity) {
        pthread_mutex_unlock(&pool->mutex);
        return false; // Queue is full
    }
    pool->queue[pool->queue_tail].function = function;
    pool->queue[pool->queue_tail].arg = arg;
    pool->queue_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    pool->active_tasks++;
    pthread_cond_signal(&pool->cond_task);
    pthread_mutex_unlock(&pool->mutex);
    return true;
}

void thread_pool_wait(ThreadPool *pool) {
    pthread_mutex_lock(&pool->mutex);
    while (pool->active_tasks > 0) {
        pthread_cond_wait(&pool->cond_done, &pool->mutex);
    }
    pthread_mutex_unlock(&pool->mutex);
}
