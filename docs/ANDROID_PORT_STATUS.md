# Android port status

This branch is the Android port workstream. The iOS app remains unchanged.

## Verified upstream architecture

- The game engine is translated from the supplied WebAssembly build to C and compiled for ARM64.
- The iOS host is Objective-C/Swift and calls the engine through `EngineHost.h`.
- Graphics enter through `NativeGraphics.h`; the current implementation in `MetalGraphicsHost.mm` forwards the game's command ring to the Metal renderer.
- The iOS build depends on Apple frameworks (UIKit, Metal, MetalKit, GameController, AVFAudio, IOSurface) and iOS-specific libarchive packaging.

## Android work completed

- ARM64 Android application and NDK/CMake build scaffold.
- Android `SurfaceView` lifecycle bridged to native code.
- Vulkan instance and Android surface creation.
- Physical-device selection requiring a graphics + presentation queue and swapchain support.
- Vulkan logical device, swapchain, image views, render pass, framebuffers, command buffers, semaphores and fence.
- Choreographer-driven acquire / clear / submit / present loop, with resize and pause/resume handling.
- Surface-resize failures now clean up partially-created swapchain resources and report a recoverable status.
- Frame-acquire, frame-fence, and swapchain-rebuild failures now report Vulkan errors and avoid retaining a partially-built swapchain.
- Vulkan frame-fence waits and swapchain-image acquisition use a bounded 1-second timeout, avoiding indefinite waits on the Android Choreographer/UI thread when a driver stalls.
- `VK_ERROR_SURFACE_LOST_KHR` during acquire or present attempts to recreate the Android surface and swapchain; failed recovery leaves a clear detach/reattach status.
- Vulkan physical-device, extension, surface-support, surface-format and swapchain-image queries now check their return codes before using results.
- Android declares Vulkan 1.1 and the required graphics capability in its package manifest, so unsupported devices are filtered by the platform.
- Activity destruction stops frame callbacks and releases any still-attached native Vulkan surface.
- Android native memory metrics: total physical RAM and current process RSS from `/proc/self/statm`. RSS is explicitly not treated as available process headroom or an Android memory limit.
- Android cgroup memory probe checks finite cgroup v2/v1 limits and reports remaining cgroup headroom; it returns 0 when no reliable finite limit is exposed. This is a separate metric, not yet wired into the upstream engine's memory guard.

The current Vulkan milestone presents a changing clear-color frame only. It validates the Android Vulkan presentation path; it is not yet the GTA V renderer.

## Remaining port work

1. Port Android host services: lifecycle, files/asset access, audio, input/controller, memory/thread primitives and logging. `EngineHost.c` still has Apple-only `TargetConditionals.h`, `os/proc.h`, Mach task/VM APIs and must be platform-guarded before it can compile under the Android NDK.
2. Implement the Vulkan equivalent of the game's native graphics command-ring consumer currently implemented by the Metal renderer.
3. Port/compile the shader pipeline for Android Vulkan and verify texture formats, BC compression and render-target behavior.
4. Integrate the portable ARM64 game engine and connect it to the Android host and Vulkan backend.
5. Add the iOS-equivalent menus, settings, touch controls and controller UI, backed by the shared engine state.
6. Validate legally owned game files and run runtime smoke tests on Adreno hardware.
7. Tune memory, streaming, frame pacing and stability for Snapdragon 8+ Gen 1.

No copyrighted game files are included. Testing requires game files the user legally owns.
