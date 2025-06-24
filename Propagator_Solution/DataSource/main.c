#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(void) {
    getchar();
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

    WaitForSingleObject(hGen, INFINITE);
    WaitForSingleObject(hSend, INFINITE);

    cp_terminate_all();

    CloseHandle(hGen);
    CloseHandle(hSend);
    CloseHandle(hStop);

    tsqueue_destroy(globalQ);
    node_info_destroy_all(nodes, nodeCount);
    WSACleanup();

    printf("[INFO] Cleaned up and safe to exit.");
    getchar();

    return EXIT_SUCCESS;
}