#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include "data_destination.h"
#include "../Common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

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
        free(city);
        free(dest);
        return NULL;
    }
    dest[dest_len] = '\0';

    if (recv_all(s, (char*)&len32, 4) != 4) {
        free(city);
        free(dest);
        return NULL;
    }
    WarningType type = (WarningType)ntohl(len32);

    uint64_t vbits;
    if (recv_all(s, (char*)&vbits, 8) != 8) {
        free(city);
        free(dest);
        return NULL;
    }
    vbits = network_to_host64(vbits);
    double value; memcpy(&value, &vbits, sizeof(value));

    uint64_t ts_bits;
    if (recv_all(s, (char*)&ts_bits, 8) != 8) {
        free(city);
        free(dest);
        return NULL;
    }
    ts_bits = network_to_host64(ts_bits);

    Warning* w = warning_create(city, type, value, ts_bits, dest);
    free(city);
    free(dest);
    return w;
}

static DWORD WINAPI listener_fn(LPVOID arg) {
    DDContext* ctx = (DDContext*)arg;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    ctx->listen_sock = s;
    struct sockaddr_in sa = { .sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_ANY), .sin_port = htons(ctx->port) };
    bind(s, (struct sockaddr*)&sa, sizeof(sa));
    listen(s, SOMAXCONN);

    while (1) {
        SOCKET client = accept(s, NULL, NULL);
        if (client == INVALID_SOCKET) break;

        int opt = 1;
        setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, sizeof(opt));

        for (;;) {
            Warning* w = deserialize_warning(client);
            if (!w) break;

            tsqueue_enqueue(ctx->queue, w);
        }

        closesocket(client);
    }
    return 0;
}

//static DWORD WINAPI listener_fn(LPVOID arg) {
//    DDContext* ctx = (DDContext*)arg;
//
//    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
//    ctx->listen_sock = s;
//    struct sockaddr_in sa = {
//        .sin_family = AF_INET,
//        .sin_addr.s_addr = htonl(INADDR_ANY),
//        .sin_port = htons(ctx->port)
//    };
//    bind(s, (struct sockaddr*)&sa, sizeof(sa));
//    listen(s, SOMAXCONN);
//
//    while (1) {
//        SOCKET client = accept(s, NULL, NULL);
//        if (client == INVALID_SOCKET) break;
//
//        Warning* w = deserialize_warning(client);
//        closesocket(client);
//        if (w) {
//            tsqueue_enqueue(ctx->queue, w);
//        }
//    }
//    return 0;
//}

static DWORD WINAPI processor_fn(LPVOID arg) {
    DDContext* ctx = (DDContext*)arg;
    for (;;) {
        Warning* w = tsqueue_dequeue(ctx->queue);
        if (!w) break;  // sentinel
        char* desc = warning_to_string(w);
        if (desc) {
            printf("[DD %s] %s\n", ctx->id, desc);
            free(desc);
        }
        warning_destroy(w);
    }
    return 0;
}

DDContext* dd_create(const char* id, uint16_t port) {
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);

    DDContext* ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        WSACleanup();
        return NULL;
    }
    ctx->id = _strdup(id);
    ctx->port = port;
    ctx->queue = tsqueue_create(0, (void(*)(void*))warning_destroy);
    if (!ctx->queue) {
        free(ctx->id);
        free(ctx);
        WSACleanup();
        return NULL;
    }

    ctx->listener_thread = CreateThread(
        NULL, 0, listener_fn, ctx, 0, NULL
    );

    if (!ctx->listener_thread) {
        tsqueue_destroy(ctx->queue);
        free(ctx->id);
        free(ctx);
        WSACleanup();
        return NULL;
    }

    ctx->processor_thread = CreateThread(
        NULL, 0, processor_fn, ctx, 0, NULL
    );

    if (!ctx->processor_thread) {
        if (ctx->listen_sock != INVALID_SOCKET) {
            closesocket(ctx->listen_sock);
        }
        CloseHandle(ctx->listener_thread); 
        tsqueue_destroy(ctx->queue);
        free(ctx->id);
        free(ctx);
        WSACleanup();
        return NULL;
    }

    return ctx;
}

void dd_destroy(DDContext* ctx) {
    if (!ctx) return;

    closesocket(ctx->listen_sock);
    WaitForSingleObject(ctx->listener_thread, INFINITE);
    CloseHandle(ctx->listener_thread);

    tsqueue_enqueue(ctx->queue, NULL);
    WaitForSingleObject(ctx->processor_thread, INFINITE);
    CloseHandle(ctx->processor_thread);

    tsqueue_destroy(ctx->queue);
    free(ctx->id);
    WSACleanup();
    free(ctx);
}