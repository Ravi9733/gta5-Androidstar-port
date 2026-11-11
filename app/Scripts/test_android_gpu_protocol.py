#!/usr/bin/env python3
"""Unit tests for extracting the original GPU command-ring contract."""
from pathlib import Path
from tempfile import TemporaryDirectory
import importlib.util

script = Path(__file__).with_name("prepare_android_gpu_protocol.py")
spec = importlib.util.spec_from_file_location("prepare_android_gpu_protocol", script)
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)

renderer = """
enum Op : uint32_t {
 NOP = 0, WRAP = 1, INIT = 2, DRAW = 3, UPLOAD_BUFFER_NO_OVERWRITE = 4,
};
static const uint32_t BIND_VB = 1, BIND_IB = 2, BIND_CB = 4, BIND_SRV = 8;
"""
header = """
enum : uint32_t {
 kConstTableBuffer = 14, kImmediateBuffer = 15, kFirstStorageBuffer = 16,
 kVertexBufferTop = 30, kZeroVertexBuffer = 15, kUavTextureBase = 64,
 kNanClampBuffer = 30,
};
"""
# A short synthetic protocol exercises parsing independently of the full 48-op source.
p = module.parse_protocol(renderer, header)
assert [x["name"] for x in p["opcodes"]] == [
    "NOP", "WRAP", "INIT", "DRAW", "UPLOAD_BUFFER_NO_OVERWRITE"
]
assert p["bind_flags"]["BIND_UAV"] if "BIND_UAV" in p["bind_flags"] else True
assert p["shader_binding_reference"]["kUavTextureBase"] == 64
h = module.render_header(p)
assert "PG_ANDROID_OP_UPLOAD_BUFFER_NO_OVERWRITE = 4" in h
assert "PG_ANDROID_GPU_RING_HEADER_BYTES 64u" in h

bad = renderer.replace("DRAW = 3", "DRAW = 9")
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
