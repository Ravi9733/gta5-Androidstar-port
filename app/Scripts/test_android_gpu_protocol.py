#!/usr/bin/env python3
"""Unit tests for extracting the original GPU command-ring contract."""
from pathlib import Path
import importlib.util

script = Path(__file__).with_name("prepare_android_gpu_protocol.py")
spec = importlib.util.spec_from_file_location("prepare_android_gpu_protocol", script)
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)

names = [
    "NOP", "WRAP", "INIT", "CREATE_TEXTURE", "DESTROY_OBJECT", "CLEAR_RT",
    "CLEAR_DS", "PRESENT", "FENCE", "SCREENSHOT", "LOG", "CREATE_BUFFER",
    "UPLOAD_BUFFER", "UPLOAD_TEXTURE", "CREATE_SHADER", "CREATE_LAYOUT",
    "CREATE_STATE", "CREATE_SRV", "SET_SHADER", "SET_INPUT_LAYOUT", "SET_TOPOLOGY",
    "SET_VERTEX_BUFFERS", "SET_INDEX_BUFFER", "SET_CBUFFERS", "SET_SRVS",
    "SET_SAMPLERS", "SET_RENDER_TARGETS", "SET_BLEND", "SET_DEPTH_STENCIL",
    "SET_RASTER", "SET_VIEWPORTS", "SET_SCISSORS", "DRAW", "CLEAR_STATE",
    "COPY_REGION", "COPY_RESOURCE", "RESOLVE", "CREATE_UAV", "SET_UAVS",
    "DISPATCH", "CLEAR_UAV", "GENERATE_MIPS", "READBACK", "COPY_COUNTER",
    "CREATE_QUERY", "BEGIN_QUERY", "END_QUERY", "UPLOAD_BUFFER_NO_OVERWRITE",
]
renderer = "enum Op : uint32_t { " + ", ".join(
    f"{name} = {number}" for number, name in enumerate(names)
) + " };\n"
renderer += "static const uint32_t BIND_VB = 1, BIND_IB = 2, BIND_CB = 4, BIND_SRV = 8, BIND_RT = 32, BIND_DS = 64, BIND_UAV = 128;\n"
header = """
enum : uint32_t {
 kConstTableBuffer = 14, kImmediateBuffer = 15, kFirstStorageBuffer = 16,
 kVertexBufferTop = 30, kZeroVertexBuffer = 15, kUavTextureBase = 64,
 kNanClampBuffer = 30,
};
"""
p = module.parse_protocol(renderer, header)
assert len(p["opcodes"]) == 48
assert p["opcodes"][-1]["name"] == "UPLOAD_BUFFER_NO_OVERWRITE"
assert p["bind_flags"]["BIND_UAV"] == 128
assert p["shader_binding_reference"]["kUavTextureBase"] == 64
h = module.render_header(p)
assert "PG_ANDROID_OP_UPLOAD_BUFFER_NO_OVERWRITE = 47" in h
assert "PG_ANDROID_GPU_RING_HEADER_BYTES 64u" in h

bad = renderer.replace("DRAW = 32", "DRAW = 99")
try:
    module.parse_protocol(bad, header)
except SystemExit as error:
    assert "contiguous" in str(error)
else:
    raise AssertionError("Non-contiguous opcode values were accepted")

try:
    module.parse_protocol("enum Op : uint32_t { NOP = 0, WRAP = 1 };", header)
except SystemExit as error:
    assert "end at opcode 47" in str(error)
else:
    raise AssertionError("An incomplete renderer protocol was accepted")

print("Android GPU protocol extraction tests passed.")
