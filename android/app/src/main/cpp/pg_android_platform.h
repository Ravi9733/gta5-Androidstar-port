#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Android process metrics. Resident memory is the current RSS, not free memory
// and not a promise of how much additional memory Android will permit.
uint64_t PGAndroidResidentMemory(void);
uint64_t PGAndroidPhysicalMemory(void);

#ifdef __cplusplus
}
#endif
