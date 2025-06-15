#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../Common/utils.h"
#include "cp_context.h"
#include "cp_listener.h"
#include "cp_thread.h"
#include "cp_dispatcher.h"

#define THREAD_POOL_SIZE 4
#define NODES_PATH "../Common/nodes.csv"

int main(int argc, char** argv) {
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

    cp_dispatcher_shutdown(dispatcher);
    cp_context_destroy(ctx);
    WSACleanup();
    return EXIT_SUCCESS;
}
