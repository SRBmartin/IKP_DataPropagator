#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include "node.h"

void initializeConsoleSettings(void);
NodeInfo* node_find_by_id(NodeInfo* nodes, size_t count, const char* id);
NodeInfo* find_root(NodeInfo* n);
uint64_t network_to_host64(uint64_t x);

#endif