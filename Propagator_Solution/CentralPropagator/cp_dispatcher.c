#include <stdio.h>
#include <stdlib.h>

#include "cp_dispatcher.h"
#include "../Common/propagator_client.h"
#include "../Common/utils.h"
#include "../Common/hashmap.h"

typedef struct {
    CPContext* ctx;
    Warning* w;
} DispatchJob;

static void dispatch_job_fn(void* arg) {
    DispatchJob* job = (DispatchJob*)arg;
    CPContext* ctx = job->ctx;
    Warning* w = job->w;

    NodeInfo* dest = hashmap_get(ctx->map, w->dest_node);

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

    if (!send_warning_to(hop->address, hop->port, w)) {
        printf("[CP %s - ERROR] Failed to propagate data to %s.\n",
            ctx->me->id, hop->id);
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

    
    if (!tp_submit(d->pool, dispatch_job_fn, job)) {
        warning_destroy(w);
        free(job);
    }
}

void cp_dispatcher_shutdown(CPDispatcher* d) {
    if (!d) return;
    
    tp_shutdown(d->pool);
    free(d);
}
