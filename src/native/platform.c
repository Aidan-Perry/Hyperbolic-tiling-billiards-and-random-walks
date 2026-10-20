#include "htb/platform.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>

double htb_now_seconds(void) {
    LARGE_INTEGER counter, frequency;
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&counter) || frequency.QuadPart == 0) return 0.0;
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

long htb_peak_memory_kb(void) {
    PROCESS_MEMORY_COUNTERS counters;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) return 0;
    return (long)(counters.PeakWorkingSetSize / 1024u);
}
#else
#include <sys/resource.h>
#include <time.h>

double htb_now_seconds(void) {
    struct timespec timestamp;
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0) return 0.0;
    return (double)timestamp.tv_sec + (double)timestamp.tv_nsec / 1e9;
}

long htb_peak_memory_kb(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
#ifdef __APPLE__
    return (long)(usage.ru_maxrss / 1024);
#else
    return (long)usage.ru_maxrss;
#endif
}
#endif
