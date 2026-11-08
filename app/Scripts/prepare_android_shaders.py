#!/usr/bin/env python3
"""Prepare and audit the game's WGSL shader set for a future Android Tint/SPIR-V build.

This deliberately does not claim to compile shaders. It reuses the upstream pack
format, validates every supported WGSL payload and emits a deterministic manifest
that a pinned Tint build can consume later.
"""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

STAGES = {"vs": "vertex", "ps": "fragment", "cs": "compute"}
HASH_RE = re.compile(r"^[0-9a-fA-F]{8,64}$")


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--shaders", type=Path, required=True,
                    help="User-supplied game shaders directory containing index.json and pack files")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    source = args.shaders.resolve()
    out = args.out.resolve()
    index_path = source / "index.json"
    if not index_path.is_file():
        raise SystemExit(f"Missing shader index: {index_path}")
    try:
        index = json.loads(index_path.read_text(encoding="utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise SystemExit(f"Invalid shader index: {exc}")
    if not isinstance(index, dict) or not isinstance(index.get("_packs"), dict):
        raise SystemExit("Shader index must be an object with an _packs map")

    # The upstream unpacker remains the single source of truth for pack offsets.
    unpacked = out / "wgsl"
    unpacker = Path(__file__).with_name("unpack_shaders.py")
    subprocess.run([sys.executable, str(unpacker), "--shaders", str(source),
                    "--out", str(unpacked)], check=True)

    records = []
    for shader_id, record in sorted(index.items()):
        if shader_id.startswith("_") or not isinstance(record, dict):
            continue
        if not record.get("ok") or record.get("stage") not in STAGES:
            continue
        if not HASH_RE.fullmatch(shader_id):
            raise SystemExit(f"Unexpected shader identifier: {shader_id!r}")
        wgsl = unpacked / (shader_id + ".wgsl")
        if not wgsl.is_file() or wgsl.stat().st_size == 0:
            raise SystemExit(f"Missing or empty WGSL payload for {shader_id}")
        try:
            source_text = wgsl.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            raise SystemExit(f"WGSL payload is not UTF-8: {shader_id}")
        if "\x00" in source_text or not re.search(r"\bfn\s+[A-Za-z_]\w*\s*\(", source_text):
            raise SystemExit(f"WGSL payload has no recognizable function: {shader_id}")
        if not re.search(r"@(vertex|fragment|compute)\b", source_text):
            raise SystemExit(f"WGSL payload has no shader stage attribute: {shader_id}")
        expected_stage = STAGES[record["stage"]]
        if not re.search(r"@" + expected_stage + r"\b", source_text):
            raise SystemExit(f"WGSL stage does not match index for {shader_id}")
        consts = unpacked / (shader_id + ".consts.json")
        const_hash = None
        if consts.exists() and consts.stat().st_size:
            try:
                json.loads(consts.read_text(encoding="utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                raise SystemExit(f"Invalid constants JSON for {shader_id}: {exc}")
            const_hash = sha256(consts)
        records.append({
            "id": shader_id,
            "stage": expected_stage,
            "wgsl": str(Path("wgsl") / wgsl.name),
            "wgsl_sha256": sha256(wgsl),
            "wgsl_bytes": wgsl.stat().st_size,
            "constants": str(Path("wgsl") / consts.name) if const_hash else None,
            "constants_sha256": const_hash,
        })
    if not records:
        raise SystemExit("No supported vertex, fragment, or compute shaders found")
    manifest = {
        "format": 1,
        "translator": "pending-pinned-tint",
        "shader_count": len(records),
        "shaders": records,
    }
    out.mkdir(parents=True, exist_ok=True)
    (out / "android-shader-manifest.json").write_text(
        json.dumps(manifest, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(f"Validated {len(records)} WGSL shaders; SPIR-V translation is not performed yet.")


if __name__ == "__main__":
    main()
