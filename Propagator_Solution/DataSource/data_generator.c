#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <stdint.h>
#include "data_generator.h"
#include "../Common/warning.h"

void* data_generator_thread(void* arg) {
    GeneratorArgs* a = (GeneratorArgs*)arg;
    size_t total = a->node_count;
    NodeInfo* all = a->nodes;
    size_t dest_count = 0;

    for (size_t i = 0; i < total; i++) {
        if (all[i].type == NODE_DESTINATION) dest_count++;
    }

    if (dest_count == 0) {
        return NULL;
    }

    NodeInfo** dests = malloc(dest_count * sizeof(NodeInfo*));
    size_t di = 0;
    for (size_t i = 0; i < total; i++) {
        if (all[i].type == NODE_DESTINATION) {
            dests[di++] = &all[i];
        }
    }

    srand((unsigned)time(NULL));

    while (WaitForSingleObject(a->stopEvent, 0) == WAIT_TIMEOUT) {
        size_t j = rand() % dest_count;
        NodeInfo* n = dests[j];

        Warning* w = warning_create(
            n->id,
            (WarningType)(rand() % 4),
            (double)(rand() % 100),
            (uint64_t)time(NULL),
            n->id
        );

        tsqueue_enqueue(a->queue, w);

        char* desc = warning_to_string(w);
        if (desc) {
			printf("[DS]: Enqueued warning: %s\n", desc);
            free(desc);
        }

        Sleep((rand() % 500) + 1000);
    }

    tsqueue_enqueue(a->queue, NULL); // We send NULL sentinel to break the loop and let the cleanup do it's job

    free(dests);
    return NULL;
}