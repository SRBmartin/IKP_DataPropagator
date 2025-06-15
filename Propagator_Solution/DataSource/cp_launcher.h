#ifndef CP_LAUNCHER_H
#define CP_LAUNCHER_H

#include <windows.h>
#include <stddef.h>
#include "../Common/node.h"

PROCESS_INFORMATION* cp_launch_all(NodeInfo* nodes, size_t     nodesCount, size_t* outLaunchCount);
PROCESS_INFORMATION* dd_launch_all(const NodeInfo* nodes, size_t nodeCount, size_t* outLaunchCount);
void cp_wait_and_cleanup(PROCESS_INFORMATION* procs, size_t launchCount);
void dd_wait_and_cleanup(PROCESS_INFORMATION* procs, size_t launchCount);

#endif