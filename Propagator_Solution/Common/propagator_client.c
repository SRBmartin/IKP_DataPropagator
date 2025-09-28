#include "propagator_client.h"
#include "propagator_connections.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")

static uint64_t host_to_network64(uint64_t x) {
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)x;
    hi = htonl(hi);
    lo = htonl(lo);
    return ((uint64_t)lo << 32) | hi;
}

bool send_warning_to(const char* address, uint16_t port, const Warning* w) {
    SOCKET s = get_or_create_connection(address, port);
    if (s == INVALID_SOCKET) {
        return false;
    }

    uint32_t city_len = (uint32_t)strlen(w->city);
    uint32_t dest_len = (uint32_t)strlen(w->dest_node);
    size_t buf_len = 4 + city_len + 4 + dest_len + 4 + 8 + 8;

    char* buf = malloc(buf_len);
    if (!buf) {
        return false;
    }

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

    int sent = send(s, buf, (int)buf_len, 0);
    free(buf);

    if (sent != (int)buf_len) {
        printf("[ERROR] Send failed to %s:%hu with code: %d\n", address, port, WSAGetLastError());
        close_connection(address, port); 
        return false;
    }

    return true;
}