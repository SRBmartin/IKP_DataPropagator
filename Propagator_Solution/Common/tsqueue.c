#include "tsqueue.h"
#include <stdlib.h>

TSQueue* tsqueue_create(size_t bufferSize, void (*freeFunc)(void*)) {
    TSQueue* q = malloc(sizeof(*q));
    if (!q) return NULL;
    q->inner = queue_create(freeFunc);
    if (!q->inner) { free(q); return NULL; }
    q->capacity = bufferSize;
    InitializeCriticalSection(&q->cs);
    InitializeConditionVariable(&q->cvNotEmpty);
    InitializeConditionVariable(&q->cvNotFull);
    return q;
}

int tsqueue_enqueue(TSQueue* q, void* data) {
    EnterCriticalSection(&q->cs);
    while (q->capacity > 0 && queue_size(q->inner) >= q->capacity)
        SleepConditionVariableCS(&q->cvNotFull, &q->cs, INFINITE);
    int r = queue_enqueue(q->inner, data);
    WakeConditionVariable(&q->cvNotEmpty);
    LeaveCriticalSection(&q->cs);
    return r;
}

void* tsqueue_dequeue(TSQueue* q) {
    EnterCriticalSection(&q->cs);
    while (queue_is_empty(q->inner))
        SleepConditionVariableCS(&q->cvNotEmpty, &q->cs, INFINITE);
    void* d = queue_dequeue(q->inner);
    WakeConditionVariable(&q->cvNotFull);
    LeaveCriticalSection(&q->cs);
    return d;
}

void* tsqueue_try_dequeue(TSQueue* q) {
    EnterCriticalSection(&q->cs);
    void* d = queue_is_empty(q->inner) ? NULL : queue_dequeue(q->inner);
    if (d) WakeConditionVariable(&q->cvNotFull);
    LeaveCriticalSection(&q->cs);
    return d;
}

void tsqueue_destroy(TSQueue* q) {
    if (!q) return;
    EnterCriticalSection(&q->cs);
    queue_destroy(q->inner);
    LeaveCriticalSection(&q->cs);
    DeleteCriticalSection(&q->cs);
    free(q);
}