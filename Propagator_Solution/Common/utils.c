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

#ifdef _DEBUG
#include <crtdbg.h>
#include <stdio.h>

void dump_to_file(FILE* file, const char* header, const _CrtMemState* state) {
    if (!file || !state) return;

    fprintf(file, "\n%s\n", header);
    fprintf(file, "Total blocks: %u\n", state->lCounts[_NORMAL_BLOCK]);
    fprintf(file, "Total bytes: %u\n", state->lSizes[_NORMAL_BLOCK]);
    fprintf(file, "CRT blocks: %u\n", state->lCounts[_CRT_BLOCK]);
    fprintf(file, "CRT bytes: %u\n", state->lSizes[_CRT_BLOCK]);
    fprintf(file, "Free blocks: %u\n", state->lCounts[_FREE_BLOCK]);
    fprintf(file, "Free bytes: %u\n", state->lSizes[_FREE_BLOCK]);
    fflush(file);
}

FILE* setup_debug_memory_log(const char* node_id) {
    char filename[256];
    snprintf(filename, sizeof(filename), "memory_leaks_%s.txt", node_id);

    FILE* file = NULL;
    if (fopen_s(&file, filename, "w") != 0) {
        return NULL;
    }

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, file);

    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, file);

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, file);

    fprintf(file, "=== MEMORY DEBUG REPORT FOR %s ===\n", node_id);
    fprintf(file, "Log initialized at program start\n\n");
    fflush(file);

    return file;
}

void print_stats_to_file(FILE* file, const _CrtMemState* s1, const _CrtMemState* s2, const _CrtMemState* sDiff, PROCESS_MEMORY_COUNTERS* pmc) {
    if (file) {
        fprintf(file, "\n=== FINAL MEMORY ANALYSIS ===\n");

        if (_CrtMemDifference(sDiff, s1, s2) && file) {
            fprintf(file, "[INFO] Memory leaks detected.\n");
            dump_to_file(file, "[STATISTICS] Initial memory state", s1);
            dump_to_file(file, "[STATISTICS] Final memory state", s2);
            dump_to_file(file, "[STATISTICS] Memory allocation difference", sDiff);
            fprintf(file, "\n[LEAK REPORT] Detailed leak information:\n");
            _CrtMemDumpAllObjectsSince(s1);
        }
        else {
            fprintf(file, "[INFO] No memory leaks detected.\n");
            dump_to_file(file, "[STATISTICS] Initial memory state", s1);
            dump_to_file(file, "[STATISTICS] Final memory state", s2);
            dump_to_file(file, "[STATISTICS] Memory allocation difference", sDiff);
        }

        
        fprintf(file, "\n[SYSTEM] Process memory usage: %lu KB\n", pmc->WorkingSetSize / 1024);
        
        fflush(file);
        fclose(file);
    }
}
#endif