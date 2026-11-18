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


## Source-to-APK compatibility check (2026-11-18)

The Android tree on this branch was inspected after the APK binary audit:

- The only Android Java source is `android/app/src/main/java/com/ravi/gta5port/MainActivity.java`.
- That activity calls `System.loadLibrary("gta5_android")` and declares JNI entry points such as `nativeCreate`, `nativeResize`, and `nativeRenderFrame`.
- The supplied APK instead contains `libfivestar.so`; its native library name does not match the Android source's expected `libgta5_android.so`.
- The repository tree contains no Android JNI C/C++ implementation, Android manifest, Gradle build files, or Vulkan renderer source for this Java shell.
- The repository's actual native graphics host is `app/Source/MetalGraphicsHost.mm`, and its renderer implementation is `app/Source/MetalRenderer/`. The native build script invokes Xcode for `iphoneos`, builds Metal shaders, and does not define an Android/Vulkan build.
- `app/Scripts/test_android_engine_pipeline.py` imports `app/Scripts/prepare_android_engine.py`, but that referenced script is absent from the inspected branch tree.

### Consequence

The current repository Android shell is **not yet a reproducible source match for the supplied APK**. The APK's Vulkan dependency alone is not enough to port the repository's Metal renderer to Android or safely inject Turnip. No renderer patch or APK rebuild should be attempted from this source state.

### Required next gate

Before implementation, obtain a source/build provenance match for the exact APK: package ID/version, signing/build metadata, and the source revision or native build inputs that produced `libfivestar.so`. Then establish a real Android JNI + Vulkan host and a reproducible Android build. Only after that can Turnip driver loading and Adreno 730 optimization be implemented and tested safely.


## APK manifest identification

A binary-XML string-pool inspection of the supplied APK's manifest recovered:

- Application ID: `site.fivestarport.app`
- Version name: `1.6.4-android`
- Application class: `site.fivestarport.app.App`
- Launcher activity: `site.fivestarport.app.MainActivity`
- Manifest includes the `android.hardware.vulkan.version` feature declaration.

This identifies the exact package namespace and launcher entry point, and confirms the manifest declares a Vulkan hardware feature. It still does not identify the Vulkan API version value or prove that Turnip/custom-driver injection is supported.


## Native Vulkan and JNI audit (2026-11-18, follow-up)

A second pass inspected the supplied APK's DEX files and the dynamic symbol table of `libfivestar.so`:

- The native library has direct undefined imports for the Vulkan API, including `vkCreateInstance`, `vkEnumeratePhysicalDevices`, `vkGetPhysicalDeviceProperties`, `vkCreateDevice`, queue/swapchain creation, graphics and compute pipelines, command submission, presentation, and GPU timestamp queries.
- This is stronger evidence than a `libvulkan.so` dependency alone: the APK's native library contains a real Vulkan renderer path.
- Imported/used extension identifiers found in the binary include `VK_KHR_android_surface`, `VK_KHR_surface`, `VK_KHR_swapchain`, `VK_KHR_timeline_semaphore`, `VK_KHR_synchronization2`, `VK_KHR_dynamic_rendering`, `VK_KHR_create_renderpass2`, `VK_KHR_depth_stencil_resolve`, `VK_EXT_extended_dynamic_state`, and `VK_EXT_extended_dynamic_state2`.
- JNI exports are named for package `site.fivestarport.app`; the Engine bridge includes `setSurface`, `configure`, `start`, `setMemoryCap`, `setPresentInterval`, `setSharperUpscaling`, `renderedFramesPerSecond`, and `shownFrameHistogram`. The library also exports Archive, Input, Torrent, and Unpacker JNI bridges.
- DEX class/string inspection identifies app components such as `MainActivity`, `GraphicsPage`, `DevicePage`, `PerformanceTarget`, and `GraphicsPreset`. User-facing settings strings include 30 FPS, 60 FPS, Unlocked up to 120, Auto resolution, Balanced/Performance/Quality/Ultra presets, a memory cap, and an adaptive behavior that lowers draw distance, pedestrians, and traffic when the phone is hot.
- The native binary contains a pipeline-cache file name, `/vulkan-pipelines.bin`, and GPU timestamp-query calls. These are promising existing foundations for profiling and shader/pipeline cache investigation.

### Turnip status after this pass

The APK definitely uses Vulkan, but this inspection did not find AdrenoTools, Turnip, `VK_DRIVER_FILES`, or `VK_ICD_FILENAMES` strings. This does **not** rule out Android system-level driver selection or a driver override performed externally; it means a built-in Turnip picker/loader is not established by the binary evidence. Do not inject a driver by patching the stripped library.

### Source matching remains the blocker

The APK exports JNI names under `site.fivestarport.app` and packages `libfivestar.so`. The repository's inspected Android Java shell instead uses package `com.ravi.gta5port` and loads `gta5_android`; its native graphics implementation in the inspected source is Metal/Xcode-based. Therefore the supplied APK's actual Android build source is still not represented by the inspected Android shell. The next safe step is to locate or recover the matching Android source/build provenance, not to edit unrelated JNI code.
