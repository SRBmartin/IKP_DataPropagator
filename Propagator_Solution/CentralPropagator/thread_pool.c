#include "thread_pool.h"
#include "../Common/tsqueue.h"
#include <windows.h>
#include <stdlib.h>

typedef struct {
    tp_task_fn fn;
    void* arg;
} Task;

struct ThreadPool {
    size_t    thread_count;
    HANDLE* threads;
    TSQueue* queue;
    bool      shutting_down;
};

static DWORD WINAPI worker_thread(LPVOID lpParam) {
    ThreadPool* pool = lpParam;
    for (;;) {
        Task* t = (Task*)tsqueue_dequeue(pool->queue);
        if (!t) break;

        t->fn(t->arg);
        free(t);
    }
    return 0;
}

ThreadPool* tp_create(size_t thread_count) {
    ThreadPool* pool = malloc(sizeof(*pool));
    if (!pool) return NULL;
    pool->thread_count = thread_count;
    pool->shutting_down = false;
    pool->queue = tsqueue_create(0, free);
    if (!pool->queue) { free(pool); return NULL; }

    pool->threads = malloc(thread_count * sizeof(HANDLE));
    if (!pool->threads) {
        tsqueue_destroy(pool->queue);
        free(pool);
        return NULL;
    }

    for (size_t i = 0; i < thread_count; i++) {
        pool->threads[i] = CreateThread(NULL, 0, worker_thread, pool, 0, NULL);
    }
    return pool;
}

bool tp_submit(ThreadPool* pool, tp_task_fn fn, void* arg) {
    if (!pool || pool->shutting_down || !fn) return false;
    Task* t = malloc(sizeof(*t));
    if (!t) return false;
    t->fn = fn;
    t->arg = arg;
    tsqueue_enqueue(pool->queue, t);
    return true;
}

void tp_shutdown(ThreadPool* pool) {
    if (!pool) return;
    pool->shutting_down = true;

    for (size_t i = 0; i < pool->thread_count; ++i) {
        tsqueue_enqueue(pool->queue, NULL);
    }

    for (size_t i = 0; i < pool->thread_count; i++) {
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
    }

    tsqueue_destroy(pool->queue);

    free(pool->threads);
    free(pool);
}
