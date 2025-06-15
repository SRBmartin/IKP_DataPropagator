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

char* warning_to_string(const Warning* w) {
    if (!w) return NULL;

    const char* fmt = "city=%s, type=%s, value=%.2f, ts=%llu, dest=%s";
    const char* type_str = warning_type_to_string(w->type);

    int needed =
#ifdef _MSC_VER
        _scprintf(fmt,
#else
            snprintf(NULL, 0, fmt,
#endif
                w->city,
                type_str,
                w->value,
                (unsigned long long)w->timestamp,
                w->dest_node
#ifdef _MSC_VER
            );
#else
        );
#endif

    if (needed < 0) return NULL;
    char* buf = malloc((size_t)needed + 1);
    if (!buf) return NULL;

#ifdef _MSC_VER
    sprintf_s(buf, (size_t)needed + 1, fmt,
#else
    snprintf(buf, (size_t)needed + 1, fmt,
#endif
        w->city,
        type_str,
        w->value,
        (unsigned long long)w->timestamp,
        w->dest_node
#ifdef _MSC_VER
    );
#else
        );
#endif

    return buf;
}

const char* warning_type_to_string(WarningType t) {
    if (t < 0 || t > WARNING_TYPE_OTHER) return "Unknown warning type";
    return WarningTypeNames[t];
}