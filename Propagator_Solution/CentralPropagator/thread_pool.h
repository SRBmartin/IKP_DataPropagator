#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>
#include <stdbool.h>

/// signature for any pool‐worker callback
typedef void (*tp_task_fn)(void* arg);

/// opaque handle to the pool
typedef struct ThreadPool ThreadPool;

/// build a pool of exactly `thread_count` workers
ThreadPool* tp_create(size_t thread_count);

/// enqueue (fn,arg) → worker
/// returns false if shutting down
bool        tp_submit(ThreadPool* pool, tp_task_fn fn, void* arg);

/// wait for all tasks, join threads, free everything
void        tp_shutdown(ThreadPool* pool);

#endif // THREAD_POOL_H
