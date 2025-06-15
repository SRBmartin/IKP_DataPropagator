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

int main(void) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    NodeInfo* nodes;
    size_t nodeCount;
    NodeInfo* root = load_root(NODES_PATH, &nodes, &nodeCount);
    if (!root) return EXIT_FAILURE;

    size_t launchCount;
    PROCESS_INFORMATION* cpProcs = cp_launch_all(nodes, nodeCount, &launchCount);
    if (!cpProcs && launchCount > 0) {
        fprintf(stderr, "Failed to launch CentralPropagator nodes.\n");
        return EXIT_FAILURE;
    }

    size_t ddCount;
    PROCESS_INFORMATION* ddProcs = dd_launch_all(nodes, nodeCount, &ddCount);
    if (!ddProcs && ddCount > 0) {
        fprintf(stderr, "Failed to launch DataDestination nodes.\n");
        return EXIT_FAILURE;
    }

    initializeConsoleSettings();
    TSQueue* globalQ = tsqueue_create(0, (void(*)(void*))warning_destroy);

    GeneratorArgs genArgs = { globalQ, nodes, nodeCount };
    SenderArgs   sendArgs = { globalQ, nodes, nodeCount };

    HANDLE hGen = CreateThread(NULL, 0, gen_thread_fn, &genArgs, 0, NULL);
    HANDLE hSend = CreateThread(NULL, 0, send_thread_fn, &sendArgs, 0, NULL);

    WaitForSingleObject(hGen, INFINITE);
    WaitForSingleObject(hSend, INFINITE);

    cp_wait_and_cleanup(cpProcs, launchCount);
    dd_wait_and_cleanup(ddProcs, ddCount);

    tsqueue_destroy(globalQ);
    node_info_destroy_all(nodes, nodeCount);

    WSACleanup();
    return 0;
}