#ifndef NETWORK_SENDER_H
#define NETWORK_SENDER_H

#include "../Common/tsqueue.h"

typedef struct { TSQueue* queue; } SenderArgs;
void* network_sender_thread(void* arg);

#endif