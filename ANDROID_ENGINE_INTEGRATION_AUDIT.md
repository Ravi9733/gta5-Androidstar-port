# Android engine integration audit

Branch: `android-mods-adaptation`

## What the repository contains

The iOS source tree contains the native game host (`app/Source/EngineHost.c/.h`), native runtime helpers, file/streaming/audio/input code, and the Metal renderer. The iOS native build is not a simple compile of `game.wasm`: `app/Scripts/build_native.sh` runs a long sequence of source-generation and compatibility patch scripts, builds Opus and Blink, translates the shader set to Metal, then invokes Xcode.

The Android app currently builds a separate NDK library from:
- `android/app/src/main/cpp/port_status.cpp`
- `android/app/src/main/cpp/pg_android_platform.cpp`

That native library creates a Vulkan instance/device/swapchain and clears/presents frames. It does not compile or link `EngineHost.c`, the generated WASM-to-C engine, the game file manifest, the Metal renderer, or the native audio/input runtime.

## WASM check now present

The Android dashboard can select a user-provided `game.wasm` through Android's document picker. `AndroidGameProbe` reads only the first eight bytes and checks the standard WebAssembly magic and version header. The selected URI and validation result are saved in app preferences.

A valid header is only a file-format check. It does not establish that the file is the expected Five Star Port build, that all game data is present, or that Android can execute it.

## Required engine port sequence

1. **Reproducible bootstrap build:** build the Android ARM64 debug APK and run a Vulkan surface/pause/resume smoke test.
2. **Generated engine inputs:** define a reproducible, legally supplied input pipeline for the exact game WASM, imports, generated C sources, function metadata and game file manifest. Do not commit proprietary game data or binaries.
3. **Portable engine host:** split platform-neutral engine code from UIKit, Metal, AVFoundation, Apple file APIs and Apple lifecycle assumptions. Keep the public `PG*` host contract stable where possible.
4. **Android platform adapters:** implement file access, threading/futex, memory pressure, audio (AAudio/Oboe), controller/touch input, logging, lifecycle and persistent save paths.
5. **Vulkan renderer:** replace the Metal command-ring consumer and resource model with Vulkan buffers/images/descriptors/pipelines/barriers/synchronization. Translate the exact WGSL shader set to SPIR-V and preserve reflection/bindings.
6. **End-to-end game validation:** load the original user-supplied game files, validate manifest sizes, boot Story and Sandbox, and test streaming, saves, audio, controls and long sessions on Adreno 730.

## Current limitations

- The game engine is not yet linked into the Android app.
- The current Play action only reveals the Vulkan surface; it does not boot GTA V.
- The WASM probe does not compile or execute WASM.
- The current mods importer stores selected files but does not mount RPF archives or load ASI plugins in-game.
- No APK build or on-device test has been completed for this branch yet.

Do not advertise this branch as a playable Android GTA V port until the engine and renderer milestones are built and tested.

## Build-input inventory (2026-11-08)

A second source-tree pass confirmed that the Android branch does not currently contain the generated engine inputs needed to compile the host:

- `app/Source/HostInternal.h` includes `game.h` and `wasm-rt-exceptions.h`; neither generated header is present in the tracked tree.
- `app/Source/EngineHost.c` also includes `wasm-rt-impl.h` and `engine-functions.h`; these generated headers are not present in the tracked tree either.
- The tracked tree contains `app/Scripts/generate_imports.py`, `app/Scripts/prepare_plain_engine.py`, and the iOS `build_native.sh` pipeline, but no generated `game_*.c` / wasm2c engine C output and no `game.wasm` input.
- The existing Android CMake target still builds only the Vulkan bring-up and Android memory adapter. Adding `EngineHost.c` to that target now would fail at the missing generated-header boundary, before renderer/audio/platform integration is even reached.

### Immediate integration gate

Do not wire `EngineHost.c` into Android CMake until the project has a reproducible Android-safe generation step for the exact user-supplied game WASM and its generated C/header/import metadata. The generation step must produce and verify the expected headers and source list without committing proprietary game data. Once those inputs exist, compile the host as a separate staged target and resolve compiler errors by platform subsystem; do not hide missing engine inputs behind placeholder headers or stubs.

### CI status

Android ARM64 debug workflow run `42500006200` completed successfully on commit `d57f8d7ee9afeca8ec3bcf36a2441e7869d2c52d`. This verifies the current scaffold build only; it does not compile the engine host or demonstrate gameplay.
