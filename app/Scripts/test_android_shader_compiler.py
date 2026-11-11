from pathlib import Path
from tempfile import TemporaryDirectory
import importlib.util, json, os, struct

script=Path(__file__).with_name("compile_android_shaders.py")
spec=importlib.util.spec_from_file_location("compile_android_shaders",script)
module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)

with TemporaryDirectory() as tmp:
    root=Path(tmp); src=root/"src"; out=root/"out"; src.mkdir()
    (src/"index.json").write_text(json.dumps({"v001":{"ok":True,"stage":"vs"},"bad":{"ok":False,"stage":"ps"}}))
    (src/"v001.wgsl").write_text("@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0); }")
    fake=root/"naga"
    fake.write_text("#!/usr/bin/env python3\nimport pathlib,struct,sys\nout=pathlib.Path(sys.argv[2]); out.write_bytes(struct.pack('<5I',0x07230203,0x00010000,0,1,0))\n")
    fake.chmod(0o755)
    result=module.compile_inventory(src,out,str(fake))
    assert result["shader_count"]==1
    assert (out/"v001.spv").read_bytes()[:4]==struct.pack("<I",0x07230203)
    assert json.loads((out/"manifest.json").read_text())["shaders"][0]["stage"]=="vs"
print("WGSL-to-SPIR-V compiler integration tests passed.")
