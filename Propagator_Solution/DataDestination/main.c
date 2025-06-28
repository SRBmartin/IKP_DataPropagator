#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <io.h>  // For _dup, _dup2, _fileno
#endif


#include "data_destination.h"
#include "../Common/utils.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

static HANDLE g_exitEvent = NULL;

#ifdef _WIN32
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_CLOSE_EVENT || signal == CTRL_C_EVENT) {
        if (g_exitEvent) {
            SetEvent(g_exitEvent);
        }
        return TRUE;
    }
    return FALSE;
}
#endif

#if _DEBUG
//int custom_report_hook(int reportType, char* message, int* returnValue) {
//    FILE* logFile = (FILE*)_CrtGetReportFile(_CRT_WARN);
//    if (logFile) {
//        fprintf(logFile, "%s", message);
//        fflush(logFile);
//    }
//    return TRUE;
//}

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
#endif

#ifdef _DEBUG
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

    //_CrtSetReportHook(custom_report_hook);

    fprintf(file, "=== MEMORY DEBUG REPORT FOR %s ===\n", node_id);
    fprintf(file, "Log initialized at program start\n\n");
    fflush(file);

    return file;
}
#endif

int main(int argc, char** argv) {

#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#endif
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_LEAK_CHECK_DF |
        _CRTDBG_CHECK_ALWAYS_DF);
    _CrtMemState s1, s2, sDiff;
    _CrtMemCheckpoint(&s1);
#endif

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <node-id> <port>\n", argv[0]);
        return 1;
    }

    const char* id = argv[1];
    uint16_t    port = (uint16_t)atoi(argv[2]);

    FILE* logFile = NULL;

#ifdef _DEBUG
    logFile = setup_debug_memory_log(id);
#endif
    if (!logFile) {
        printf("Log files failed to open!");
    }

    initializeConsoleSettings();

    DDContext* dd = dd_create(id, port);
    if (!dd) {
        fprintf(stderr, "Failed to start DataDestination %s\n", id);
        if (logFile) fclose(logFile);
        return 1;
    }
#ifdef _WIN32
    char eventName[128];
    snprintf(eventName, sizeof(eventName), "Global\\DD_Exit_%s", id);
    g_exitEvent = CreateEventA(NULL, TRUE, FALSE, eventName);

    if (!g_exitEvent) {
        fprintf(stderr, "Failed to create exit event.\n");
        dd_destroy(dd);
        return 1;
    }
#ifdef _DEBUG
    void* test_leak = malloc(100); // Intentional leak
#endif
    SetConsoleCtrlHandler(console_handler, TRUE);
#endif

    printf("[DD %s] listening on port %hu\n", id, port);
#ifdef _WIN32
    WaitForSingleObject(g_exitEvent, INFINITE);
#else
    getchar();
#endif
    //printf("Press any key to exit and stop the node...\n");
    //getchar();
    dd_destroy(dd);
#ifdef _DEBUG
    if (logFile) {
        fprintf(logFile, "\n=== FINAL MEMORY ANALYSIS ===\n");

        _CrtMemCheckpoint(&s2);
        if (_CrtMemDifference(&sDiff, &s1, &s2) && logFile) {
            fprintf(logFile, "[INFO] Memory leaks detected.\n");
            dump_to_file(logFile, "[STATISTICS] Initial memory state", &s1);
            dump_to_file(logFile, "[STATISTICS] Final memory state", &s2);
            dump_to_file(logFile, "[STATISTICS] Memory allocation difference", &sDiff);
            fprintf(logFile, "\n[LEAK REPORT] Detailed leak information:\n");
            _CrtMemDumpAllObjectsSince(&s1);
        }
        else {
            fprintf(logFile, "[INFO] No memory leaks detected.\n");
            dump_to_file(logFile, "[STATISTICS] Initial memory state", &s1);
            dump_to_file(logFile, "[STATISTICS] Final memory state", &s2);
            dump_to_file(logFile, "[STATISTICS] Memory allocation difference", &sDiff);
        }

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            fprintf(logFile, "\n[SYSTEM] Process memory usage: %lu KB\n",
                pmc.WorkingSetSize / 1024);
        }
        fflush(logFile);
        fclose(logFile);
    }
#endif
#ifdef _WIN32
    if (g_exitEvent) CloseHandle(g_exitEvent);
#endif

    return 0;
}