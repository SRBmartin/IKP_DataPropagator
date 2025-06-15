#ifndef PROPAGATOR_CLIENT_H
#define PROPAGATOR_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "warning.h"

bool send_warning_to(const char* address, uint16_t port, const Warning* w);

#endif