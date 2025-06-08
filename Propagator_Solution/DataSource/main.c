#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include "../Common/tsqueue.h"
#include "../Common/Warning.h"
#include "../Common/node.h"
#include "config_loader.h"
#include "data_generator.h"
#include "network_sender.h"
#include "../Common/propagator_client.h"

static DWORD WINAPI gen_thread_fn(LPVOID arg) {
    data_generator_thread(arg);
    return 0;
}

static DWORD WINAPI send_thread_fn(LPVOID arg) {
    network_sender_thread(arg);
    return 0;
}

void initializeConsoleSettings();

int main(void) {
    NodeInfo* nodes;
    size_t    nodeCount;
    NodeInfo* root = load_root("nodes.csv", &nodes, &nodeCount);

    if (!root) {
        printf("[ERROR]: nodes.csv file could not be found.");
		return EXIT_FAILURE;
    }

    initializeConsoleSettings();

    propagator_client_init(root->address, root->port);

    TSQueue* globalQ = tsqueue_create(0, (void(*)(void*))warning_destroy);

    // launch CentralPropagator nodes here

    GeneratorArgs genArgs = { globalQ, nodes, nodeCount };
    SenderArgs    sendArgs = { globalQ };

    HANDLE hGen = CreateThread(NULL, 0, gen_thread_fn, &genArgs, 0, NULL);
    HANDLE hSend = CreateThread(NULL, 0, send_thread_fn, &sendArgs, 0, NULL);

    WaitForSingleObject(hGen, INFINITE);
    WaitForSingleObject(hSend, INFINITE);

    CloseHandle(hGen);
    CloseHandle(hSend);

    tsqueue_destroy(globalQ);
    propagator_client_shutdown();
    node_info_destroy_all(nodes, nodeCount);
    return 0;
}

void initializeConsoleSettings() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}