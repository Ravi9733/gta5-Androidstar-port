#include "pg_android_platform.h"

#include <android/log.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdint.h>

extern "C" uint64_t PGAndroidPhysicalMemory(void) {
    struct sysinfo info{};
    if (sysinfo(&info) != 0) return 0;
    return static_cast<uint64_t>(info.totalram) * info.mem_unit;
}

extern "C" uint64_t PGAndroidProcessAvailableMemory(void) {
    // Approximation only: free plus buffer RAM is system-wide, not this app's
    // remaining process budget. Do not substitute it for iOS Jetsam accounting.
    // The engine integration must use Android process/runtime limits separately.
    struct sysinfo info{};
    if (sysinfo(&info) != 0) return 0;
    const uint64_t freeBytes = static_cast<uint64_t>(info.freeram + info.bufferram) * info.mem_unit;
    return freeBytes;
}
