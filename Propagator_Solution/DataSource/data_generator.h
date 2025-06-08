#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "../Common/tsqueue.h"
#include "../Common/node.h"

typedef struct { TSQueue* queue; NodeInfo* nodes; size_t node_count; } GeneratorArgs;
void* data_generator_thread(void* arg);

#endif