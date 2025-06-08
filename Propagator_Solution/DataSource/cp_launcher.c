#include "cp_launcher.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PROCESS_INFORMATION* cp_launch_all(const NodeInfo* nodes, size_t nodeCount, size_t* outLaunchCount)
{
    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    char* slash = strrchr(modulePath, '\\');
    if (slash) *(slash + 1) = '\0';
    char cpExePath[MAX_PATH];
    snprintf(cpExePath, sizeof(cpExePath), "%sCentralPropagator.exe", modulePath);

    size_t eligible = 0;
    for (size_t i = 0; i < nodeCount; i++) {
        if (nodes[i].type != NODE_DESTINATION) eligible++;
    }
    if (eligible == 0) {
        *outLaunchCount = 0;
        return NULL;
    }

    PROCESS_INFORMATION* procs =
        malloc(eligible * sizeof(*procs));
    if (!procs) return NULL;

    STARTUPINFOA si = { .cb = sizeof(si) };
    char cmd[1024];
    size_t idx = 0;

    for (size_t i = 0; i < nodeCount; i++) {
        if (nodes[i].type == NODE_DESTINATION) continue;

        ZeroMemory(&procs[idx], sizeof(procs[idx]));

        snprintf(cmd, sizeof(cmd),
            "\"%s\" \"%s\" %u",
            cpExePath,
            nodes[i].id,
            nodes[i].port);

        if (!CreateProcessA(
            NULL,
            cmd,
            NULL, NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si,
            &procs[idx]))
        {
            //this is the cleanup if fails
            for (size_t j = 0; j < idx; j++) {
                TerminateProcess(procs[j].hProcess, 1);
                CloseHandle(procs[j].hThread);
                CloseHandle(procs[j].hProcess);
            }
            free(procs);
            *outLaunchCount = 0;
            return NULL;
        }
        idx++;
    }

    *outLaunchCount = eligible;
    return procs;
}

void cp_wait_and_cleanup(PROCESS_INFORMATION* procs, size_t launchCount)
{
    if (!procs) return;
    for (size_t i = 0; i < launchCount; i++) {
        WaitForSingleObject(procs[i].hProcess, INFINITE);
        CloseHandle(procs[i].hThread);
        CloseHandle(procs[i].hProcess);
    }
    free(procs);
}