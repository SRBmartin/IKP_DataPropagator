#include "warning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

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

static void format_local_time(uint64_t ts, char* buf, size_t bufsz) {
    time_t t = (time_t)ts;
    struct tm tm;
#ifdef _MSC_VER
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    strftime(buf, bufsz, "%d.%m.%Y %H:%M:%S", &tm);
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

    const char* type_str = warning_type_to_string(w->type);
    char        timebuf[32];
    format_local_time(w->timestamp, timebuf, sizeof(timebuf));

    const char* fmt =
        "city=%s, type=%s, value=%.2f, time=%s, dest=%s";

    int needed =
#ifdef _MSC_VER
        _scprintf(fmt,
#else
            snprintf(NULL, 0, fmt,
#endif
                    w->city,
                    type_str,
                    w->value,
                    timebuf,
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
    sprintf_s(
        buf,
        (size_t)needed + 1,
        fmt,
        w->city,
        type_str,
        w->value,
        timebuf,
        w->dest_node
    );
#else
    snprintf(
        buf,
        (size_t)needed + 1,
        fmt,
        w->city,
        type_str,
        w->value,
        timebuf,
        w->dest_node
    );
#endif

    return buf;
}

const char* warning_type_to_string(WarningType t) {
    if (t < 0 || t > WARNING_TYPE_OTHER) return "Unknown warning type";
    return WarningTypeNames[t];
}