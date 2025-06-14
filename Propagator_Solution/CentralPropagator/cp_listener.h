#ifndef CP_LISTENER_H
#define CP_LISTENER_H

#include <stdint.h>
#include "cp_context.h"
#include "cp_dispatcher.h"

// blocks, listening on `port`
void cp_listener_run(
    const CPContext* ctx,
    uint16_t         port,
    CPDispatcher* dispatcher);

#endif // CP_LISTENER_H
