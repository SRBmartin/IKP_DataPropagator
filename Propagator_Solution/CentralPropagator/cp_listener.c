#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cp_shutdown.h"
#include "cp_listener.h"
#include "../Common/warning.h"
#include "../Common/utils.h"

static int recv_all(SOCKET s, char* buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = recv(s, buf + total, n - total, 0);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

static Warning* deserialize_warning(SOCKET s) {
    uint32_t len32;
    if (recv_all(s, (char*)&len32, 4) != 4) return NULL;
    uint32_t city_len = ntohl(len32);

    char* city = malloc(city_len + 1);
    if (!city) return NULL;
    if (recv_all(s, city, city_len) != (int)city_len) {
        free(city);
        return NULL;
    }
    city[city_len] = '\0';

    if (recv_all(s, (char*)&len32, 4) != 4) { free(city); return NULL; }
    uint32_t dest_len = ntohl(len32);

    char* dest = malloc(dest_len + 1);
    if (!dest) { free(city); return NULL; }
    if (recv_all(s, dest, dest_len) != (int)dest_len) {
        free(city); free(dest);
        return NULL;
    }
    dest[dest_len] = '\0';

    if (recv_all(s, (char*)&len32, 4) != 4) {
        free(city); free(dest);
        return NULL;
    }
    WarningType type = (WarningType)ntohl(len32);

    uint64_t vbits;
    if (recv_all(s, (char*)&vbits, 8) != 8) {
        free(city); free(dest);
        return NULL;
    }
    vbits = network_to_host64(vbits);
    double value; memcpy(&value, &vbits, sizeof(value));

    uint64_t ts;
    if (recv_all(s, (char*)&ts, 8) != 8) {
        free(city); free(dest);
        return NULL;
    }
    ts = network_to_host64(ts);

    Warning* w = warning_create(city, type, value, ts, dest);
    free(city);
    free(dest);
    return w;
}

void cp_listener_run(const CPContext* ctx, uint16_t port, CPDispatcher* dispatcher) {
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in sa = { .sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_ANY), .sin_port = htons(port) };
    bind(listen_sock, (struct sockaddr*)&sa, sizeof(sa));
    listen(listen_sock, SOMAXCONN);

    for (;;) {
#ifdef _WIN32
        if (WaitForSingleObject(g_exitEvent, 0) == WAIT_OBJECT_0) break;
#endif
        SOCKET client = accept(listen_sock, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        int opt = 1;
        setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, sizeof(opt));

        for (;;) {
            Warning* w = deserialize_warning(client);
            if (!w) break; 

            char* desc = warning_to_string(w);
            if (desc) {
                printf("[CP %s] Received: %s\n", ctx->me->id, desc);
                free(desc);
            }

            cp_dispatcher_submit(dispatcher, w);
        }

        closesocket(client);
    }

    closesocket(listen_sock);
}
