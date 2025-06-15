#include "utils.h"

#pragma comment(lib, "Ws2_32.lib")

void initializeConsoleSettings(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

NodeInfo* node_find_by_id(NodeInfo* nodes, size_t count, const char* id) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(nodes[i].id, id) == 0) {
            return &nodes[i];
        }
    }
    return NULL;
}

NodeInfo* find_root(NodeInfo* n) {
    while (n->parent) {
        n = n->parent;
    }
    return n;
}

uint64_t network_to_host64(uint64_t x) {
    uint32_t hi_net = (uint32_t)x;
    uint32_t lo_net = (uint32_t)(x >> 32);
    uint32_t hi = ntohl(hi_net);
    uint32_t lo = ntohl(lo_net);
    return ((uint64_t)hi << 32) | lo;
}