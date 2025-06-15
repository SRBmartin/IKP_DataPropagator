#ifndef WARNING_H
#define WARNING_H

#include <stdint.h>

typedef enum WarningType {
    WARNING_TYPE_FLOOD,
    WARNING_TYPE_FIRE,
    WARNING_TYPE_STORM,
    WARNING_TYPE_OTHER
} WarningType;

typedef struct Warning {
    char* city;
    WarningType  type;
    double       value;
    uint64_t     timestamp;
    char*        dest_node;
} Warning;

static const char* WarningTypeNames[] = {
    [WARNING_TYPE_FLOOD] = "Flood alert",
    [WARNING_TYPE_FIRE] = "Fire alert",
    [WARNING_TYPE_STORM] = "Storm alert",
    [WARNING_TYPE_OTHER] = "Other alert"
};

Warning* warning_create(const char* city,
    WarningType type,
    double value,
    uint64_t timestamp,
    const char* dest_node);
void warning_destroy(Warning* w);
char* warning_to_string(const Warning* w);
const char* warning_type_to_string(WarningType t);

#endif