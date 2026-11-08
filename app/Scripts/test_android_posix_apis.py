#!/usr/bin/env python3
"""Compile Android engine-host POSIX API probes with the Android NDK."""
from pathlib import Path
import os
import subprocess
import sys

ndk = Path(os.environ.get("ANDROID_NDK_HOME") or os.environ.get("ANDROID_NDK_ROOT") or "")
if not ndk.is_dir():
    raise SystemExit("Android NDK not found in ANDROID_NDK_HOME/ANDROID_NDK_ROOT")
prebuilt = ndk / "toolchains/llvm/prebuilt/linux-x86_64/bin"
clang = prebuilt / "aarch64-linux-android29-clang"
if not clang.is_file():
    raise SystemExit(f"Android ARM64 API-29 clang not found: {clang}")
source = Path(__file__).resolve().parents[1] / "Tests/android_posix_api_probe.c"
command = [str(clang), "-std=c11", "-Wall", "-Wextra", "-Werror",
           "-fsyntax-only", str(source)]
print("+", " ".join(command), flush=True)
subprocess.run(command, check=True)
print("Android ARM64/Bionic POSIX API probe passed.")
