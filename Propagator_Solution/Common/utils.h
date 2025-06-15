#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include "node.h"

void initializeConsoleSettings(void);
NodeInfo* node_find_by_id(NodeInfo* nodes, size_t count, const char* id);
NodeInfo* find_root(NodeInfo* n);

#endif