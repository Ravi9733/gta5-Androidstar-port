#!/usr/bin/env python3
"""Compile the original game's WGSL inventory to validated Vulkan SPIR-V using Naga CLI."""
from __future__ import annotations
import argparse, json, shutil, struct, subprocess, sys
from pathlib import Path

SPIRV_MAGIC = 0x07230203

def compile_inventory(shader_dir: Path, output_dir: Path, compiler: str = "naga") -> dict:
    compiler_path = shutil.which(compiler) if not Path(compiler).is_file() else compiler
    if not compiler_path:
        raise SystemExit(f"WGSL compiler not found: {compiler}. Install naga-cli first.")
    index = json.loads((shader_dir / "index.json").read_text(encoding="utf-8"))
    output_dir.mkdir(parents=True, exist_ok=True)
    compiled=[]
    for name, record in sorted(index.items()):
        if name.startswith("_") or not isinstance(record,dict) or not record.get("ok") or record.get("stage") not in {"vs","ps","cs"}:
            continue
        source=shader_dir/f"{name}.wgsl"
        if not source.is_file(): raise SystemExit(f"Missing WGSL source: {source}")
        target=output_dir/f"{name}.spv"
        result=subprocess.run([str(compiler_path),str(source),str(target)],capture_output=True,text=True)
        if result.returncode:
            raise SystemExit(f"Naga failed for {name}: {(result.stderr or result.stdout).strip()}")
        data=target.read_bytes()
        if len(data)<20 or len(data)%4 or struct.unpack_from("<I",data)[0]!=SPIRV_MAGIC:
            raise SystemExit(f"Compiler produced invalid SPIR-V for {name}")
        compiled.append({"id":name,"stage":record["stage"],"spirv":target.name,"bytes":len(data)})
    if not compiled: raise SystemExit("No WGSL shaders were compiled.")
    manifest={"format":1,"backend":"Vulkan","compiler":"Naga CLI","shader_count":len(compiled),"shaders":compiled}
    (output_dir/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    return manifest

def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("shader_dir",type=Path); p.add_argument("--output-dir",type=Path,required=True)
    p.add_argument("--compiler",default="naga")
    a=p.parse_args(argv); m=compile_inventory(a.shader_dir,a.output_dir,a.compiler)
    print(f"Compiled {m['shader_count']} WGSL shaders to SPIR-V.")
    return 0
if __name__=="__main__": sys.exit(main())
