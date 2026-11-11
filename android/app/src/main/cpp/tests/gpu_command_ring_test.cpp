#include "gpu_command_ring.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

static bool gAcceptPackets = true;
static uint32_t gLastOpcode = UINT32_MAX;
static uint32_t gExecutedPackets = 0;
extern "C" bool PGAndroidExecuteGpuPacket(uint32_t opcode, const uint32_t *, uint32_t) {
    gLastOpcode = opcode;
    ++gExecutedPackets;
    return gAcceptPackets;
}

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
    put(ring, 64, PG_ANDROID_OP_CREATE_BUFFER, 2, payload);
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 80, &cursor, &packet)
           == PG_ANDROID_RING_PACKET);
    assert(packet.opcode == PG_ANDROID_OP_CREATE_BUFFER);
    assert(packet.word_count == 2 && packet.words[0] == payload[0] &&
           packet.words[1] == payload[1] && packet.next_tail == 80);
    cursor.tail = packet.next_tail;
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), cursor.tail, &cursor, &packet)
           == PG_ANDROID_RING_EMPTY);
    assert(packet.words == nullptr && packet.word_count == 0);

    put(ring, 80, PG_ANDROID_OP_WRAP, 0);
    assert(PGAndroidGpuRingRead(ring.data(), ring.size(), 88, &cursor, &packet)
           == PG_ANDROID_RING_WRAP);
    assert(cursor.tail == PG_ANDROID_GPU_RING_HEADER_BYTES);

    cursor.tail = 64;
    put(ring, 64, PG_ANDROID_OP_DRAW, 100);
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

    std::vector<uint8_t> unaligned(257, 0);
    cursor.tail = 64;
    assert(PGAndroidGpuRingRead(unaligned.data() + 1, 256, 80, &cursor, &packet)
           == PG_ANDROID_RING_CORRUPT);
    // The consumer advances only after a supported packet succeeds.
    cursor.tail = 64;
    put(ring, 64, PG_ANDROID_OP_CREATE_BUFFER, 2, payload);
    uint32_t processed = 0;
    gAcceptPackets = true;
    gExecutedPackets = 0;
    assert(PGAndroidConsumeGpuRing(ring.data(), ring.size(), 80, &cursor, 4, &processed));
    assert(processed == 1 && cursor.tail == 80);
    assert(gExecutedPackets == 1 && gLastOpcode == PG_ANDROID_OP_CREATE_BUFFER);

    // An unsupported packet remains at the head of the queue for a later backend.
    cursor.tail = 64;
    put(ring, 64, PG_ANDROID_OP_DRAW, 0);
    processed = 0;
    gAcceptPackets = false;
    assert(!PGAndroidConsumeGpuRing(ring.data(), ring.size(), 72, &cursor, 4, &processed));
    assert(cursor.tail == 64 && processed == 0);

    // WRAP is consumed, then the packet at the ring data start is processed.
    cursor.tail = 240;
    put(ring, 240, PG_ANDROID_OP_WRAP, 0);
    put(ring, 64, PG_ANDROID_OP_CREATE_BUFFER, 2, payload);
    gAcceptPackets = true;
    processed = 0;
    assert(PGAndroidConsumeGpuRing(ring.data(), ring.size(), 80, &cursor, 4, &processed));
    assert(processed == 2 && cursor.tail == 80);

    // NOP advances the ring without invoking the GPU backend.
    cursor.tail = 64;
    put(ring, 64, PG_ANDROID_OP_NOP, 0);
    const uint32_t beforeNop = gExecutedPackets;
    processed = 0;
    assert(PGAndroidConsumeGpuRing(ring.data(), ring.size(), 72, &cursor, 4, &processed));
    assert(processed == 1 && cursor.tail == 72 && gExecutedPackets == beforeNop);
    return 0;
n}
