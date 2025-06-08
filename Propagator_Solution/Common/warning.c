#include "warning.h"
#include <stdlib.h>
#include <string.h>

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