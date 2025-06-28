#include "cp_launcher.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

PROCESS_INFORMATION * g_cpProcs = NULL;
size_t               g_cpCount = 0;
PROCESS_INFORMATION * g_ddProcs = NULL;
size_t               g_ddCount = 0;

static HANDLE g_job = NULL;

static BOOL create_job_object()
{
    g_job = CreateJobObject(NULL, NULL);
    if (!g_job) return FALSE;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = { 0 };
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    return SetInformationJobObject(
        g_job,
        JobObjectExtendedLimitInformation,
        &limits,
        sizeof(limits)
    );
}

PROCESS_INFORMATION* cp_launch_all(
    const NodeInfo* nodes,
    size_t          nodeCount,
    size_t* outLaunchCount)
{
    if (!create_job_object()) return NULL;

    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    char* slash = strrchr(modulePath, '\\');
    if (slash) *(slash + 1) = '\0';

    char cpExePath[MAX_PATH];
    snprintf(cpExePath, sizeof(cpExePath),
        "%sCentralPropagator.exe", modulePath);

    size_t eligible = 0;
    for (size_t i = 0; i < nodeCount; i++)
        if (nodes[i].type != NODE_DESTINATION)
            eligible++;
    if (eligible == 0) { *outLaunchCount = 0; return NULL; }

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
            NULL, cmd,
            NULL, NULL, TRUE,
            CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si, &procs[idx]))
        {
            // Cleanup on failure
            for (size_t j = 0; j < idx; j++) {
                TerminateProcess(procs[j].hProcess, 1);
                CloseHandle(procs[j].hThread);
                CloseHandle(procs[j].hProcess);
            }
            free(procs);
            CloseHandle(g_job);
            g_job = NULL;
            *outLaunchCount = 0;
            return NULL;
        }

        AssignProcessToJobObject(g_job, procs[idx].hProcess);
        idx++;
    }

    *outLaunchCount = eligible;
    g_cpProcs = procs;
    g_cpCount = eligible;
    return g_cpProcs;
}

PROCESS_INFORMATION* dd_launch_all(const NodeInfo* nodes, size_t nodeCount, size_t* outLaunchCount)
{
    if (!g_job && !create_job_object()) return NULL;

    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    char* slash = strrchr(modulePath, '\\');
    if (slash) *(slash + 1) = '\0';

    char exePath[MAX_PATH];
    snprintf(exePath, sizeof(exePath),
        "%sDataDestination.exe", modulePath);

    size_t count = 0;
    for (size_t i = 0; i < nodeCount; i++) {
        if (nodes[i].type == NODE_DESTINATION) count++;
    }
    if (count == 0) { *outLaunchCount = 0; return NULL; }

    PROCESS_INFORMATION* procs =
        malloc(count * sizeof(*procs));
    if (!procs) return NULL;

    STARTUPINFOA si = { .cb = sizeof(si) };
    char cmd[1024];
    size_t idx = 0;

    for (size_t i = 0; i < nodeCount; i++) {
        if (nodes[i].type != NODE_DESTINATION) continue;

        ZeroMemory(&procs[idx], sizeof(procs[idx]));
        snprintf(cmd, sizeof(cmd),
            "\"%s\" \"%s\" %u",
            exePath,
            nodes[i].id,
            nodes[i].port);

        if (!CreateProcessA(
            NULL, cmd,
            NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si, &procs[idx]))
        {
            // cleanup on failure
            for (size_t j = 0; j < idx; j++) {
                TerminateProcess(procs[j].hProcess, 1);
                CloseHandle(procs[j].hThread);
                CloseHandle(procs[j].hProcess);
            }
            free(procs);
            CloseHandle(g_job);
            g_job = NULL;
            *outLaunchCount = 0;
            return NULL;
        }
        AssignProcessToJobObject(g_job, procs[idx].hProcess);
        idx++;
    }

    *outLaunchCount = count;
    g_ddCount = count;
    g_ddProcs = procs;
    return g_ddProcs;
}

void cp_terminate_all(void) {
    if (g_job) {
        CloseHandle(g_job);
        g_job = NULL;
    }

    if (g_cpProcs) {
        for (size_t i = 0; i < g_cpCount; i++) {
            WaitForSingleObject(g_cpProcs[i].hProcess, INFINITE);
            CloseHandle(g_cpProcs[i].hThread);
            CloseHandle(g_cpProcs[i].hProcess);
        }
        free(g_cpProcs);
        g_cpProcs = NULL;
        g_cpCount = 0;
    }

    if (g_ddProcs) {
        for (size_t i = 0; i < g_ddCount; i++) {
            WaitForSingleObject(g_ddProcs[i].hProcess, INFINITE);
            CloseHandle(g_ddProcs[i].hThread);
            CloseHandle(g_ddProcs[i].hProcess);
        }
        free(g_ddProcs);
        g_ddProcs = NULL;
        g_ddCount = 0;
    }
}