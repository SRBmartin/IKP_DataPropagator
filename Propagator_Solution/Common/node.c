#include "node.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/** strip leading/trailing whitespace in-place **/
static char* trim(char* s) {
    char* end;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static size_t hash_string(const void* k) {
    const unsigned char* s = k;
    size_t h = 5381;
    while (*s) h = ((h << 5) + h) + *s++;
    return h;
}

static bool eq_string(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b) == 0;
}

NodeInfo* node_load_all(const char* path, size_t* outCount) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;

    size_t cap = 16, cnt = 0;
    NodeInfo* arr = malloc(cap * sizeof(NodeInfo));
    char** parent_ids = malloc(cap * sizeof(char*));
    char      line[512];

    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        size_t L = strlen(p);
        if (L && (p[L - 1] == '\n' || p[L - 1] == '\r')) p[--L] = '\0';

        char* id = strtok(p, ",");
        char* addr = strtok(NULL, ",");
        char* ps = strtok(NULL, ",");
        char* pid = strtok(NULL, ",");

        if (!id || !addr || !ps) continue;
        if (cnt >= cap) {
            cap *= 2;
            arr = realloc(arr, cap * sizeof(NodeInfo));
            parent_ids = realloc(parent_ids, cap * sizeof(char*));
        }

        id = trim(id);
        addr = trim(addr);
        ps = trim(ps);
        pid = pid ? trim(pid) : NULL;

        arr[cnt].id = _strdup(id);
        arr[cnt].address = _strdup(addr);
        arr[cnt].port = (uint16_t)atoi(ps);
        arr[cnt].parent = NULL;
        arr[cnt].children = NULL;
        arr[cnt].child_count = 0;
        arr[cnt].type = NODE_DESTINATION;

        parent_ids[cnt] = (pid && *pid) ? _strdup(pid) : NULL;
        cnt++;
    }
    fclose(f);

    // build a map of id → NodeInfo*
    HashMap* m = hashmap_create(cap, hash_string, eq_string, free, NULL);
    for (size_t i = 0; i < cnt; i++) {
        hashmap_put(m, _strdup(arr[i].id), &arr[i]);
    }

    // link parents
    for (size_t i = 0; i < cnt; i++) {
        if (parent_ids[i]) {
            NodeInfo* par = hashmap_get(m, parent_ids[i]);
            arr[i].parent = par;
            if (par) par->child_count++;
        }
    }

    // allocate children arrays
    for (size_t i = 0; i < cnt; i++) {
        if (arr[i].child_count > 0) {
            arr[i].children = malloc(
                arr[i].child_count * sizeof(NodeInfo*)
            );
            arr[i].child_count = 0;
        }
    }

    // populate children pointers
    for (size_t i = 0; i < cnt; i++) {
        if (arr[i].parent) {
            NodeInfo* par = arr[i].parent;
            par->children[par->child_count++] = &arr[i];
        }
    }

    // assign node types
    for (size_t i = 0; i < cnt; i++) {
        if (!arr[i].parent)            arr[i].type = NODE_ROOT;
        else if (arr[i].child_count)   arr[i].type = NODE_CENTRAL;
        else                            arr[i].type = NODE_DESTINATION;
    }

    // clean up temp parent_ids
    for (size_t i = 0; i < cnt; i++) {
        free(parent_ids[i]);
    }
    free(parent_ids);
    hashmap_destroy(m);

    *outCount = cnt;
    return arr;
}

void node_info_destroy(NodeInfo* n) {
    if (!n) return;
    free(n->id);
    free(n->address);
    free(n->children);
}

void node_info_destroy_all(NodeInfo* nodes, size_t count) {
    if (!nodes) return;
    for (size_t i = 0; i < count; i++) {
        node_info_destroy(&nodes[i]);
    }
    free(nodes);
}