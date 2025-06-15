#ifndef NETWORK_SENDER_H
#define NETWORK_SENDER_H

#include "../Common/tsqueue.h"
#include "../Common/node.h"

typedef struct {
    TSQueue* queue;
    NodeInfo* nodes;
    size_t     node_count;
    HANDLE stopEvent;
} SenderArgs;

void* network_sender_thread(void* arg);

#endif