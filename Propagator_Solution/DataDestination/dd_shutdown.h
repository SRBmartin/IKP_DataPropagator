#pragma once
#ifdef _WIN32
#include <windows.h>
extern HANDLE g_exitEvent;
BOOL WINAPI console_handler(DWORD signal);
#endif