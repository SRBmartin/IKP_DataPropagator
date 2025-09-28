// cp_shutdown.c
#ifdef _WIN32
#include <windows.h>
HANDLE g_exitEvent = NULL;

BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_CLOSE_EVENT || signal == CTRL_C_EVENT) {
        if (g_exitEvent) {
            SetEvent(g_exitEvent);
        }
        return TRUE;
    }
    return FALSE;
}
#endif

DWORD WINAPI shutdown_waiter_fn(LPVOID arg) {
    WaitForSingleObject(g_exitEvent, INFINITE);
    return 0;
}
