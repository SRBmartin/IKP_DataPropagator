#ifndef CP_CONTEXT_H
#define CP_CONTEXT_H

#include <stddef.h>
#include "../Common/node.h"
#include "../Common/hashmap.h"

// A CPContext holds your subtree map + pointers into it.
typedef struct {
    HashMap* map;         // id → NodeInfo*
    NodeInfo* me;          // this node’s own info
    NodeInfo** children;    // immediate children
    size_t     child_count;
    NodeInfo* all_nodes;   // for cleanup
    size_t     all_count;
} CPContext;

// load & destroy
CPContext* cp_context_create(const char* nodes_csv, const char* root_id);
void       cp_context_destroy(CPContext* ctx);

#endif // CP_CONTEXT_H
