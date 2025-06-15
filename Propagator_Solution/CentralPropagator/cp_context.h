#ifndef CP_CONTEXT_H
#define CP_CONTEXT_H

#include <stddef.h>
#include "../Common/node.h"
#include "../Common/hashmap.h"

typedef struct {
    HashMap* map;
    NodeInfo* me;
    NodeInfo** children;
    size_t     child_count;
    NodeInfo* all_nodes;
    size_t     all_count;
} CPContext;

CPContext* cp_context_create(const char* nodes_csv, const char* root_id);
void       cp_context_destroy(CPContext* ctx);

#endif
