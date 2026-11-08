# Linux-hosted Android wasm2c bootstrap

This tool reproduces the upstream iOS port's **WABT 1.0.42 custom wasm2c generation stage** on Ubuntu/Linux. It does not build the Android APK or claim to boot gameplay.

## Requirements

- Ubuntu 22.04/24.04 host (GitHub Actions runner is suitable)
- Python 3.10+, g++, cc, zlib/bzip2 development libraries, OpenSSL development headers
- A locally supplied `game.wasm` whose SHA-256 is:
  `11ca8d2c04c5e843d18ff4aea4899d72c86973c6b031df334e67c446b2ae83e0`

On Ubuntu, install host dependencies with:

```sh
sudo apt-get update
sudo apt-get install -y build-essential python3 zlib1g-dev libbz2-dev libssl-dev
```

## Validate the host toolchain in CI

The repository workflow builds the custom Linux host executable without requiring or uploading proprietary game files:

```sh
python3 app/Scripts/prepare_android_wasm2c.py --toolchain-only
```

## Generate C from the supported local engine input

From the repository root:

```sh
python3 app/Scripts/prepare_android_wasm2c.py \
  --engine-wasm /absolute/path/to/game.wasm \
  --output-dir /absolute/path/to/generated/android
```

The script downloads and verifies the official Linux x86-64 WABT 1.0.42 release archive (SHA-256 pinned in the script), downloads the matching WABT source files, applies the repository's exact `patch_wasm2c.py`, builds a **host** `wasm2c-native`, and runs it with the upstream flags:

`--enable-threads --no-debug-names --module-name=game --num-outputs=64`

The engine WASM is read in place and is never copied into Git. Generated C and runtime files are written to the selected output directory. Keep generated engine artifacts and proprietary game inputs out of public commits.

## Run the complete portable engine preparation sequence

When the local mirror contains both the supported engine and the matching game data manifest, run:

```sh
python3 app/Scripts/prepare_android_engine.py --mirror /absolute/path/to/game-mirror
```

This invokes the Linux wasm2c generator, writes its output into `app/Generated`, creates function-name/import metadata, then executes the same ordered non-Apple engine patch sequence used by upstream. It expects the mirror layout `b/8b0b5899ed/game.wasm` and `data/manifest.json`. It never uploads the mirror or commits generated proprietary files.

## Important boundary

This is only the Linux generator bootstrap. It does not yet run `engine_names.py`, `generate_imports.py`, the full ordered upstream engine patch sequence, Android NDK compilation, Vulkan rendering, audio/input adapters, or gameplay tests. The generated C is not yet wired into Android CMake. The Android ARM64 debug APK remains a Vulkan scaffold until those later stages are completed.
