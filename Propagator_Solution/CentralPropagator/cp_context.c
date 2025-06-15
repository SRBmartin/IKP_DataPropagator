#include "cp_context.h"
#include "../Common/node.h"
#include "../Common/hashmap.h"
#include <stdlib.h>
#include <string.h>

static size_t hash_string(const void* k) {
    const unsigned char* s = k;
    size_t h = 5381;
    while (*s) h = ((h << 5) + h) + *s++;
    return h;
}

static bool eq_string(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b) == 0;
}

static void collect_subtree(HashMap* m, NodeInfo* n) {
    hashmap_put(m, _strdup(n->id), n);
    for (size_t i = 0; i < n->child_count; i++) {
        collect_subtree(m, n->children[i]);
    }
}

CPContext* cp_context_create(const char* nodes_csv,
    const char* root_id)
{
    size_t total;
    NodeInfo* all = node_load_all(nodes_csv, &total);

    if (!all) return NULL;

    HashMap* allmap = hashmap_create(total,
        hash_string,
        eq_string,
        free,
        NULL);

    for (size_t i = 0; i < total; i++) {
        hashmap_put(allmap, _strdup(all[i].id), &all[i]);
    }

    NodeInfo* me = hashmap_get(allmap, root_id);
    if (!me) {
        hashmap_destroy(allmap);
        node_info_destroy_all(all, total);
        return NULL;
    }

    HashMap* submap = hashmap_create(total,
        hash_string,
        eq_string,
        free,
        NULL);
    collect_subtree(submap, me);

    CPContext* ctx = malloc(sizeof(*ctx));
    ctx->map = submap;
    ctx->me = me;
    ctx->child_count = me->child_count;
    ctx->children = malloc(ctx->child_count * sizeof(*ctx->children));
    for (size_t i = 0; i < ctx->child_count; i++) {
        ctx->children[i] = me->children[i];
    }
    ctx->all_nodes = all;
    ctx->all_count = total;

    hashmap_destroy(allmap);
    return ctx;
}

void cp_context_destroy(CPContext* ctx) {
    if (!ctx) return;
    hashmap_destroy(ctx->map);
    free(ctx->children);
    node_info_destroy_all(ctx->all_nodes, ctx->all_count);
    free(ctx);
}
