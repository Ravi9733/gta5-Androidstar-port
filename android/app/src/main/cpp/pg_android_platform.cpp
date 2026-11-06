#include "pg_android_platform.h"

#include <sys/sysinfo.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>

extern "C" uint64_t PGAndroidPhysicalMemory(void) {
    struct sysinfo info{};
    if (sysinfo(&info) != 0) return 0;
    return static_cast<uint64_t>(info.totalram) * info.mem_unit;
}

extern "C" uint64_t PGAndroidResidentMemory(void) {
    // /proc/self/statm reports virtual pages followed by resident pages.
    // This is a measurement of current RSS, not a process memory limit.
    FILE* statm = std::fopen("/proc/self/statm", "r");
    if (!statm) return 0;

    unsigned long virtualPages = 0;
    unsigned long residentPages = 0;
    const int fields = std::fscanf(statm, "%lu %lu", &virtualPages, &residentPages);
    std::fclose(statm);
    if (fields != 2) return 0;

    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) return 0;
    return static_cast<uint64_t>(residentPages) * static_cast<uint64_t>(pageSize);
}
