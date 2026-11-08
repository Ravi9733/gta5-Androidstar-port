#include "pg_android_platform.h"

#include <sys/sysinfo.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

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
static bool readCgroupValue(const char* path, uint64_t* out, bool allowUnlimited) {
    FILE* file = std::fopen(path, "r");
    if (!file) return false;

    char value[64]{};
    const bool ok = std::fgets(value, sizeof(value), file) != nullptr;
    std::fclose(file);
    if (!ok) return false;

    char* begin = value;
    while (*begin == ' ' || *begin == '\t') ++begin;
    if (allowUnlimited && std::strncmp(begin, "max", 3) == 0) return false;
    // cgroup counters are unsigned decimal byte counts; reject signs and junk.
    if (*begin < '0' || *begin > '9') return false;

    char* parseEnd = nullptr;
    const unsigned long long parsed = std::strtoull(begin, &parseEnd, 10);
    if (parseEnd == begin) return false;
    while (*parseEnd == ' ' || *parseEnd == '\t' || *parseEnd == '\r' || *parseEnd == '\n') ++parseEnd;
    if (*parseEnd != '\0') return false;

    *out = static_cast<uint64_t>(parsed);
    return true;
}

static bool cgroupHeadroom(const char* limitPath, const char* usagePath,
                           uint64_t* outHeadroom) {
    uint64_t limit = 0;
    uint64_t usage = 0;
    if (!readCgroupValue(limitPath, &limit, true) ||
        !readCgroupValue(usagePath, &usage, false)) return false;

    // cgroup v1 commonly reports a very large sentinel for an unlimited limit.
    if (!limit || limit >= (1ull << 60)) return false;
    *outHeadroom = limit > usage ? limit - usage : 0;
    return true;
}
}
extern "C" uint64_t PGAndroidCgroupMemoryHeadroom(void) {
    uint64_t headroom = 0;
    if (cgroupHeadroom("/sys/fs/cgroup/memory.max",
                       "/sys/fs/cgroup/memory.current", &headroom)) return headroom;
    if (cgroupHeadroom("/sys/fs/cgroup/memory/memory.limit_in_bytes",
                       "/sys/fs/cgroup/memory/memory.usage_in_bytes", &headroom)) return headroom;
    return 0;
}


namespace {
static uint64_t androidMemAvailable(void) {
    FILE* file = std::fopen("/proc/meminfo", "r");
    if (!file) return 0;
    char line[256];
    uint64_t availableKb = 0;
    while (std::fgets(line, sizeof(line), file)) {
        unsigned long long value = 0;
        if (std::sscanf(line, "MemAvailable: %llu kB", &value) == 1) {
            availableKb = static_cast<uint64_t>(value);
            break;
        }
    }
    std::fclose(file);
    return availableKb * 1024ull;
}
}

extern "C" uint64_t PGAndroidAvailableMemoryEstimate(void) {
    // MemAvailable is a system-wide estimate, not an Android per-process kill limit.
    // If a finite cgroup limit exists, use the tighter of the two headroom values.
    const uint64_t systemAvailable = androidMemAvailable();
    const uint64_t cgroup = PGAndroidCgroupMemoryHeadroom();
    if (systemAvailable && cgroup) return systemAvailable < cgroup ? systemAvailable : cgroup;
    if (systemAvailable) return systemAvailable;
    if (cgroup) return cgroup;

    // Last-resort fallback only: this can overestimate headroom on Android.
    const uint64_t physical = PGAndroidPhysicalMemory();
    const uint64_t resident = PGAndroidResidentMemory();
    return physical > resident ? physical - resident : 0;
}
