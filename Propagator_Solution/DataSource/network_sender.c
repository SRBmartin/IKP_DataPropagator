#include <stdio.h>
#include "network_sender.h"
#include "../Common/Warning.h"
#include "../Common/propagator_client.h"
#include "../Common/utils.h"

void* network_sender_thread(void* arg) {
    SenderArgs* a = (SenderArgs*)arg;
    while (1) {
        Warning* w = tsqueue_dequeue(a->queue);
        if (!w) continue;

        NodeInfo* dest = node_find_by_id(
            a->nodes, a->node_count, w->dest_node
        );
        if (!dest) {
            warning_destroy(w);
            continue;
        }

        NodeInfo* root = find_root(dest);

        if (!send_warning_to(root->address, root->port, w)) {
            printf("[DS - ERROR] Failed to propagate data to %s.\n", root->id);
        }

        warning_destroy(w);
    }
    return NULL;
}