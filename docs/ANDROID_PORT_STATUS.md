# Android port status

This branch is the Android port workstream. The iOS app remains unchanged.

## Verified upstream architecture

- The game engine is translated from the supplied WebAssembly build to C and compiled for ARM64.
- The iOS host is Objective-C/Swift and calls the engine through `EngineHost.h`.
- Graphics enter through `NativeGraphics.h`; the current implementation in `MetalGraphicsHost.mm` forwards the game's command ring to the Metal renderer.
- The iOS build depends on Apple frameworks (UIKit, Metal, MetalKit, GameController, AVFAudio, IOSurface) and iOS-specific libarchive packaging.

## Android work plan

1. Android ARM64 app + NDK/JNI build pipeline.
2. Android host services: lifecycle, files/asset access, audio, input/controller, memory/thread primitives, logging.
3. Vulkan renderer implementing the same command-ring contract exposed by `PGStartGraphics`; Metal remains iOS-only.
4. Shader pipeline for Android Vulkan and device capability checks.
5. Real game-file validation and runtime smoke tests on Adreno hardware.
6. Performance and stability tuning on Snapdragon 8+ Gen 1.

## Current milestone: scaffold only

The `android/` project builds an ARM64 Android shell and a JNI library. It intentionally reports that the port is not playable yet. The game engine, host services, Vulkan renderer, shader compiler, and game-file integration have **not** been ported in this milestone.

No copyrighted game files are included. Testing requires game files the user legally owns.
