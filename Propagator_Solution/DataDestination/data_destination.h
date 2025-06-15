#ifndef DATA_DESTINATION_H
#define DATA_DESTINATION_H

#include <winsock2.h>
#include <windows.h>
#include "../Common/tsqueue.h"
#include "../Common/warning.h"

typedef struct {
    char* id;
    uint16_t  port;
    SOCKET    listen_sock;
    TSQueue* queue;
    HANDLE    listener_thread;
    HANDLE    processor_thread;
} DDContext;

DDContext* dd_create(const char* id, uint16_t port);
void dd_destroy(DDContext* ctx);

#endif