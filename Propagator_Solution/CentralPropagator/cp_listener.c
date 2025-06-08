#include "cp_listener.h"
#include "../Common/propagator_client.h"
#include "../Common/warning.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

void cp_listener_run(const CPContext* ctx,
    uint16_t port)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listen_sock = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);

    bind(listen_sock,
        (struct sockaddr*)&sa,
        sizeof(sa));
    listen(listen_sock, SOMAXCONN);

    while (1) {
        SOCKET client = accept(listen_sock, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        Warning w;
        int got = recv(client,
            (char*)&w,
            sizeof(w),
            0);
        if (got == sizeof(w)) {
            for (size_t i = 0; i < ctx->child_count; i++) {
                NodeInfo* c = ctx->children[i];
                propagator_client_init(c->address,
                    c->port);
                propagate_warning(&w);
                propagator_client_shutdown();
            }
        }
        closesocket(client);
    }

    closesocket(listen_sock);
    WSACleanup();
}