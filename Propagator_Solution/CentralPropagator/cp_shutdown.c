// cp_shutdown.c
#ifdef _WIN32
#include <windows.h>
HANDLE g_exitEvent = NULL;
#endif
