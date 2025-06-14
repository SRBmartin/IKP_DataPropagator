#ifndef PROPAGATOR_CLIENT_H
#define PROPAGATOR_CLIENT_H

#include "warning.h"
#include <stdint.h>
#include <stdbool.h>

bool propagator_client_init(const char* address, uint16_t port);
bool propagate_warning(const Warning* w);
void propagator_client_shutdown(void);

#endif