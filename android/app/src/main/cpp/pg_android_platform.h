#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Android process metrics. Cgroup headroom is 0 when no finite cgroup limit is exposed.
uint64_t PGAndroidCgroupMemoryHeadroom(void);
// Resident memory is the current RSS, not free memory
// and not a promise of how much additional memory Android will permit.
uint64_t PGAndroidResidentMemory(void);
uint64_t PGAndroidPhysicalMemory(void);

#ifdef __cplusplus
}
#endif
