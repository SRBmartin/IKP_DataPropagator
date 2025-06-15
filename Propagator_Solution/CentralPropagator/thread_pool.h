#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>
#include <stdbool.h>

typedef void (*tp_task_fn)(void* arg);
typedef struct ThreadPool ThreadPool;

ThreadPool* tp_create(size_t thread_count);
bool        tp_submit(ThreadPool* pool, tp_task_fn fn, void* arg);
void        tp_shutdown(ThreadPool* pool);

#endif
