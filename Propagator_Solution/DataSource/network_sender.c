#include "network_sender.h"
#include "../Common/propagator_client.h"
#include "../Common/Warning.h"

void* network_sender_thread(void* arg) {
    SenderArgs* a = arg;
    while (1) {
        Warning* w = tsqueue_dequeue(a->queue);
        propagate_warning(w);
        warning_destroy(w);
    }
    return NULL;
}