#!/usr/bin/env python3
"""Tests for the Android WGSL inventory validator."""
from pathlib import Path
from tempfile import TemporaryDirectory
import importlib.util
import json

script = Path(__file__).with_name("prepare_android_shaders.py")
spec = importlib.util.spec_from_file_location("prepare_android_shaders", script)
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)

with TemporaryDirectory(prefix="pg-android-shaders-") as temp:
    root = Path(temp)
    (root / "index.json").write_text(json.dumps({
        "v001": {"ok": True, "stage": "vs", "effect": "world", "program": "main"},
        "p002": {"ok": True, "stage": "ps", "consts": True},
        "c003": {"ok": True, "stage": "cs"},
        "bad": {"ok": False, "stage": "ps"},
        "_meta": {"ok": True, "stage": "vs"},
    }))
    for name in ("v001", "p002", "c003"):
        (root / (name + ".wgsl")).write_text("@compute @workgroup_size(1) fn main() {}")
    (root / "p002.consts.json").write_text("{}")
    result = module.build_manifest(root)
    assert result["shader_count"] == 3
    assert result["stage_counts"] == {"vs": 1, "ps": 1, "cs": 1}
    assert result["shaders"][1]["constants"] == "p002.consts.json"

    (root / "index.json").write_text(json.dumps({"missing": {"ok": True, "stage": "vs"}}))
    try:
        module.build_manifest(root)
    except SystemExit as error:
        assert "Missing WGSL source" in str(error)
    else:
        raise AssertionError("Missing shader source was accepted")

    (root / "index.json").write_text(json.dumps({"../escape": {"ok": True, "stage": "vs"}}))
    try:
        module.build_manifest(root)
    except SystemExit as error:
        assert "Unsafe shader identifier" in str(error)
    else:
        raise AssertionError("Unsafe shader identifier was accepted")

print("Android shader inventory tests passed.")
