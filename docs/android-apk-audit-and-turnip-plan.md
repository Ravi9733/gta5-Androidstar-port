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


### Repository source search result

GitHub code searches on the repository's default branch for `vkCreateInstance`, `Android Vulkan`, `site.fivestarport.app`, `PGStartGraphics`, and `VK_KHR_android_surface` returned no indexed source matches. This reinforces that the Android Vulkan implementation which produced the supplied APK is not currently available through the repository's indexed source files. Code search indexes the default branch, so this result is not proof that no source exists in any unindexed branch or external upstream.


## Official Five Star Port Android release and upstream-source check (2026-11-18)

The official project site was checked against the APK manifest:

- The official Android page lists version **1.6.4**, released **2026-11-17**, for Android 10+, arm64, Vulkan graphics. Its APK size is described as about 200 MB, consistent with the supplied ~197 MiB package.
- The page recommends Vulkan 1.3 and Adreno 7xx/8xx or Mali-G7xx/Immortalis, with 8 GB RAM or more. It reports beta examples around 40 FPS at 540p on Snapdragon 8 Elite/12 GB, 45 FPS at 540p on an Adreno 735/12 GB, and 50 FPS at 720p on Mali-G720/8 GB. These are publisher-reported examples, not measurements on the user's Snapdragon 8+ Gen 1 / Adreno 730.
- The same page says the project is open source only **up to v1.5.2** and links separate iPhone and Mac source repositories. The supplied APK is v1.6.4-android, so the public source statement does not establish source availability for this exact Android binary.
- The public iPhone repository source describes a Metal renderer and iOS/Xcode build, which is not the Android Vulkan implementation found in the APK.
- The official FAQ's texture-loading advice explains that missing/grey ground and blurry buildings generally indicate memory pressure, often from resolution or texture settings being too high; it recommends Auto resolution and the preset's own texture setting, plus closing other apps. This FAQ material is written primarily for iOS, so its Apple-specific entitlement advice must not be applied to Android.
- The FAQ also documents that high texture quality consumes memory, and that overheating slows asset loading. These are plausible investigation areas, not proof of the exact cause on this Android build.

### Additional repository screening

The connected GitHub account's accessible repositories were enumerated. Besides this gta5-Androidstar-port mirror, the account has gta-v-native-android and FivePocket repositories. Their READMEs describe unrelated experimental iOS/Metal work and an Xbox 360 static-recompilation project, respectively; neither identifies itself as the source of site.fivestarport.app or libfivestar.so. GitHub code searches across the accessible account for site.fivestarport.app, libfivestar, and Engine_setSurface returned no matching indexed source.

### Updated conclusion / next safe action

We have now matched the APK to the official Five Star Port Android release/version family, but not to its build source. The official public-source cutoff (v1.5.2) is older than the supplied v1.6.4-android APK. Therefore there is still no source-backed way to modify its renderer or safely add an in-app Turnip loader. The next practical engineering route is a runtime-only diagnostic on the rooted Adreno 730 phone: capture the app's Vulkan driver/device identity, Vulkan capability/extension list, memory and thermal state, and the app's own log immediately after a texture-streaming slowdown. This can determine whether the system driver is already Turnip and separate memory/thermal/streaming pressure from a renderer defect without changing the APK or game data.

No APK was rebuilt, installed, patched, or modified in this step. Existing game files and saves remain untouched.


## Turnip integration feasibility and Adreno 730 driver selection (2026-11-18)

The latest upstream/community Turnip release was checked on 2026-11-18:

- The412Banner/Banners-Turnip publishes `Turnip-v27.0.0-20261118-r2.zip` for Adreno 6xx/7xx, including Adreno 730. The release is an automated Mesa-main build checked by CI but explicitly not device-tested. It is therefore a candidate driver package, not a verified performance upgrade for this app/device.
- The package variant intended for Android app integration is the standard X11/AdrenoTools ZIP. Wayland and Linux-runtime variants are not interchangeable with an Android app's in-process Vulkan driver.
- `libadrenotools` documentation states that each app must explicitly integrate the library; it is not a system-wide driver installer. A standalone Turnip ZIP cannot make an app use Turnip if that app has no custom-driver loading path.
- The supplied `site.fivestarport.app` APK's stripped native library has no evidence of AdrenoTools/Turnip loader integration. Its real Vulkan renderer imports alone do not provide a safe hook to redirect Vulkan loading.

### Decision

Do not try to force this APK onto Turnip using global `VK_DRIVER_FILES`, system library replacement, Magisk/KernelSU overlays, or binary patching. These are not a supported substitute for an app-level AdrenoTools integration and could break Vulkan apps or the device UI. The correct implementation requires the matching Android source/build inputs and an explicit per-app driver loader before Vulkan instance creation.

Candidate to evaluate only after that hook exists: standard Adreno 6xx/7xx Turnip v27.0.0-20261118-r2, not A8xx, A710/A720/A722 test, OneUI A740, Wayland, or Linux packages. Preserve system-driver fallback and allow the user to switch back without touching game data.

This is a source/driver compatibility decision only. No APK, device driver, root module, or game data was changed, and no performance gain is claimed.


## Upstream source audit: public v1.5.2 repository vs Android v1.6.4 APK (2026-11-18)

- kevinerikjs/gta5-ios-port README describes an iPhone-only WebAssembly-to-ARM64 build with a native Metal renderer. Its build instructions target a Mac/Xcode workflow.
- The upstream FAQ explicitly says there is no Android version in that source project. GitHub code search in the upstream default branch for VK_KHR_android_surface, vkCreateInstance, libfivestar, Android Vulkan renderer, and build_android_app.sh found no matching Android Vulkan implementation.
- Android-related strings found in the source search were incidental libarchive header comments/conditional definitions, not an Android app or renderer.
- The Android landing page advertises an Android 1.6.4 APK, but the upstream public source README/FAQ and source cutoff statement do not expose a matching Android project. Matching version numbers or a similar product name are not sufficient to establish that public iOS code can build the supplied Android APK.

### Engineering consequence

Do not transplant the upstream Metal renderer into the Android APK or treat the iOS source as a drop-in Android port. It would require a real Android platform layer, Vulkan renderer, Android build system, JNI bridge, asset/thread/memory integration, and matching engine revision. The Android APK's libfivestar.so exports a different JNI surface and imports Vulkan directly. A speculative rewrite would not preserve compatibility or prove the game can boot.

### Next work queue (source-side, safe)

1. Keep searching public branches/forks/releases for the exact Android package/JNI identifiers and matching native library provenance.
2. Inspect the public v1.5.2 source build scripts and identify reusable platform-neutral engine components, clearly separating them from Apple Metal, Objective-C, and Xcode-only code.
3. Prepare an Android Vulkan/AdrenoTools integration design with system-driver fallback and no global driver overrides, but do not wire it into an unrelated binary.
4. Only once matching Android source/build inputs are found, implement and compile the Turnip loader, then verify driver identity and rendering on Adreno 730. Until then, do not claim Turnip is active or performance improved.

No APK was rebuilt, installed, patched, or modified in this step. Existing game files and saves remain untouched.
