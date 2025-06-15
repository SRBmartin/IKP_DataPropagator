#ifndef CP_DISPATCHER_H
#define CP_DISPATCHER_H

#include <stddef.h>
#include "cp_context.h"
#include "thread_pool.h"
#include "../Common/Warning.h"

typedef struct CPDispatcher {
    ThreadPool* pool;
    CPContext* ctx;
} CPDispatcher;

CPDispatcher* cp_dispatcher_create(CPContext* ctx, size_t num_workers);
void          cp_dispatcher_submit(CPDispatcher* d, Warning* w);
void          cp_dispatcher_shutdown(CPDispatcher* d);

#endif
