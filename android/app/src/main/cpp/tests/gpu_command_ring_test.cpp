#include "gpu_command_ring.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

static void put(std::vector<uint8_t> &ring, uint32_t at, uint32_t opcode, uint32_t words,
                const uint32_t *payload = nullptr) {
    std::memcpy(ring.data() + at, &opcode, sizeof(opcode));
    std::memcpy(ring.data() + at + 4, &words, sizeof(words));
    if (words && payload)
        std::memcpy(ring.data() + at + 8, payload, words * sizeof(uint32_t));
}

int main() {
    std::vector<uint8_t> ring(256, 0);
    PGAndroidGpuRingCursor cursor{PG_ANDROID_GPU_RING_HEADER_BYTES};
    PGAndroidGpuPacket packet{};

    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), cursor.tail, &cursor, &packet)
           == PG_ANDROID_RING_EMPTY);

    const uint32_t payload[] = {0x1234u, 0x5678u};
    put(ring, 64, PG_ANDROID_GPU_OP_CREATE_BUFFER, 2, payload);
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 80, &cursor, &packet)
           == PG_ANDROID_RING_PACKET);
    assert(packet.opcode == PG_ANDROID_GPU_OP_CREATE_BUFFER);
    assert(packet.word_count == 2 && packet.words[0] == payload[0] &&
           packet.words[1] == payload[1] && packet.next_tail == 80);
    cursor.tail = packet.next_tail;

    put(ring, 80, PG_ANDROID_GPU_OP_WRAP, 0);
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 88, &cursor, &packet)
           == PG_ANDROID_RING_WRAP);
    assert(cursor.tail == PG_ANDROID_GPU_RING_HEADER_BYTES);

    cursor.tail = 64;
    put(ring, 64, PG_ANDROID_GPU_OP_DRAW, 100);
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 128, &cursor, &packet)
           == PG_ANDROID_RING_CORRUPT);

    cursor.tail = 64;
    put(ring, 64, 999, 0);
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 72, &cursor, &packet)
           == PG_ANDROID_RING_CORRUPT);

    cursor.tail = 65;
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 72, &cursor, &packet)
           == PG_ANDROID_RING_CORRUPT);

    cursor.tail = 64;
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 256, &cursor, &packet)
           == PG_ANDROID_RING_CORRUPT);
    return 0;
}
