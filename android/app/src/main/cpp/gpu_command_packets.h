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

/* Original iOS CREATE_SHADER: id, stage, hash low/high, input count, then
 * four words per input signature. */
struct PGAndroidCreateShaderPacket {
    uint32_t objectId=0, stage=0, hashLow=0, hashHigh=0, inputCount=0;
    const uint32_t *inputSignatures=nullptr;
};
inline bool PGAndroidDecodeCreateShaderPacket(const uint32_t *words, uint32_t wordCount,
                                               PGAndroidCreateShaderPacket &out) {
    if (!words || wordCount < 5 || words[1] > 2 || words[4] > 64 ||
        wordCount < 5u + words[4] * 4u) return false;
    out.objectId=words[0]; out.stage=words[1]; out.hashLow=words[2];
    out.hashHigh=words[3]; out.inputCount=words[4];
    out.inputSignatures=words+5;
    return out.objectId != 0;
}

/* Original iOS CREATE_LAYOUT: id, element count, then seven words per element. */
struct PGAndroidCreateLayoutPacket {
    uint32_t objectId=0, elementCount=0;
    const uint32_t *elements=nullptr;
};
inline bool PGAndroidDecodeCreateLayoutPacket(const uint32_t *words, uint32_t wordCount,
                                               PGAndroidCreateLayoutPacket &out) {
    if (!words || wordCount < 2 || words[0] == 0 || words[1] > 32 ||
        wordCount < 2u + words[1] * 7u) return false;
    out.objectId=words[0]; out.elementCount=words[1]; out.elements=words+2;
    return true;
}

struct PGAndroidDispatchPacket { uint32_t x=0, y=0, z=0; };

inline bool PGAndroidDecodeDrawPacket(const uint32_t *words, uint32_t wordCount,
                                      PGAndroidDrawPacket &out) {
    if (!words || wordCount < 6 || words[0] > PG_ANDROID_DRAW_INDIRECT) return false;
    PGAndroidDrawPacket decoded{};
    decoded.kind=words[0];
    decoded.countOrArgumentBuffer=words[1];
    decoded.instanceCountOrArgumentOffset=words[2];
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
