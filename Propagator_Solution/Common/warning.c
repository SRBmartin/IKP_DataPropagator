#include "warning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

Warning* warning_create(const char* city,
    WarningType type,
    double value,
    uint64_t timestamp,
    const char* dest_node)
{
    Warning* w = malloc(sizeof(Warning));
    if (!w) return NULL;

    w->city = _strdup(city);
    if (!w->city) {
        free(w);
        return NULL;
    }

    w->type = type;
    w->value = value;
    w->timestamp = timestamp;

    w->dest_node = _strdup(dest_node);
    if (!w->dest_node) {
        free(w->city);
        free(w);
        return NULL;
    }

    return w;
}

void warning_destroy(Warning* w)
{
    if (!w) return;
    free(w->city);
    free(w->dest_node);
    free(w);
}

char* warning_to_string(const Warning* w) {
    int n = snprintf(
        NULL, 0,
        "city=%s, type=%d, value=%.2f, ts=%" PRIu64 ", dest=%s",
        w->city, w->type, w->value, (uint64_t)w->timestamp, w->dest_node
    );
    if (n < 0) return NULL;
    char* buf = malloc(n + 1);
    if (!buf) return NULL;
    snprintf(
        buf, n + 1,
        "city=%s, type=%d, value=%.2f, ts=%" PRIu64 ", dest=%s",
        w->city, w->type, w->value, (uint64_t)w->timestamp, w->dest_node
    );
    return buf;
}