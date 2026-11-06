#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Android replacements for the small set of process services currently supplied
// by Apple's os/proc and mach APIs in EngineHost.c.
uint64_t PGAndroidProcessAvailableMemory(void);
uint64_t PGAndroidPhysicalMemory(void);

#ifdef __cplusplus
}
#endif
