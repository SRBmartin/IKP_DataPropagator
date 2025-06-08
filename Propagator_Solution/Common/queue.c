#include "queue.h"
#include <stdlib.h>

Queue* queue_create(void (*freeFunc)(void*)) {
    Queue* q = malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    q->freeFunc = freeFunc;
    return q;
}

int queue_enqueue(Queue* q, void* data) {
    if (!q) return -1;
    QueueNode* node = malloc(sizeof(QueueNode));
    if (!node) return -1;
    node->data = data;
    node->next = NULL;
    if (q->rear) {
        q->rear->next = node;
    }
    else {
        q->front = node;
    }
    q->rear = node;
    q->size++;
    return 0;
}

void* queue_dequeue(Queue* q) {
    if (!q || !q->front) return NULL;
    QueueNode* node = q->front;
    void* data = node->data;
    q->front = node->next;
    if (!q->front) q->rear = NULL;
    free(node);
    q->size--;
    return data;
}

void* queue_peek(const Queue* q) {
    return (q && q->front) ? q->front->data : NULL;
}

bool queue_is_empty(const Queue* q) {
    return !q || q->size == 0;
}

size_t queue_size(const Queue* q) {
    return q ? q->size : 0;
}

void queue_destroy(Queue* q) {
    if (!q) return;
    QueueNode* cur = q->front;
    while (cur) {
        QueueNode* next = cur->next;
        if (q->freeFunc && cur->data) {
            q->freeFunc(cur->data);
        }
        free(cur);
        cur = next;
    }
    free(q);
}