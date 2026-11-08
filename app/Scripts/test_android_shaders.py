#!/usr/bin/env python3
"""Synthetic test for Android WGSL extraction and manifest generation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "Scripts" / "prepare_android_shaders.py"

def main():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        src, out = root / "shaders", root / "out"
        src.mkdir()
        wgsl = b"@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0); }"
        (src / "pack0.bin").write_bytes(wgsl)
        index = {
            "_packs": {"0": {"file": "pack0.bin"}},
            "deadbeef": {"ok": True, "stage": "vs", "p": [0, 0, len(wgsl), 0]},
        }
        (src / "index.json").write_text(json.dumps(index), encoding="utf-8")
        subprocess.run([sys.executable, str(SCRIPT), "--shaders", str(src),
                        "--out", str(out)], check=True)
        manifest = json.loads((out / "android-shader-manifest.json").read_text())
        assert manifest["shader_count"] == 1
        item = manifest["shaders"][0]
        assert item["id"] == "deadbeef" and item["stage"] == "vertex"
        assert (out / item["wgsl"]).read_bytes() == wgsl
    print("Android WGSL extraction/manifest test passed.")

if __name__ == "__main__":
    main()
