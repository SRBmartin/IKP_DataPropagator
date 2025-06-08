#ifndef TSQUEUE_H
#define TSQUEUE_H

#include "queue.h"
#include <stdbool.h>
#include <windows.h>

typedef struct TSQueue {
    Queue* inner;
    size_t           capacity;
    CRITICAL_SECTION  cs;
    CONDITION_VARIABLE cvNotEmpty;
    CONDITION_VARIABLE cvNotFull;
} TSQueue;

TSQueue* tsqueue_create(size_t bufferSize, void (*freeFunc)(void*));
int      tsqueue_enqueue(TSQueue* q, void* data);
void* tsqueue_dequeue(TSQueue* q);
void* tsqueue_try_dequeue(TSQueue* q);
void     tsqueue_destroy(TSQueue* q);

#endif