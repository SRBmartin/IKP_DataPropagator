#include <stdio.h>
#include <stdlib.h>

#include "cp_dispatcher.h"
#include "../Common/propagator_client.h"
#include "../Common/utils.h"
#include "../Common/hashmap.h"    // for hashmap_get

// internal job struct: pairs context + warning
typedef struct {
    CPContext* ctx;
    Warning* w;
} DispatchJob;

// this function runs on a thread pool worker
static void dispatch_job_fn(void* arg) {
    DispatchJob* job = (DispatchJob*)arg;
    CPContext* ctx = job->ctx;
    Warning* w = job->w;

    // find the subtree node for this warning
    NodeInfo* dest = hashmap_get(ctx->map, w->dest_node);

    // compute the next hop
    NodeInfo* hop;
    if (dest == NULL) {
        hop = ctx->me;
    }
    else if (dest == ctx->me) {
        hop = ctx->me;
    }
    else {
        NodeInfo* x = dest;
        while (x->parent && x->parent != ctx->me) {
            x = x->parent;
        }
        hop = x;
    }

    printf("[CP %s] Received warning, propagating to %s\n",
        ctx->me->id, hop->id);

    if (propagator_client_init(hop->address, hop->port)) {
        propagate_warning(w);
        propagator_client_shutdown();
    }

    warning_destroy(w);
    free(job);
}

CPDispatcher* cp_dispatcher_create(CPContext* ctx, size_t num_workers) {
    CPDispatcher* d = malloc(sizeof(*d));
    if (!d) return NULL;

    d->pool = tp_create(num_workers);
    if (!d->pool) {
        free(d);
        return NULL;
    }

    d->ctx = ctx;
    return d;
}

void cp_dispatcher_submit(CPDispatcher* d, Warning* w) {
    DispatchJob* job = malloc(sizeof(*job));
    if (!job) {
        warning_destroy(w);
        return;
    }

    job->ctx = d->ctx;
    job->w = w;

    // *** pass all three args! ***
    if (!tp_submit(d->pool, dispatch_job_fn, job)) {
        warning_destroy(w);
        free(job);
    }
}

void cp_dispatcher_shutdown(CPDispatcher* d) {
    if (!d) return;
    // matches bool tp_submit(ThreadPool*,tp_task_fn,void*)
    tp_shutdown(d->pool);
    free(d);
}
