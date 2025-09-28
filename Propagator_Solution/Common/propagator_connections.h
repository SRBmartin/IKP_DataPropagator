#pragma once
#ifndef PROPAGATOR_CONNECTIONS_H
#define PROPAGATOR_CONNECTIONS_H

#include <winsock2.h>
#include <stdint.h>
#include "hashmap.h"
#include <windows.h> 

typedef struct {
    char* address;
    uint16_t port;
} ConnectionKey;

extern HashMap* connections_map;
extern CRITICAL_SECTION conn_mutex;

void connections_init();
SOCKET get_or_create_connection(const char* address, uint16_t port);
void close_connection(const char* address, uint16_t port);
void connections_cleanup();

#endif