#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../Common/utils.h"
#include "cp_context.h"
#include "cp_listener.h"
#include "cp_thread.h"

int main(int argc, char** argv) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    if (argc != 3) {
        fprintf(stderr, "[USAGE]: %s <node_id> <port>\n", argv[0]);
        return 1;
    }

    initializeConsoleSettings();

    const char* root_id = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);

    CPContext* ctx = cp_context_create("../Common/nodes.csv", root_id);
    if (!ctx) {
        fprintf(stderr, "Failed to load subtree for %s\n", root_id);
        getchar();
        return 1;
    }

    HANDLE hListener = cp_start_listener_thread(ctx, port);
    if (!hListener) {
        fprintf(stderr, "[CP %s] failed to start listener on port %hu\n",
            root_id, port);
        cp_context_destroy(ctx);
        return 1;
    }
    
    printf("[CP %s] listening on port %hu\n", root_id, port);

    cp_join_listener_thread(hListener);
    cp_context_destroy(ctx);

    WSACleanup();
    return 0;
}