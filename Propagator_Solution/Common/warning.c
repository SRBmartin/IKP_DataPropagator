#include "warning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

Warning* warning_create(
    const char* city,
    WarningType  type,
    double       value,
    uint64_t     timestamp,
    const char* dest_node)
{
    Warning* w = malloc(sizeof(*w));
    if (!w) return NULL;

    w->city = _strdup(city);
    if (!w->city) { free(w); return NULL; }

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

char* warning_to_string(const Warning* w)
{
    if (!w) return NULL;

    const char* fmt = "city=%s, type=%d, value=%.2f, ts=%" PRIu64 ", dest=%s";

    size_t city_len = strlen(w->city);
    size_t dest_len = strlen(w->dest_node);
    size_t buf_len = city_len + dest_len + 64;

    char* buf = malloc(buf_len);
    if (!buf) return NULL;

#ifdef _MSC_VER
    sprintf_s(
        buf,
        buf_len,
        fmt,
        w->city,
        (int)w->type,
        w->value,
        (uint64_t)w->timestamp,
        w->dest_node
    );
#else
    snprintf(
        buf,
        buf_len,
        fmt,
        w->city,
        (int)w->type,
        w->value,
        (uint64_t)w->timestamp,
        w->dest_node
    );
#endif

    return buf;
}
