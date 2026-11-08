# Android Vulkan port status

Branch: `android-mods-adaptation`

## Goal

Port the Five Star Port iOS application to Android while retaining its user-facing feature set and game behavior. Android uses Vulkan. This is a native port, not a WebView wrapper or a separate game implementation.

## Existing bootstrap

The Android bootstrap currently has a Kotlin landscape activity, an NDK/CMake ARM64-v8a library, Vulkan instance/device selection, Android surface and swapchain creation, clear-pass submission/presentation, and Android process memory metrics.

**This is not a playable GTA V build.** The current native renderer only clears/presents a Vulkan frame. It does not yet execute the game's D3D11-style command ring, draw the world, load game assets, run the WASM-derived engine, or expose the iOS app's feature set.

## Platform migration map

| iOS | Android |
|---|---|
| Swift/UIKit | Kotlin/Android UI |
| MetalKit/CAMetalLayer | SurfaceView/ANativeWindow + Vulkan |
| Metal queues, resources and render passes | Vulkan queues, buffers/images, descriptors and command buffers |
| WGSL to Metal library | WGSL to SPIR-V via Tint/Dawn |
| Files app/document picker | Storage Access Framework and app-private cache |
| AVFAudio/audio session | Oboe/AAudio |
| GameController/touch/gyro | Android input, gamepad and sensor APIs |
| iOS memory reporting | Android RSS/cgroup headroom |
| iOS lifecycle | Activity lifecycle and Android audio/input lifecycle |

## Milestones

1. Reproducible Android ARM64 build and Vulkan surface/swapchain smoke test, including pause/resume, resize and device-loss recovery.
2. Build Tint with WGSL reader and SPIR-V writer; emit SPIR-V and reflection metadata for the exact game shader set. The Android WGSL extraction/validation manifest stage is now implemented; it does not yet translate WGSL to SPIR-V.
3. Port the Metal renderer's command-ring consumer to Vulkan: resource lifetime, descriptors, pipeline cache, barriers, copies, queries, fences, readbacks and synchronization.
4. Build the same WASM-to-C/ARM64 engine pipeline for Android and replace Apple-specific file, thread, audio, input, memory, logging and lifecycle APIs.
5. Port feature parity: file discovery/integrity, archive extraction, downloads/block cache, Story, Sandbox, saves, settings, graphics presets, FPS/resolution, touch layout, controller, gyro, mods, cheats and logs.
6. Validate on Adreno 730 / Snapdragon 8+ Gen 1 for world streaming, texture residency, audio timing, frame pacing, thermals, suspend/resume and long sessions.

## Performance rules

- Target Vulkan 1.1 baseline; query optional extensions and provide fallbacks.
- Keep frames in flight bounded; avoid queue-wide idle waits during normal rendering.
- Avoid per-draw CPU allocations; use per-frame transient allocators.
- Compile/prepare shaders and pipelines outside gameplay where practical.
- Preserve original texture/world quality; expose render scale and frame cap as explicit settings.
- Do not claim FPS, stability or compatibility until measured on the target device.

## Build status

No playable Android APK is produced by this bootstrap. The next gate is a reproducible APK build and Vulkan smoke test, followed by shader translation and the renderer port. Game files remain external and user-supplied; no game data or proprietary binaries belong in the repository.\n## Android shader preparation status\n\n`app/Scripts/prepare_android_shaders.py` reuses the upstream shader-pack unpacker, validates stage/function payloads and constants JSON, and writes a deterministic SHA-256 manifest for the Android Tint/SPIR-V stage. A synthetic fixture test runs in CI. This is extraction and validation only: no Tint binary is built, no SPIR-V is emitted, and no game shaders are committed.\n