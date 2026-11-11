#!/usr/bin/env python3
"""Host-compile tests for original D3D11 shader binding packet decoding."""
import pathlib
import subprocess
import tempfile

HARNESS = r"""
#include "gpu_binding_state.h"
#include <cassert>
#include <cstdint>
int main() {
    PGAndroidGpuBindingState state;
    const uint32_t cb[] = {1, 14, 2, 91, 256, 64, 92, 512, 128};
    assert(state.setConstantBuffers(cb, 9));
    assert(state.constantBuffer(1,14).objectId == 91);
    assert(state.constantBuffer(1,15).offset == 512);
    const uint32_t srv[] = {2, 126, 2, 701, 702};
    assert(state.setShaderResources(srv, 5));
    assert(state.shaderResource(2,127) == 702);
    const uint32_t smp[] = {0, 15, 1, 801};
    assert(state.setSamplers(smp, 4));
    assert(state.sampler(0,15) == 801);
    const uint32_t uav[] = {2, 6, 2, 901, 902, 0xffffffffu, 903};
    assert(state.setUnorderedAccess(uav, 7));
    assert(state.unorderedAccess(2,6).objectId == 901);
    assert(state.unorderedAccess(2,7).counterObjectId == 903);
    assert(!state.setConstantBuffers(cb, 8));
    const uint32_t badStage[] = {3,0,0};
    assert(!state.setShaderResources(badStage,3));
    const uint32_t overflow[] = {0,127,2,1,2};
    assert(!state.setShaderResources(overflow,5));

    const uint32_t rt[] = {
        100,1,2,3, 101,2,3,4, 102,3,4,5, 103,4,5,6,
        104,5,6,7, 105,6,7,8, 106,7,8,9, 107,8,9,10,
        200,9,10,11,12
    };
    assert(state.setRenderTargets(rt,37));
    assert(state.renderTarget(0).viewId==100 && state.renderTarget(7).flags==10);
    assert(state.depthTarget().viewId==200 && state.depthTarget().readOnly==12);
    assert(!state.setRenderTargets(rt,36));
    state.clear();
    assert(state.renderTarget(0).viewId==0 && state.depthTarget().viewId==0);
    state.clear();
    assert(state.constantBuffer(1,14).objectId == 0);
    assert(state.shaderResource(2,127) == 0);
    assert(state.sampler(0,15) == 0);
    assert(state.unorderedAccess(2,6).objectId == 0);
    return 0;
}
"""

def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    header = root / "android/app/src/main/cpp"
    with tempfile.TemporaryDirectory(prefix="pg-bindings-") as tmp:
        src = pathlib.Path(tmp) / "test.cpp"
        exe = pathlib.Path(tmp) / "test"
        src.write_text(HARNESS)
        subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-I", str(header), str(src), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("Android shader binding packet tests passed")

if __name__ == "__main__":
    main()
