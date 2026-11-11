#!/usr/bin/env python3
"""Validate the original game's WGSL bundle and emit a deterministic Android shader inventory.

This is deliberately a source-inventory step, not a WGSL-to-SPIR-V compiler. The iOS
project's pgmsl tool emits Metal only and must never be reused as an Android shader backend.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

STAGES = {"vs", "ps", "cs"}
SAFE_NAME = re.compile(r"^[A-Za-z0-9_-]+$")


def build_manifest(shader_dir: Path | str) -> dict:
    root = Path(shader_dir).resolve()
    index_path = root / "index.json"
    try:
        index = json.loads(index_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"Cannot read shader index {index_path}: {error}") from error
    if not isinstance(index, dict):
        raise SystemExit("Shader index must be a JSON object.")

    shaders = []
    for name, record in sorted(index.items()):
        if name.startswith("_") or not isinstance(record, dict):
            continue
        if not record.get("ok") or record.get("stage") not in STAGES:
            continue
        if not SAFE_NAME.fullmatch(name):
            raise SystemExit(f"Unsafe shader identifier in index: {name!r}")
        source = root / f"{name}.wgsl"
        if not source.is_file():
            raise SystemExit(f"Missing WGSL source for {name}: {source.name}")
        item = {
            "id": name,
            "stage": record["stage"],
            "source": source.name,
            "effect": record.get("effect", "?"),
            "program": record.get("program", "?"),
        }
        if record.get("consts"):
            constants = root / f"{name}.consts.json"
            if not constants.is_file():
                raise SystemExit(f"Missing constants table for {name}: {constants.name}")
            try:
                json.loads(constants.read_text(encoding="utf-8"))
            except json.JSONDecodeError as error:
                raise SystemExit(f"Invalid constants JSON for {name}: {error}") from error
            item["constants"] = constants.name
        shaders.append(item)

    if not shaders:
        raise SystemExit("No supported, successful vertex/pixel/compute shaders found.")
    return {
        "format": 1,
        "source_format": "WGSL",
        "android_backend": "Vulkan",
        "shader_count": len(shaders),
        "stage_counts": {stage: sum(item["stage"] == stage for item in shaders)
                         for stage in ("vs", "ps", "cs")},
        "shaders": shaders,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("shader_dir", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    manifest = build_manifest(args.shader_dir)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Validated {manifest['shader_count']} WGSL shaders for the Android Vulkan pipeline.")
    print("Note: this inventory does not compile SPIR-V or implement Vulkan reflection.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
