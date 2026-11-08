#!/usr/bin/env python3
"""Unit-test the generated wasm runtime trap callback patch."""
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import shutil
import subprocess
import tempfile

script = Path(__file__).resolve().parent / "prepare_android_engine.py"
spec = spec_from_file_location("prepare_android_engine", script)
module = module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory(prefix="pg-runtime-hook-") as directory:
    runtime = Path(directory) / "wasm-rt-impl.c"
    marker = "void wasm_rt_trap(wasm_rt_trap_t code) {"
    runtime.write_text("#include <stdlib.h>\ntypedef int wasm_rt_trap_t;\n" + marker + "\n  abort();\n}\n")
    module.install_trap_callback(runtime)
    patched = runtime.read_text()
    assert patched.count("pg_record_trap(code)") == 1
    assert "\\n" not in patched, "The generated C contains literal backslash-n text"
    compiler = shutil.which("cc")
    if compiler:
        subprocess.run([compiler, "-fsyntax-only", str(runtime)], check=True)
    assert patched.index("pg_record_trap(code)") < patched.index("abort();")
    module.install_trap_callback(runtime)
    assert runtime.read_text() == patched

    invalid = Path(directory) / "invalid.c"
    invalid.write_text("void unrelated(void) {}\n")
    try:
        module.install_trap_callback(invalid)
    except SystemExit as error:
        assert "Cannot install native trap diagnostics" in str(error)
    else:
        raise AssertionError("A runtime without wasm_rt_trap was accepted")

print("Android generated-runtime trap callback tests passed.")
