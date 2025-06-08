#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <stddef.h>
#include "../Common/node.h"

NodeInfo* load_root(const char* path, NodeInfo** outNodes, size_t* outCount);

#endif