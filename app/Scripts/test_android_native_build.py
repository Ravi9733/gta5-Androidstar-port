#!/usr/bin/env python3
"""Build the real Android Vulkan JNI shared library with the installed NDK."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ndk = os.environ.get("ANDROID_NDK_HOME") or os.environ.get("ANDROID_NDK_ROOT")
if not ndk:
    raise SystemExit("ANDROID_NDK_HOME/ANDROID_NDK_ROOT is required")
ndk = Path(ndk).resolve()
toolchain = ndk / "build/cmake/android.toolchain.cmake"
if not toolchain.is_file():
    raise SystemExit(f"Android NDK CMake toolchain not found: {toolchain}")
source = Path(__file__).resolve().parents[2] / "android/app/src/main/cpp"
if not (source / "CMakeLists.txt").is_file():
    raise SystemExit(f"Android CMake project not found: {source}")
cmake = shutil.which("cmake")
if not cmake:
    raise SystemExit("cmake is required")
with tempfile.TemporaryDirectory(prefix="pg-android-native-") as build:
    subprocess.run([
        cmake, "-S", str(source), "-B", build,
        "-DCMAKE_TOOLCHAIN_FILE=" + str(toolchain),
        "-DANDROID_ABI=arm64-v8a",
        "-DANDROID_PLATFORM=android-29",
        "-DCMAKE_BUILD_TYPE=Release",
    ], check=True)
    subprocess.run([cmake, "--build", build, "--parallel", "2"], check=True)
    library = Path(build) / "libgta5port.so"
    if not library.is_file():
        raise SystemExit("CMake completed without producing libgta5port.so")
    print(f"Android ARM64 native library build passed: {library.stat().st_size} bytes")
