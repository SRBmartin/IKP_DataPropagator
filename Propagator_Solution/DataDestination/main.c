#include <stdio.h>
#include <stdlib.h>
#include "data_destination.h"
#include "../Common/utils.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <node-id> <port>\n", argv[0]);
        return 1;
    }

    const char* id = argv[1];
    uint16_t    port = (uint16_t)atoi(argv[2]);

    initializeConsoleSettings();

    DDContext* dd = dd_create(id, port);
    if (!dd) {
        fprintf(stderr, "Failed to start DataDestination %s\n", id);
        return 1;
    }

    printf("[DD %s] listening on port %hu\n", id, port);
    printf("Press any key to exit and stop the node...\n");
    getchar();

    dd_destroy(dd);
    return 0;
}