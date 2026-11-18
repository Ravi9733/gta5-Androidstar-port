# Android APK audit and Turnip optimization plan

## Scope

This branch tracks investigation of the user-provided Android package:

- Filename: `GTA V Mobile Native Port Offline-Online Mode.apk`
- Size: 206,000,062 bytes
- SHA-256: `45f7e7c43f59906b91d75a757d7d1b4bec07b7c1ab88f98bc630656b135a367d`
- Native ABI inspected: `arm64-v8a`
- Main native library: `lib/arm64-v8a/libfivestar.so`
- Native library size: 116,060,232 bytes
- ELF: AArch64, Android API 29 target, NDK r30 build, stripped
- Dynamic dependency includes `libvulkan.so`

## What this proves (and does not prove)

The APK contains an ARM64 native engine and links against Android's Vulkan loader. This establishes Vulkan API dependency, but **does not prove** that the app exposes driver selection, supports custom Vulkan driver loading, or has AdrenoTools/Turnip integration.

A static APK inspection cannot establish real-world FPS, frame pacing, thermal behavior, memory pressure, or texture-streaming performance. Those require runtime logs and controlled device tests.

The native library is stripped. No original C/C++ source or symbols sufficient to safely patch the renderer have been recovered from this APK. Replacing or binary-patching the game engine without source-level understanding would risk crashes, corrupted saves, or rendering regressions.

## Development goals

1. Identify the exact Android application package/version and the Java/Kotlin entry points.
2. Map the Vulkan initialization path and determine whether the project has a supported custom-driver hook.
3. If a compatible hook exists, evaluate a maintained AdrenoTools integration with:
   - explicit user-selected driver package import,
   - validation of ABI and driver files,
   - safe fallback to the system Vulkan driver,
   - clear error reporting and rollback.
4. Profile frame pacing, GPU/CPU utilization, memory use, shader compilation, asset streaming, and thermal throttling on Adreno 730.
5. Make measured, reversible optimizations. Preserve original game data and saves.
6. Build and test a signed debug/release APK only from reproducible source and legally distributable components.

## Device test target

Primary target: Snapdragon 8+ Gen 1 / Adreno 730 / 8 GB RAM.

Tests should record:
- average FPS and 1% low / frame-time percentiles,
- GPU and CPU clocks/load,
- memory footprint and allocation failures,
- temperature and sustained performance after 10-15 minutes,
- shader-cache warm/cold behavior,
- texture/geometry streaming delays,
- Vulkan validation or driver errors.

## Current status

- [x] APK archive and native library identified.
- [x] Confirmed ARM64 native library.
- [x] Confirmed dynamic dependency on `libvulkan.so`.
- [ ] Identify application ID, version, and Android entry points.
- [ ] Inspect Java/Kotlin bytecode and native JNI boundaries.
- [ ] Map Vulkan instance/device creation and extension usage.
- [ ] Confirm whether custom driver loading can be integrated safely.
- [ ] Implement driver selection only after the hook is verified.
- [ ] Add performance telemetry and controlled benchmark scenes.
- [ ] Build, install, and validate on physical Adreno 730 hardware.

## Safety and compatibility constraints

- Do not distribute proprietary game data, assets, or the supplied APK in this repository.
- Do not overwrite or delete the user's existing installation, game files, or saves during tests.
- Keep system-driver fallback available.
- Never describe a build as "perfect" or "optimized" until repeatable device tests support the claim.
