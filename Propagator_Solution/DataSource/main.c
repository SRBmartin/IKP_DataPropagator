#ifdef _DEBUG
#define WIN32_LEAN_AND_MEAN
#define _CRTDBG_MAP_ALLOC

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#endif
#pragma comment(lib, "User32.lib")
#include "../Common/tsqueue.h"
#include "../Common/warning.h"
#include "../Common/node.h"
#include "config_loader.h"
#include "data_generator.h"
#include "network_sender.h"
#include "../Common/propagator_client.h"
#include "../Common/utils.h"
#include "cp_launcher.h"

#define NODES_PATH "../Common/nodes.csv"

static DWORD WINAPI gen_thread_fn(LPVOID arg) {
    return (DWORD)(uintptr_t)data_generator_thread(arg);
}
static DWORD WINAPI send_thread_fn(LPVOID arg) {
    return (DWORD)(uintptr_t)network_sender_thread(arg);
}

//This is only partial awaiter, it's not always gonna await fully since we don't have the full control over the external processes.
static void await_cp_and_dd_propagation_objects(int cpCount, int ddCount, PROCESS_INFORMATION* cpProcs, PROCESS_INFORMATION* ddProcs) {
    if (cpCount > 0) {
        HANDLE* handles = malloc(cpCount * sizeof(HANDLE));
        for (size_t i = 0; i < cpCount; i++) {
            handles[i] = cpProcs[i].hProcess;
        }

        WaitForMultipleObjects(
            (DWORD)cpCount,
            handles,
            FALSE,
            2000
        );

        free(handles);
    }

    if (ddCount > 0) {
        HANDLE* handles = malloc(ddCount * sizeof(HANDLE));
        for (size_t i = 0; i < ddCount; i++) {
            handles[i] = ddProcs[i].hProcess;
        }

        WaitForMultipleObjects(
            (DWORD)ddCount,
            handles,
            FALSE,
            2000
        );

        free(handles);
    }
}

static BOOL CALLBACK SendWmCloseToWindow(HWND hWnd, LPARAM lParam) {
    DWORD winPID;
    GetWindowThreadProcessId(hWnd, &winPID);
    if ((DWORD)lParam == winPID) {
        PostMessage(hWnd, WM_CLOSE, 0, 0); 
        return FALSE;
    }
    return TRUE;
}

static void gracefully_close_all(PROCESS_INFORMATION* procs, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        DWORD pid = procs[i].dwProcessId;

        BOOL found = EnumWindows(SendWmCloseToWindow, (LPARAM)pid);
        if (!found) {
            TerminateProcess(procs[i].hProcess, 0);
        }
    }
}


int main(void) {
    getchar();

#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_LEAK_CHECK_DF |
        _CRTDBG_CHECK_ALWAYS_DF);
    _CrtMemState s1, s2, sDiff;
    _CrtMemCheckpoint(&s1);
#endif

    FILE* logFile = NULL;

#ifdef _DEBUG
    logFile = setup_debug_memory_log("DataSource");
#endif
    if (!logFile) {
        printf("Log files failed to open!");
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to start WSA services. Exiting...");
        return EXIT_FAILURE;
    }

    NodeInfo* nodes;
    size_t nodeCount;
    NodeInfo* root = load_root(NODES_PATH, &nodes, &nodeCount);
    if (!root) return EXIT_FAILURE;

    size_t cpCount;
    PROCESS_INFORMATION* cpProcs = cp_launch_all(nodes, nodeCount, &cpCount);
    if (!cpProcs && cpCount > 0) {
        fprintf(stderr, "Failed to launch CentralPropagator nodes.\n");
        return EXIT_FAILURE;
    }

    size_t ddCount;
    PROCESS_INFORMATION* ddProcs = dd_launch_all(nodes, nodeCount, &ddCount);
    if (!ddProcs && ddCount > 0) {
        fprintf(stderr, "Failed to launch DataDestination nodes.\n");
        return EXIT_FAILURE;
    }

    await_cp_and_dd_propagation_objects(cpCount, ddCount, cpProcs, ddProcs);

    initializeConsoleSettings();
    TSQueue* globalQ = tsqueue_create(0, (void(*)(void*))warning_destroy);

    HANDLE hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hStop) {
        fprintf(stderr, "Failed to create a stop event. Exiting...");
        return EXIT_FAILURE;
    }

    GeneratorArgs genArgs = { globalQ, nodes, nodeCount };
    SenderArgs   sendArgs = { globalQ, nodes, nodeCount };
    genArgs.stopEvent = hStop;
    sendArgs.stopEvent = hStop;

    HANDLE hGen = CreateThread(NULL, 0, gen_thread_fn, &genArgs, 0, NULL);
    HANDLE hSend = CreateThread(NULL, 0, send_thread_fn, &sendArgs, 0, NULL);

    printf("[INFO]: To stop press Enter.\n");

    getchar();

    SetEvent(hStop);
    // Signal named events for DataDestination processes
    for (size_t i = 0; i < nodeCount; i++) {
        if (nodes[i].type == NODE_DESTINATION) {
            char eventName[128];
            snprintf(eventName, sizeof(eventName), "Global\\DD_Exit_%s", nodes[i].id);
            HANDLE hExit = OpenEventA(EVENT_MODIFY_STATE, FALSE, eventName);
            if (hExit) {
                SetEvent(hExit);
                CloseHandle(hExit);
                printf("[INFO] Signaled shutdown to DataDestination node %s\n", nodes[i].id);
            }
            else {
                printf("[WARN] Could not open exit event for node %s\n", nodes[i].id);
            }
        }
    }

    // Signal named events for CentralPropagator processes
    for (size_t i = 0; i < nodeCount; i++) {
        if (nodes[i].type != NODE_DESTINATION) { // CP nodes
            char eventName[128];
            snprintf(eventName, sizeof(eventName), "Global\\CP_Exit_%s", nodes[i].id);
            HANDLE hExit = OpenEventA(EVENT_MODIFY_STATE, FALSE, eventName);
            if (hExit) {
                SetEvent(hExit);
                CloseHandle(hExit);
                printf("[INFO] Signaled shutdown to CentralPropagator node %s\n", nodes[i].id);
            }
            else {
                printf("[WARN] Could not open exit event for CP node %s\n", nodes[i].id);
            }
        }
    }


    WaitForSingleObject(hGen, INFINITE);
    WaitForSingleObject(hSend, INFINITE);

    gracefully_close_all(cpProcs, cpCount);
    gracefully_close_all(ddProcs, ddCount);

    cp_terminate_all();

    CloseHandle(hGen);
    CloseHandle(hSend);
    CloseHandle(hStop);

    tsqueue_destroy(globalQ);
    node_info_destroy_all(nodes, nodeCount);
    WSACleanup();

    printf("[INFO] Cleaned up and safe to exit.");
    getchar();

    _CrtMemCheckpoint(&s2);
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

    print_stats_to_file(logFile, &s1, &s2, &sDiff, &pmc);

    if (logFile) {
        fclose(logFile);
    }

    return EXIT_SUCCESS;
}