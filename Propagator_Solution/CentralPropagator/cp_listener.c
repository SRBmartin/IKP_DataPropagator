#include "cp_listener.h"
#include <stdio.h>
#include "../Common/Warning.h"
#include "../Common/propagator_client.h"
#include "../Common/node.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma comment(lib, "Ws2_32.lib")

static inline uint64_t network_to_host64(uint64_t x) {
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)x;
    hi = ntohl(hi);
    lo = ntohl(lo);
    return ((uint64_t)lo << 32) | hi;
}

static int recv_all(SOCKET s, char* buf, int n) {
    int t = 0;
    while (t < n) {
        int r = recv(s, buf + t, n - t, 0);
        if (r <= 0) return r;
        t += r;
    }
    return t;
}

static Warning* deserialize_warning(SOCKET s) {
    uint32_t L;
    if (recv_all(s, (char*)&L, 4) != 4) return NULL;
    uint32_t city_len = ntohl(L);
    char* city = malloc(city_len + 1);
    if (!city) return NULL;
    if (recv_all(s, city, city_len) != (int)city_len) { free(city);return NULL; }
    city[city_len] = 0;

    if (recv_all(s, (char*)&L, 4) != 4) { free(city);return NULL; }
    uint32_t dest_len = ntohl(L);
    char* dest = malloc(dest_len + 1);
    if (!dest) { free(city);return NULL; }
    if (recv_all(s, dest, dest_len) != (int)dest_len) { free(city);free(dest);return NULL; }
    dest[dest_len] = 0;

    if (recv_all(s, (char*)&L, 4) != 4) { free(city);free(dest);return NULL; }
    WarningType type = (WarningType)ntohl(L);

    uint64_t v;
    if (recv_all(s, (char*)&v, 8) != 8) { free(city);free(dest);return NULL; }
    v = network_to_host64(v);
    double value; memcpy(&value, &v, 8);

    uint64_t ts;
    if (recv_all(s, (char*)&ts, 8) != 8) { free(city);free(dest);return NULL; }
    ts = network_to_host64(ts);

    Warning* w = warning_create(city, type, value, ts, dest);
    free(city); free(dest);
    return w;
}

void cp_listener_run(const CPContext* ctx, uint16_t port) {
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sa = { 0 };
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);
    bind(listen_sock, (struct sockaddr*)&sa, sizeof(sa));
    listen(listen_sock, SOMAXCONN);
    while (1) {
        SOCKET client = accept(listen_sock, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        Warning* w = deserialize_warning(client);
        closesocket(client);
        if (!w) continue;

        // print locally
        char* desc = warning_to_string(w);
        if (desc) {
            printf("[CP %s]: recv %s\n", ctx->me->id, desc);
            free(desc);
        }

        // look up the destination in our subtree map
        NodeInfo* destInfo = hashmap_get(ctx->map, w->dest_node);
        if (destInfo && destInfo != ctx->me) {
            // climb up until parent == me
            NodeInfo* hop = destInfo;
            while (hop->parent && hop->parent != ctx->me) {
                hop = hop->parent;
            }
            // now hop is the immediate child of me on the path to dest
            if (propagator_client_init(hop->address, hop->port)) {
                //propagate_warning(w);
                propagator_client_shutdown();
            }
        }
        // if destInfo==me (we are the final destination), we stop here

        warning_destroy(w);
    }

    closesocket(listen_sock);
    WSACleanup();
}