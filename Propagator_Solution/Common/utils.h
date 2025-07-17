#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include "node.h"

void initializeConsoleSettings(void);
NodeInfo* node_find_by_id(NodeInfo* nodes, size_t count, const char* id);
NodeInfo* find_root(NodeInfo* n);
uint64_t network_to_host64(uint64_t x);


#ifdef _DEBUG
#include <stdio.h>
#include <crtdbg.h>
#include <Psapi.h>

void dump_to_file(FILE* file, const char* header, const _CrtMemState* state);
FILE* setup_debug_memory_log(const char* node_id);
void print_stats_to_file(FILE* file, const _CrtMemState* s1, const _CrtMemState* s2, const _CrtMemState* sDiff, PROCESS_MEMORY_COUNTERS* pmc);
#endif // _DEBUG

#endif // _UTILS_H