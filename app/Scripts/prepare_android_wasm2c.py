#!/usr/bin/env python3
"""Build the pinned, host-native WABT 1.0.42 wasm2c used by the Android port.

The proprietary game.wasm is supplied locally and is never copied into the repository.
This script only generates C sources; Android NDK compilation is a separate step.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile
import urllib.request

WABT_VERSION = "1.0.42"
WABT_ARCHIVE = f"wabt-{WABT_VERSION}-linux-x64.tar.gz"
WABT_SHA256 = "84895407a6bbb80e918f33b16b2fb2206021c150b6bc9ff6f761263a745ab131"
ENGINE_SHA256 = "11ca8d2c04c5e843d18ff4aea4899d72c86973c6b031df334e67c446b2ae83e0"
WABT_BASE = f"https://github.com/WebAssembly/wabt/releases/download/{WABT_VERSION}"
WABT_RAW = f"https://raw.githubusercontent.com/WebAssembly/wabt/{WABT_VERSION}"
UPSTREAM_PATCH = "https://raw.githubusercontent.com/kevinerikjs/gta5-ios-port/main/app/Scripts/patch_wasm2c.py"


def download(url: str, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(url, headers={"User-Agent": "gta5-androidstar-port"})
    with urllib.request.urlopen(request, timeout=90) as response, destination.open("wb") as out:
        shutil.copyfileobj(response, out)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_extract(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    root = destination.resolve()
    with tarfile.open(archive, "r:gz") as package:
        for member in package.getmembers():
            target = (destination / member.name).resolve()
            if target != root and root not in target.parents:
                raise SystemExit(f"Unsafe path in WABT archive: {member.name}")
            if member.issym() or member.islnk():
                link = (target.parent / member.linkname).resolve()
                if link != root and root not in link.parents:
                    raise SystemExit(f"Unsafe link in WABT archive: {member.name}")
        if hasattr(tarfile, "data_filter"):
            package.extractall(destination, filter="data")
        else:
            package.extractall(destination)


def run(args: list[str], cwd: Path) -> None:
    print("+", " ".join(args), flush=True)
    subprocess.run(args, cwd=cwd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-wasm", type=Path, required=True,
                        help="local game.wasm matching the pinned upstream build")
    parser.add_argument("--output-dir", type=Path, default=Path("Generated/android"),
                        help="directory for generated game.c and wasm2c runtime files")
    parser.add_argument("--tools-dir", type=Path, default=Path("Tools/android-wasm2c"),
                        help="cache directory for pinned host build tools")
    parser.add_argument("--allow-unpinned-engine", action="store_true",
                        help="development-only override; generated output will not be supported")
    options = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    engine = options.engine_wasm.expanduser().resolve()
    if not engine.is_file():
        raise SystemExit(f"Engine WASM not found: {engine}")
    actual_engine_hash = sha256(engine)
    if actual_engine_hash != ENGINE_SHA256 and not options.allow_unpinned_engine:
        raise SystemExit(
            "Unsupported game.wasm SHA-256. Expected "
            f"{ENGINE_SHA256}, got {actual_engine_hash}. "
            "Use --allow-unpinned-engine only for an isolated experiment."
        )

    tools = options.tools_dir.expanduser().resolve()
    output = options.output_dir.expanduser().resolve()
    tools.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)

    archive = tools / WABT_ARCHIVE
    if not archive.exists() or sha256(archive) != WABT_SHA256:
        download(f"{WABT_BASE}/{WABT_ARCHIVE}", archive)
    actual_archive_hash = sha256(archive)
    if actual_archive_hash != WABT_SHA256:
        raise SystemExit(f"WABT checksum mismatch: {actual_archive_hash}")

    wabt = tools / f"wabt-{WABT_VERSION}"
    if not (wabt / "lib/libwabt.a").is_file() or not (wabt / "include").is_dir():
        safe_extract(archive, tools)
    if not (wabt / "lib/libwabt.a").is_file():
        raise SystemExit("Pinned WABT archive is missing lib/libwabt.a")

    originals = {
        "c-writer-original.cc": f"{WABT_RAW}/src/c-writer.cc",
        "wasm2c-main.cc": f"{WABT_RAW}/src/tools/wasm2c.cc",
    }
    for filename, url in originals.items():
        destination = tools / filename
        if not destination.exists():
            download(url, destination)

    # The checked-in patcher is rooted at app/ and deliberately writes app/Tools.
    patcher = repo / "Scripts/patch_wasm2c.py"
    if not patcher.is_file():
        raise SystemExit(f"Missing upstream patcher: {patcher}")
    patch_root = repo / "Tools"
    patch_root.mkdir(parents=True, exist_ok=True)
    shutil.copy2(tools / "c-writer-original.cc", patch_root / "c-writer-original.cc")
    shutil.copy2(tools / "wasm2c-main.cc", patch_root / "wasm2c-main.cc")
    run(["python3", str(patcher)], repo)

    # The upstream macOS CommonCrypto shim is replaced by OpenSSL's SHA256 API.
    shim = patch_root / "sha256_shim_linux.c"
    shim.write_text(
        "#include <openssl/sha.h>\\n"
        "#include <stddef.h>\\n"
        "#include <stdint.h>\\n"
        "unsigned char *SHA256(const unsigned char *bytes, size_t length, "
        "unsigned char *digest) {\\n"
        "  SHA256_CTX context;\\n"
        "  SHA256_Init(&context);\\n"
        "  while (length) {\\n"
        "    size_t chunk = length > UINT32_MAX ? UINT32_MAX : length;\\n"
        "    SHA256_Update(&context, bytes, chunk);\\n"
        "    bytes += chunk; length -= chunk;\\n"
        "  }\\n"
        "  SHA256_Final(digest, &context);\\n"
        "  return digest;\\n"
        "}\\n"
    )
    shim_object = patch_root / "sha256_shim_linux.o"
    run(["cc", "-Wno-deprecated-declarations", "-c", str(shim), "-o", str(shim_object)], patch_root)

    compiler = os.environ.get("CXX", "g++")
    wasm2c = patch_root / "wasm2c-native"
    run([
        compiler, "-std=c++20", "-O1", "-Wno-deprecated-declarations",
        "-I", str(wabt / "include"),
        str(patch_root / "wasm2c-main.cc"),
        str(patch_root / "c-writer-patched.cc"),
        str(shim_object),
        str(wabt / "lib/libwabt.a"),
        "-lz", "-lbz2", "-lcrypto", "-pthread",
        "-o", str(wasm2c),
    ], patch_root)

    generated_c = output / "game.c"
    run([
        str(wasm2c), "--enable-threads", "--no-debug-names",
        "--module-name=game", "--num-outputs=64",
        str(engine), "-o", str(generated_c),
    ], patch_root)

    runtime_dir = wabt / "share/wabt/wasm2c"
    if not (runtime_dir / "wasm-rt-impl.c").is_file():
        raise SystemExit("WABT wasm2c runtime files are missing from the pinned archive.")
    for runtime_file in runtime_dir.iterdir():
        if runtime_file.is_file():
            shutil.copy2(runtime_file, output / runtime_file.name)

    print(f"Generated: {generated_c}")
    print(f"Engine SHA-256: {actual_engine_hash}")
    print("Next required stage: run engine_names.py and generate_imports.py, then compile with Android NDK.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
