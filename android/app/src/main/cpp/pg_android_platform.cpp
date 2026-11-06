#include "pg_android_platform.h"

#include <sys/sysinfo.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits>

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

namespace {
static uint64_t readLimit(const char* path) {
    FILE* file = std::fopen(path, "r");
    if (!file) return 0;
    char value[64]{};
    const bool ok = std::fgets(value, sizeof(value), file) != nullptr;
    std::fclose(file);
    if (!ok || value[0] == 'm') return 0; // cgroup v2: "max"
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    return end == value ? 0 : static_cast<uint64_t>(parsed);
}
static uint64_t cgroupHeadroom(const char* limitPath, const char* usagePath) {
    const uint64_t limit = readLimit(limitPath);
    const uint64_t usage = readLimit(usagePath);
    // Ignore absent, unlimited, or implausibly large v1 sentinel limits.
    if (!limit || !usage || limit >= (1ull << 60)) return 0;
    return limit > usage ? limit - usage : 0;
}
}
extern "C" uint64_t PGAndroidCgroupMemoryHeadroom(void) {
    uint64_t headroom = cgroupHeadroom("/sys/fs/cgroup/memory.max",
                                       "/sys/fs/cgroup/memory.current");
    if (headroom) return headroom;
    return cgroupHeadroom("/sys/fs/cgroup/memory/memory.limit_in_bytes",
                          "/sys/fs/cgroup/memory/memory.usage_in_bytes");
}
