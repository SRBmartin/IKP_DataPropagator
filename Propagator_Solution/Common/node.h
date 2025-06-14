#ifndef NODE_H
#define NODE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    NODE_ROOT,
    NODE_CENTRAL,
    NODE_DESTINATION
} NodeType;

typedef struct NodeInfo {
    char* id;
    char* address;
    uint16_t        port;
    struct NodeInfo* parent;
    struct NodeInfo** children;
    size_t          child_count;
    NodeType        type;
} NodeInfo;

NodeInfo* node_load_all(const char* path, size_t* outCount);
void      node_info_destroy(NodeInfo* n);
void      node_info_destroy_all(NodeInfo* nodes, size_t count);

#endif