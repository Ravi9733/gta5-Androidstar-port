from pathlib import Path
from tempfile import TemporaryDirectory
import importlib.util, json, struct, hashlib

script=Path(__file__).with_name("compile_android_shaders.py")
spec=importlib.util.spec_from_file_location("compile_android_shaders",script)
module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)

with TemporaryDirectory() as tmp:
    root=Path(tmp); src=root/"src"; out=root/"out"; src.mkdir()
    (src/"index.json").write_text(json.dumps({"0123456789abcdef":{"ok":True,"stage":"vs"},"bad":{"ok":False,"stage":"ps"}}))
    (src/"0123456789abcdef.wgsl").write_text("@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0); }")
    fake=root/"naga"
    fake.write_text("#!/usr/bin/env python3\nimport pathlib,struct,sys\nout=pathlib.Path(sys.argv[2]); out.write_bytes(struct.pack('<5I',0x07230203,0x00010000,0,1,0))\n")
    fake.chmod(0o755)
    result=module.compile_inventory(src,out,str(fake))
    assert result["format"]==2 and result["shader_count"]==1
    binary=(out/"0123456789abcdef.spv").read_bytes()
    assert binary[:4]==struct.pack("<I",0x07230203)
    entry=json.loads((out/"manifest.json").read_text())["shaders"][0]
    assert entry["id"]=="0123456789abcdef" and entry["original_hash"] is None
    assert entry["stage"]=="vs"
    assert entry["sha256"]==hashlib.sha256(binary).hexdigest()

    # Shader IDs are not assumed to be original iOS 64-bit hashes.
    (src/"index.json").write_text(json.dumps({
        "game_vertex_01":{"ok":True,"stage":"vs","original_hash":"ABCDEF0123456789"}
    }))
    (src/"game_vertex_01.wgsl").write_text("@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0); }")
    result=module.compile_inventory(src,out,str(fake))
    entry=json.loads((out/"manifest.json").read_text())["shaders"][0]
    assert entry["id"]=="game_vertex_01"
    assert entry["original_hash"]=="abcdef0123456789"

    (src/"index.json").write_text(json.dumps({
        "game_vertex_01":{"ok":True,"stage":"vs","original_hash":"not-a-hash"}
    }))
    try:
        module.compile_inventory(src,out,str(fake))
    except SystemExit as error:
        assert "Invalid original iOS shader hash" in str(error)
    else:
        raise AssertionError("Invalid original shader hash was accepted")

    (src/"index.json").write_text(json.dumps({"../escape":{"ok":True,"stage":"vs"}}))
    try:
        module.compile_inventory(src,out,str(fake))
    except SystemExit as error:
        assert "Unsafe shader identifier" in str(error)
    else:
        raise AssertionError("Unsafe shader identifier was accepted")
print("WGSL-to-SPIR-V compiler integration tests passed.")
