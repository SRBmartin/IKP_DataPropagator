#include "propagator_connections.h"
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>
#include <stdio.h>

HashMap* connections_map = NULL;
CRITICAL_SECTION conn_mutex;

static size_t hash_conn_key(const void* k) {
    const ConnectionKey* key = (const ConnectionKey*)k;
    size_t h = 5381;
    const char* s = key->address;
    while (*s) h = ((h << 5) + h) + *s++;
    h = ((h << 5) + h) + key->port;
    return h;
}

static bool eq_conn_key(const void* a, const void* b) {
    const ConnectionKey* ka = (const ConnectionKey*)a;
    const ConnectionKey* kb = (const ConnectionKey*)b;
    return strcmp(ka->address, kb->address) == 0 && ka->port == kb->port;
}

static void free_conn_key(void* k) {
    ConnectionKey* key = (ConnectionKey*)k;
    free(key->address);
    free(key);
}

static void free_socket(void* v) {
    SOCKET* s_ptr = (SOCKET*)v;
    if (s_ptr && *s_ptr != INVALID_SOCKET) {
        closesocket(*s_ptr);
        free(s_ptr); 
    }
}

void connections_init() {
    connections_map = hashmap_create(16, hash_conn_key, eq_conn_key, free_conn_key, free_socket);
    if (!connections_map) {
        printf("[ERROR] Failed to initialize connections_map\n");
        return;
    }
    InitializeCriticalSection(&conn_mutex);
    printf("[INFO] Connections initialized\n");
}

SOCKET get_or_create_connection(const char* address, uint16_t port) {
    if (!connections_map) {
        printf("[ERROR] connections_map not initialized\n");
        return INVALID_SOCKET;
    }

    EnterCriticalSection(&conn_mutex);

    ConnectionKey* temp_key = malloc(sizeof(ConnectionKey));
    if (!temp_key) {
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to allocate temp_key\n");
        return INVALID_SOCKET;
    }
    temp_key->address = _strdup(address);
    if (!temp_key->address) {
        free(temp_key);
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to allocate temp_key->address\n");
        return INVALID_SOCKET;
    }
    temp_key->port = port;

    SOCKET* existing = (SOCKET*)hashmap_get(connections_map, temp_key);
    if (existing && *existing != INVALID_SOCKET) {
        SOCKET s = *existing;
        free(temp_key->address);
        free(temp_key);
        LeaveCriticalSection(&conn_mutex);
        printf("[INFO] Reusing connection to %s:%hu\n", address, port);
        return s;
    }

    free(temp_key->address);
    free(temp_key);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to create socket: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*)&opt, sizeof(opt));

    struct sockaddr_in sa = { 0 };
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    InetPtonA(AF_INET, address, &sa.sin_addr);

    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
        int err = WSAGetLastError();
        closesocket(s);
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Connect failed to %s:%hu with code: %d\n", address, port, err);
        return INVALID_SOCKET;
    }

    ConnectionKey* new_key = malloc(sizeof(ConnectionKey));
    if (!new_key) {
        closesocket(s);
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to allocate new_key\n");
        return INVALID_SOCKET;
    }
    new_key->address = _strdup(address);
    if (!new_key->address) {
        free(new_key);
        closesocket(s);
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to allocate new_key->address\n");
        return INVALID_SOCKET;
    }
    new_key->port = port;

    SOCKET* s_ptr = malloc(sizeof(SOCKET));
    if (!s_ptr) {
        free(new_key->address);
        free(new_key);
        closesocket(s);
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to allocate s_ptr\n");
        return INVALID_SOCKET;
    }
    *s_ptr = s;

    if (hashmap_put(connections_map, new_key, s_ptr) != 0) {
        free(new_key->address);
        free(new_key);
        free(s_ptr);
        closesocket(s);
        LeaveCriticalSection(&conn_mutex);
        printf("[ERROR] Failed to put socket in hashmap\n");
        return INVALID_SOCKET;
    }

    printf("[INFO] Created new connection to %s:%hu\n", address, port);
    LeaveCriticalSection(&conn_mutex);
    return s;
}

void close_connection(const char* address, uint16_t port) {
    EnterCriticalSection(&conn_mutex);

    ConnectionKey key = { (char*)address, port };
    hashmap_remove(connections_map, &key);

    LeaveCriticalSection(&conn_mutex);
    printf("[INFO] Closed connection to %s:%hu\n", address, port);
}

void connections_cleanup() {
    if (connections_map) {
        hashmap_destroy(connections_map);
        connections_map = NULL;
    }
    DeleteCriticalSection(&conn_mutex);
    printf("[INFO] Connections cleaned up\n");
}