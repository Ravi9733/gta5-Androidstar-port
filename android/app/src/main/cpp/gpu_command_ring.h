#pragma once
#include <stdint.h>
#include "PGAndroidGpuProtocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PGAndroidGpuRingResult {
    PG_ANDROID_RING_EMPTY = 0,
    PG_ANDROID_RING_PACKET = 1,
    PG_ANDROID_RING_WRAP = 2,
    PG_ANDROID_RING_CORRUPT = 3
} PGAndroidGpuRingResult;

typedef struct PGAndroidGpuRingCursor {
    uint32_t tail;
} PGAndroidGpuRingCursor;

typedef struct PGAndroidGpuPacket {
    uint32_t opcode;
    uint32_t word_count;
    const uint32_t *words;
    uint32_t next_tail;
} PGAndroidGpuPacket;

/* ring points at the ring header; head and cursor.tail are byte offsets. */
PGAndroidGpuRingResult PGAndroidGpuRingRead(
    const uint8_t *ring, uint32_t ring_size, uint32_t head,
    PGAndroidGpuRingCursor *cursor, PGAndroidGpuPacket *packet);

/* Consume supported packets in order. Unsupported packets remain at tail. */
int PGAndroidConsumeGpuRing(const uint8_t *ring, uint32_t ring_size,
                            uint32_t head, PGAndroidGpuRingCursor *cursor,
                            uint32_t max_packets, uint32_t *processed);

#ifdef __cplusplus
}
#endif
