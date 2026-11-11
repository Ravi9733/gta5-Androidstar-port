#include "gpu_command_ring.h"

#include <cstring>
#include <limits.h>

extern "C" PGAndroidGpuRingResult PGAndroidGpuRingRead(
    const uint8_t *ring, uint32_t ring_size, uint32_t head,
    PGAndroidGpuRingCursor *cursor, PGAndroidGpuPacket *packet) {
    if (!ring || !cursor || !packet ||
        (reinterpret_cast<uintptr_t>(ring) & 3u) != 0u ||
        ring_size < PG_ANDROID_GPU_RING_HEADER_BYTES + 8u ||
        (ring_size & 3u) != 0u ||
        head < PG_ANDROID_GPU_RING_HEADER_BYTES || head >= ring_size ||
        cursor->tail < PG_ANDROID_GPU_RING_HEADER_BYTES ||
        cursor->tail >= ring_size ||
        (head & 3u) != 0u || (cursor->tail & 3u) != 0u) {
        return PG_ANDROID_RING_CORRUPT;
    }
    packet->opcode = 0;
    packet->word_count = 0;
    packet->words = nullptr;
    packet->next_tail = cursor->tail;
    if (head == cursor->tail) return PG_ANDROID_RING_EMPTY;

    const uint32_t tail = cursor->tail;
    if (tail > ring_size - 8u || (tail < head && head - tail < 8u))
        return PG_ANDROID_RING_CORRUPT;

    uint32_t header[2];
    std::memcpy(header, ring + tail, sizeof(header));
    const uint32_t opcode = header[0];
    const uint32_t words = header[1];

    if (opcode == PG_ANDROID_OP_WRAP) {
        cursor->tail = PG_ANDROID_GPU_RING_HEADER_BYTES;
        packet->opcode = opcode;
        packet->word_count = 0;
        packet->words = nullptr;
        packet->next_tail = cursor->tail;
        return PG_ANDROID_RING_WRAP;
    }

    if (opcode > PG_ANDROID_OP_UPLOAD_BUFFER_NO_OVERWRITE ||
        words > (ring_size - tail - 8u) / 4u ||
        (tail < head && (uint64_t)tail + 8u + (uint64_t)words * 4u > head) ||
        (uint64_t)tail + 8u + (uint64_t)words * 4u > UINT32_MAX) {
        return PG_ANDROID_RING_CORRUPT;
    }

    packet->opcode = opcode;
    packet->word_count = words;
    packet->words = reinterpret_cast<const uint32_t *>(ring + tail + 8u);
    packet->next_tail = tail + 8u + words * 4u;
    return PG_ANDROID_RING_PACKET;
}
