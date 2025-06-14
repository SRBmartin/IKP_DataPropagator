#include "propagator_client.h"
#include "warning.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

static SOCKET g_sock = INVALID_SOCKET;

static inline uint64_t host_to_network64(uint64_t x) {
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)x;
    hi = htonl(hi);
    lo = htonl(lo);
    return ((uint64_t)lo << 32) | hi;
}

bool propagator_client_init(const char* address, uint16_t port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
    g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_sock == INVALID_SOCKET) return false;
    struct sockaddr_in sa = { 0 };
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    InetPtonA(AF_INET, address, &sa.sin_addr);
    if (connect(g_sock, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        return false;
    }
    return true;
}

bool propagate_warning(const Warning* w) {
    uint32_t city_len = (uint32_t)strlen(w->city);
    uint32_t dest_len = (uint32_t)strlen(w->dest_node);
    size_t buf_len = 4 + city_len + 4 + dest_len + 4 + 8 + 8;
    char* buf = malloc(buf_len);
    if (!buf) return false;
    char* p = buf;
    uint32_t tmp32;
    tmp32 = htonl(city_len);
    memcpy(p, &tmp32, 4); p += 4;
    memcpy(p, w->city, city_len); p += city_len;
    tmp32 = htonl(dest_len);
    memcpy(p, &tmp32, 4); p += 4;
    memcpy(p, w->dest_node, dest_len); p += dest_len;
    tmp32 = htonl((uint32_t)w->type);
    memcpy(p, &tmp32, 4); p += 4;
    uint64_t vbits;
    memcpy(&vbits, &w->value, 8);
    vbits = host_to_network64(vbits);
    memcpy(p, &vbits, 8); p += 8;
    uint64_t tbits = host_to_network64(w->timestamp);
    memcpy(p, &tbits, 8);
    int sent = send(g_sock, buf, (int)buf_len, 0);
    free(buf);
    return sent == (int)buf_len;
}

void propagator_client_shutdown(void) {
    if (g_sock != INVALID_SOCKET) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
}