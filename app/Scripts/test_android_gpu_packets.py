#!/usr/bin/env python3
"""Host-compile tests for original iOS DRAW and DISPATCH packet layouts."""
import pathlib
import subprocess
import tempfile

HARNESS = r"""
#include "gpu_command_packets.h"
#include <cassert>
#include <cstdint>
int main() {
    PGAndroidDrawPacket draw{};
    const uint32_t direct[] = {3, 120, 2, 4, 0xfffffffeu, 7};
    assert(PGAndroidDecodeDrawPacket(direct, 6, draw));
    assert(draw.kind == PG_ANDROID_DRAW_INDEXED_INSTANCED);
    assert(draw.countOrArgumentBuffer == 120 && draw.instanceCountOrArgumentOffset == 2);
    assert(draw.startVertexOrIndex == 4 && draw.baseVertex == -2 && draw.startInstance == 7);
    const uint32_t indirect[] = {4, 55, 16, 0, 0, 0};
    assert(PGAndroidDecodeDrawPacket(indirect, 6, draw));
    assert(draw.kind == PG_ANDROID_DRAW_INDEXED_INDIRECT && draw.instanceCountOrArgumentOffset == 16);
    const uint32_t badKind[] = {6, 1, 1, 0, 0, 0};
    const uint32_t badIndirect[] = {5, 55, 2, 0, 0, 0};
    assert(!PGAndroidDecodeDrawPacket(direct, 5, draw));
    assert(!PGAndroidDecodeDrawPacket(badKind, 6, draw));
    assert(!PGAndroidDecodeDrawPacket(badIndirect, 6, draw));

    PGAndroidCreateShaderPacket shader{};
    const uint32_t shaderWords[] = {91, 1, 0x12345678u, 0x9abcdef0u, 1,
                                    2, 3, 4, 5};
    assert(PGAndroidDecodeCreateShaderPacket(shaderWords, 9, shader));
    assert(shader.objectId == 91 && shader.stage == 1 && shader.hashLow == 0x12345678u);
    assert(shader.hashHigh == 0x9abcdef0u && shader.inputCount == 1);
    assert(shader.inputSignatures[0] == 2 && shader.inputSignatures[3] == 5);
    const uint32_t badShaderStage[] = {91, 3, 1, 2, 0};
    const uint32_t shortShader[] = {91, 1, 1, 2, 1, 3, 4};
    assert(!PGAndroidDecodeCreateShaderPacket(badShaderStage, 5, shader));
    assert(!PGAndroidDecodeCreateShaderPacket(shortShader, 7, shader));

    PGAndroidCreateLayoutPacket layout{};
    const uint32_t layoutWords[] = {77, 1, 10, 11, 12, 13, 14, 15, 16};
    assert(PGAndroidDecodeCreateLayoutPacket(layoutWords, 9, layout));
    assert(layout.objectId == 77 && layout.elementCount == 1);
    assert(layout.elements[0] == 10 && layout.elements[6] == 16);
    const uint32_t shortLayout[] = {77, 1, 10, 11};
    assert(!PGAndroidDecodeCreateLayoutPacket(shortLayout, 4, layout));

    PGAndroidDispatchPacket dispatch{};
    const uint32_t groups[] = {8, 4, 2};
    assert(PGAndroidDecodeDispatchPacket(groups, 3, dispatch));
    assert(dispatch.x == 8 && dispatch.y == 4 && dispatch.z == 2);
    const uint32_t zeroGroup[] = {8, 0, 2};
    assert(!PGAndroidDecodeDispatchPacket(groups, 2, dispatch));
    assert(!PGAndroidDecodeDispatchPacket(zeroGroup, 3, dispatch));
    return 0;
}
"""

def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    header = root / "android/app/src/main/cpp"
    with tempfile.TemporaryDirectory(prefix="pg-packets-") as tmp:
        src = pathlib.Path(tmp) / "test.cpp"
        exe = pathlib.Path(tmp) / "test"
        src.write_text(HARNESS)
        subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-I", str(header), str(src), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("Android draw and dispatch packet tests passed")

if __name__ == "__main__":
    main()
