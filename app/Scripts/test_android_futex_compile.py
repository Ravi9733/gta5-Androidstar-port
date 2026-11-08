#!/usr/bin/env python3
"""Compile the Android/POSIX branch of WasmFutex.c on the Linux CI host."""
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[2]
source = repo / "Source/WasmFutex.c"
with tempfile.TemporaryDirectory(prefix="pg-futex-") as directory:
    temp = Path(directory)
    (temp / "wasm-rt.h").write_text(
        "#pragma once\n"
        "#include <stddef.h>\n"
        "#include <stdint.h>\n"
        "typedef struct { uint8_t *data; uint64_t size; } wasm_rt_shared_memory_t;\n"
        "enum { WASM_RT_TRAP_UNALIGNED = 1, WASM_RT_TRAP_OOB = 2 };\n"
        "void wasm_rt_trap(int);\n"
    )
    command = [
        "cc", "-std=c11", "-D_GNU_SOURCE", "-Wall", "-Wextra",
        "-Werror=implicit-function-declaration", "-fsyntax-only",
        "-I", str(temp), str(source),
    ]
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True)
print("Android/POSIX WASM futex source syntax check passed.")
