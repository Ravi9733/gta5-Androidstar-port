#!/usr/bin/env python3
"""Extract the original iOS renderer's stable command-ring protocol for Android.

The Android Vulkan renderer must consume the exact D3D11 command stream emitted by
the original game engine. This script extracts opcode numbers and shared binding
constants from the iOS reference implementation; it does not implement rendering.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

OP_ENUM = re.compile(r"enum\\s+Op\\s*:\\s*uint32_t\\s*\\{(.*?)\\};", re.S)
OP_ENTRY = re.compile(r"([A-Z][A-Z0-9_]*)\\s*=\\s*(0x[0-9a-fA-F]+|[0-9]+)")
CONST = re.compile(r"static\\s+const\\s+uint32_t\\s+(BIND_[A-Z]+)\\s*=\\s*(0x[0-9a-fA-F]+|[0-9]+)")
LAYOUT = re.compile(r"(k(?:ConstTableBuffer|ImmediateBuffer|FirstStorageBuffer|VertexBufferTop|ZeroVertexBuffer|UavTextureBase|NanClampBuffer))\\s*=\\s*(0x[0-9a-fA-F]+|[0-9]+)")


def parse_protocol(renderer_source: str, internal_header: str) -> dict:
    match = OP_ENUM.search(renderer_source)
    if not match:
        raise SystemExit("Could not find the original renderer's enum Op : uint32_t.")
    ops = [{"name": name, "value": int(value, 0)} for name, value in OP_ENTRY.findall(match.group(1))]
    if not ops:
        raise SystemExit("The original renderer opcode enum is empty.")
    values = [item["value"] for item in ops]
    if len(values) != len(set(values)):
        raise SystemExit("Duplicate command-ring opcode values detected.")
    if values != list(range(len(values))):
        raise SystemExit("Opcode values are not contiguous and ordered; review protocol before porting.")
    if ops[0] != {"name": "NOP", "value": 0} or ops[1] != {"name": "WRAP", "value": 1}:
        raise SystemExit("Unexpected NOP/WRAP command-ring prefix.")
    if ops[-1]["value"] != 47:
        raise SystemExit(f"Expected original protocol to end at opcode 47, got {ops[-1]['value']}.")

    bindings = {name: int(value, 0) for name, value in CONST.findall(renderer_source)}
    if not bindings:
        raise SystemExit("Could not extract D3D11 resource binding flags.")
    layouts = {name: int(value, 0) for name, value in LAYOUT.findall(internal_header)}
    required = {"kConstTableBuffer", "kImmediateBuffer", "kFirstStorageBuffer",
                "kVertexBufferTop", "kZeroVertexBuffer", "kUavTextureBase", "kNanClampBuffer"}
    if not required.issubset(layouts):
        raise SystemExit("The renderer's Metal shader binding layout is incomplete.")
    return {
        "format": 1,
        "source": "app/Source/MetalRenderer/MRRenderer.mm",
        "protocol": "GTA game D3D11 command ring",
        "ring_data_header_bytes": 64,
        "frame_slots": 3,
        "opcodes": ops,
        "bind_flags": bindings,
        "shader_binding_reference": layouts,
    }


def render_header(protocol: dict) -> str:
    lines = [
        "#pragma once",
        "/* Generated from the original iOS renderer. Do not edit by hand. */",
        "#include <stdint.h>",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "typedef enum PGAndroidGpuOpcode {",
    ]
    for item in protocol["opcodes"]:
        lines.append(f'    PG_ANDROID_OP_{item["name"]} = {item["value"]},')
    lines += [
        "} PGAndroidGpuOpcode;",
        f'#define PG_ANDROID_GPU_RING_HEADER_BYTES {protocol["ring_data_header_bytes"]}u',
        f'#define PG_ANDROID_GPU_FRAME_SLOTS {protocol["frame_slots"]}u',
        "#ifdef __cplusplus",
        "}",
        "#endif",
        "",
    ]
    return "\\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--renderer", type=Path, required=True)
    parser.add_argument("--internal-header", type=Path, required=True)
    parser.add_argument("--json", type=Path, required=True)
    parser.add_argument("--header", type=Path, required=True)
    args = parser.parse_args(argv)
    protocol = parse_protocol(
        args.renderer.read_text(encoding="utf-8"),
        args.internal_header.read_text(encoding="utf-8"),
    )
    for path in (args.json, args.header):
        path.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(protocol, indent=2) + "\\n", encoding="utf-8")
    args.header.write_text(render_header(protocol), encoding="utf-8")
    print(f'Extracted {len(protocol["opcodes"])} original GPU opcodes (0–{len(protocol["opcodes"])-1}).')
    print("Generated protocol data only; no Vulkan commands are executed by this step.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
