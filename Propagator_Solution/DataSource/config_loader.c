#include "config_loader.h"

NodeInfo* load_root(const char* path, NodeInfo** outNodes, size_t* outCount) {
    NodeInfo* nodes = node_load_all(path, outCount);
    *outNodes = nodes;
    return nodes;
}