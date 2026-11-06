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
    // Android's per-process memory limit is not the same as free system RAM.
    // Read the app's RLIMIT_AS when available and fall back to MemAvailable.
    // The engine integration must still apply its own conservative device cap.
    struct sysinfo info{};
    if (sysinfo(&info) != 0) return 0;
    const uint64_t freeBytes = static_cast<uint64_t>(info.freeram + info.bufferram) * info.mem_unit;
    return freeBytes;
}
