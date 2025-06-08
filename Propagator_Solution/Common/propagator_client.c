#include "propagator_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* g_addr;
static uint16_t g_port;

bool propagator_client_init(const char* address, uint16_t port) {
    g_addr = _strdup(address);
    g_port = port;
    return true;
}

bool propagate_warning(const Warning* w) {
    printf("[STUB] %s:%u -> city=%s type=%d value=%.2f\n",
        g_addr, g_port, w->city, w->type, w->value);
    return true;
}

void propagator_client_shutdown(void) {
    free(g_addr);
}