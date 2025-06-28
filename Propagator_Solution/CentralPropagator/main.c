#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <crtdbg.h>
#endif

#include "../Common/utils.h"
#include "cp_context.h"
#include "cp_listener.h"
#include "cp_thread.h"
#include "cp_dispatcher.h"
#include "cp_shutdown.h"

#define THREAD_POOL_SIZE 4
#define NODES_PATH "../Common/nodes.csv"

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


    fprintf(file, "[DEBUG] Log file initialized.\n");
    fflush(file);

    return file;
}
#endif

int main(int argc, char** argv) {
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_LEAK_CHECK_DF |
        _CRTDBG_CHECK_ALWAYS_DF);
    _CrtMemState s1, s2, sDiff;
    _CrtMemCheckpoint(&s1);
#endif

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Startup of WSA service has failed. Exiting...");
        return EXIT_FAILURE;
    }


    if (argc != 3) {
        fprintf(stderr, "Usage: %s <node_id> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    initializeConsoleSettings();

    const char* root_id = argv[1];
    uint16_t     port = (uint16_t)atoi(argv[2]);

    FILE* logFile = NULL;

#ifdef _DEBUG
    logFile = setup_debug_memory_log(root_id);
#endif
    if (!logFile) {
        printf("Log files failed to open!");
    }

#ifdef _WIN32
    // Inicijalizuj exit event (isto kao kod DD)
    char eventName[128];
    snprintf(eventName, sizeof(eventName), "Global\\CP_Exit_%s", root_id);
    g_exitEvent = CreateEventA(NULL, TRUE, FALSE, eventName);

    if (!g_exitEvent) {
        fprintf(stderr, "Failed to create exit event.\n");
        return EXIT_FAILURE;
    }

    SetConsoleCtrlHandler(console_handler, TRUE);
#endif

    CPContext* ctx = cp_context_create(NODES_PATH, root_id);
    if (!ctx) {
        fprintf(stderr, "Failed to load subtree for %s\n", root_id);
        return EXIT_FAILURE;
    }

    CPDispatcher* dispatcher = cp_dispatcher_create(ctx, THREAD_POOL_SIZE);
    if (!dispatcher) {
        cp_context_destroy(ctx);
        fprintf(stderr, "Failed to start dispatcher\n");
        return EXIT_FAILURE;
    }

    HANDLE hListener = cp_start_listener_thread(ctx, port, dispatcher);
    if (!hListener) {
        cp_dispatcher_shutdown(dispatcher);
        cp_context_destroy(ctx);
        fprintf(stderr, "Failed to start listener on port %hu\n", port);
        return EXIT_FAILURE;
    }

    printf("[CP %s] listening on port %hu\n", root_id, port);
    cp_join_listener_thread(hListener);
    fprintf(logFile, "HELLO.\n");
    fflush(logFile);
    cp_dispatcher_shutdown(dispatcher);
    cp_context_destroy(ctx);
    WSACleanup();

#ifdef _DEBUG
    _CrtMemCheckpoint(&s2);
    if (_CrtMemDifference(&sDiff, &s1, &s2) && logFile) {
        _CrtMemDumpStatistics(&sDiff);
        _CrtDumpMemoryLeaks();
        fprintf(logFile, "[INFO] Memory leaks detected.\n");
    }
    else {
        if (logFile) {
            fprintf(logFile, "[INFO] No memory leaks detected.\n");
        }
    }
    fflush(logFile);
    fclose(logFile);
#endif

#ifdef _WIN32
    if (g_exitEvent) CloseHandle(g_exitEvent);
#endif

    return EXIT_SUCCESS;
}
