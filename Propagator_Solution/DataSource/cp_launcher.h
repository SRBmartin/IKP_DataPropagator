#ifndef CP_LAUNCHER_H
#define CP_LAUNCHER_H

#include <windows.h>
#include <stddef.h>
#include "../Common/node.h"

extern PROCESS_INFORMATION * g_cpProcs;
extern size_t               g_cpCount;
extern PROCESS_INFORMATION * g_ddProcs;
extern size_t               g_ddCount;

PROCESS_INFORMATION* cp_launch_all(NodeInfo* nodes, size_t     nodesCount, size_t* outLaunchCount);
PROCESS_INFORMATION* dd_launch_all(const NodeInfo* nodes, size_t nodeCount, size_t* outLaunchCount);
void cp_terminate_all(void);

#endif