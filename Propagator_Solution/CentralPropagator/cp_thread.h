#ifndef CP_THREAD_H
#define CP_THREAD_H

#include <windows.h>        // for HANDLE
#include <stdint.h>
#include "cp_context.h"
#include "cp_dispatcher.h"

// spawn & join the listener thread
HANDLE cp_start_listener_thread(
    CPContext* ctx,
    uint16_t      port,
    CPDispatcher* dispatcher);

void cp_join_listener_thread(HANDLE hListener);

#endif // CP_THREAD_H
