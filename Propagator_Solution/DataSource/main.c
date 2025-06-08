#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../Common/tsqueue.h"
#include "../Common/Warning.h"
#include "../Common/node.h"
#include "config_loader.h"
#include "data_generator.h"
#include "network_sender.h"
#include "../Common/propagator_client.h"
#include "../Common/utils.h"
#include "cp_launcher.h"

static DWORD WINAPI gen_thread_fn(LPVOID arg) {
    return (DWORD)(uintptr_t)data_generator_thread(arg);
}
static DWORD WINAPI send_thread_fn(LPVOID arg) {
    return (DWORD)(uintptr_t)network_sender_thread(arg);
}

int main(void) {
    NodeInfo* nodes;
    size_t    nodeCount;
    NodeInfo* root = load_root("../Common/nodes.csv", &nodes, &nodeCount);
    if (!root) return EXIT_FAILURE;

    size_t launchCount;
    PROCESS_INFORMATION* procs = cp_launch_all(nodes, nodeCount, &launchCount);
    if (!procs) return EXIT_FAILURE;

    initializeConsoleSettings();
    propagator_client_init(root->address, root->port);
    TSQueue* globalQ = tsqueue_create(0, (void(*)(void*))warning_destroy);

    GeneratorArgs genArgs = { globalQ, nodes, nodeCount };
    SenderArgs   sendArgs = { globalQ };

    HANDLE hGen = CreateThread(NULL, 0, gen_thread_fn, &genArgs, 0, NULL);
    HANDLE hSend = CreateThread(NULL, 0, send_thread_fn, &sendArgs, 0, NULL);

    WaitForSingleObject(hGen, INFINITE);
    WaitForSingleObject(hSend, INFINITE);

    cp_wait_and_cleanup(procs, launchCount);
    tsqueue_destroy(globalQ);
    propagator_client_shutdown();
    node_info_destroy_all(nodes, nodeCount);
    return 0;
}