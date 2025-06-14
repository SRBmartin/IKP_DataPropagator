#include "utils.h"

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