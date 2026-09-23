/* Test-only Win32 compatibility for ppocr_recognizer.c's Android/POSIX ABI. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

struct PpocrTimespec {
    long tv_sec;
    long tv_nsec;
};

static _Thread_local char host_dl_error[128];

static void host_set_error(const char *operation) {
    unsigned long code = (unsigned long)GetLastError();
    snprintf(host_dl_error, sizeof(host_dl_error), "%s failed (Win32 %lu)",
             operation, code);
}

void *dlopen(const char *filename, int flags) {
    const char *override_path = getenv("PPOCR_HOST_ORT_DLL");
    HMODULE module;
    (void)filename;
    (void)flags;
    host_dl_error[0] = 0;
    module = LoadLibraryA(override_path && override_path[0]
                              ? override_path
                              : "onnxruntime.dll");
    if (!module) host_set_error("LoadLibraryA");
    return (void *)module;
}

void *dlsym(void *handle, const char *symbol) {
    FARPROC address;
    union {
        FARPROC function;
        void *object;
    } conversion;
    host_dl_error[0] = 0;
    address = GetProcAddress((HMODULE)handle, symbol);
    if (!address) {
        host_set_error("GetProcAddress");
        return 0;
    }
    conversion.function = address;
    return conversion.object;
}

int dlclose(void *handle) {
    host_dl_error[0] = 0;
    if (!FreeLibrary((HMODULE)handle)) {
        host_set_error("FreeLibrary");
        return -1;
    }
    return 0;
}

const char *dlerror(void) {
    return host_dl_error[0] ? host_dl_error : 0;
}

int clock_gettime(int clock_id, struct PpocrTimespec *value) {
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    unsigned long long seconds;
    unsigned long long remainder;
    (void)clock_id;
    if (!value || !QueryPerformanceCounter(&counter) ||
        !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
        return -1;
    seconds = (unsigned long long)counter.QuadPart /
              (unsigned long long)frequency.QuadPart;
    remainder = (unsigned long long)counter.QuadPart %
                (unsigned long long)frequency.QuadPart;
    value->tv_sec = (long)seconds;
    value->tv_nsec = (long)((remainder * 1000000000ull) /
                            (unsigned long long)frequency.QuadPart);
    return 0;
}
