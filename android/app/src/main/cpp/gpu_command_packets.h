#pragma once
#include <cstdint>

/* Packet layouts are copied from MRRenderer.mm execute(DRAW/DISPATCH). */
enum PGAndroidDrawKind : uint32_t {
    PG_ANDROID_DRAW = 0,
    PG_ANDROID_DRAW_INDEXED = 1,
    PG_ANDROID_DRAW_INSTANCED = 2,
    PG_ANDROID_DRAW_INDEXED_INSTANCED = 3,
    PG_ANDROID_DRAW_INDEXED_INDIRECT = 4,
    PG_ANDROID_DRAW_INDIRECT = 5
};

struct PGAndroidDrawPacket {
    uint32_t kind=0, countOrArgumentBuffer=0, instanceCountOrArgumentOffset=0;
    uint32_t startVertexOrIndex=0;
    int32_t baseVertex=0;
    uint32_t startInstance=0;
};

struct PGAndroidDispatchPacket { uint32_t x=0, y=0, z=0; };

inline bool PGAndroidDecodeDrawPacket(const uint32_t *words, uint32_t wordCount,
                                      PGAndroidDrawPacket &out) {
    if (!words || wordCount < 6 || words[0] > PG_ANDROID_DRAW_INDIRECT) return false;
    PGAndroidDrawPacket decoded{};
    decoded.kind=words[0];
    decoded.countOrArgumentBuffer=words[1];
    decoded.instanceCount=words[2];
    decoded.startVertexOrIndex=words[3];
    decoded.baseVertex=static_cast<int32_t>(words[4]);
    decoded.startInstance=words[5];
    if (decoded.kind >= PG_ANDROID_DRAW_INDEXED_INDIRECT) {
        // Original MRRenderer.mm reads p[1] as the argument buffer and p[2]
        // as its byte offset for both indirect draw variants.
        if (decoded.countOrArgumentBuffer==0 ||
            (decoded.instanceCountOrArgumentOffset & 3u)) return false;
    }
    out=decoded;
    return true;
}

inline bool PGAndroidDecodeDispatchPacket(const uint32_t *words, uint32_t wordCount,
                                          PGAndroidDispatchPacket &out) {
    if (!words || wordCount < 3 || words[0]==0 || words[1]==0 || words[2]==0)
        return false;
    out={words[0],words[1],words[2]};
    return true;
}
