#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <io.h> 
#endif


#include "data_destination.h"
#include "../Common/utils.h"
#include "../CentralPropagator/cp_shutdown.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
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
    SetConsoleCtrlHandler(console_handler, TRUE);
#endif

    printf("[DD %s] listening on port %hu\n", id, port);

#ifdef _WIN32
    WaitForSingleObject(g_exitEvent, INFINITE);
#else
    getchar();
#endif

    dd_destroy(dd);

    _CrtMemCheckpoint(&s2);
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

    print_stats_to_file(logFile, &s1, &s2, &sDiff, &pmc);

    if (logFile) {
        fclose(logFile);
    }

#ifdef _WIN32
    if (g_exitEvent) CloseHandle(g_exitEvent);
#endif

    return 0;
}