#ifndef CP_DISPATCHER_H
#define CP_DISPATCHER_H

#include <stddef.h>
#include "cp_context.h"      // for CPContext
#include "thread_pool.h"     // for ThreadPool
#include "../Common/Warning.h"

/// now define the struct so users see its members:
typedef struct CPDispatcher {
    ThreadPool* pool;
    CPContext* ctx;
} CPDispatcher;

/// create a dispatcher backed by a pool of `num_workers` threads
CPDispatcher* cp_dispatcher_create(CPContext* ctx, size_t num_workers);

/// enqueue a warning for propagation
void          cp_dispatcher_submit(CPDispatcher* d, Warning* w);

/// gracefully shut everything down (blocks until done)
void          cp_dispatcher_shutdown(CPDispatcher* d);

#endif // CP_DISPATCHER_H
