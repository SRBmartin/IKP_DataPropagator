#include <windows.h>
#include <stdlib.h>

#include "cp_thread.h"
#include "cp_listener.h"

typedef struct {
    CPContext* ctx;
    uint16_t      port;
    CPDispatcher* dispatcher;
} ListenerArgs;

static DWORD WINAPI listener_fn(LPVOID arg) {
    ListenerArgs* args = arg;
    cp_listener_run(args->ctx, args->port, args->dispatcher);
    free(args);
    return 0;
}

HANDLE cp_start_listener_thread(
    CPContext* ctx,
    uint16_t      port,
    CPDispatcher* dispatcher)
{
    ListenerArgs* args = malloc(sizeof(*args));
    if (!args) return NULL;
    args->ctx = ctx;
    args->port = port;
    args->dispatcher = dispatcher;
    return CreateThread(NULL, 0, listener_fn, args, 0, NULL);
}

void cp_join_listener_thread(HANDLE hListener) {
    if (!hListener) return;
    WaitForSingleObject(hListener, INFINITE);
    CloseHandle(hListener);
}
