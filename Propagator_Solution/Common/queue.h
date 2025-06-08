#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct QueueNode {
    void* data;
    struct QueueNode* next;
} QueueNode;

typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
    size_t size;
    void (*freeFunc)(void*);
} Queue;

Queue* queue_create(void (*freeFunc)(void*));
int    queue_enqueue(Queue* q, void* data);
void* queue_dequeue(Queue* q);
void* queue_peek(const Queue* q);
bool   queue_is_empty(const Queue* q);
size_t queue_size(const Queue* q);
void   queue_destroy(Queue* q);

#endif
