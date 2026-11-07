# iOS v1.5.2 → Android Vulkan porting reference

This document is the source-of-truth checklist for the Android port. Port behavior from the public iOS v1.5.2 source; do not invent a parallel feature design where the iOS implementation already defines the behavior.

## Source baseline inspected

- Upstream: https://github.com/kevinerikjs/gta5-ios-port/tree/v1.5.2
- The upstream README describes an ARM64 WebAssembly game engine, native Metal renderer, Story and Sandbox modes, touch/controller/gyro controls, graphics presets, game-file integrity/install flows, and PC mod installation.
- The repository does not contain GTA game files or game code. Android must use the user's own compatible game files.
- v1.5.2 is the public source baseline. Do not assume private/newer iOS engine source is available.

## Feature parity map

| iOS source / feature | Android port requirement | Status |
|---|---|---|
| `HomeView.swift`, `NativePortViewController.swift` | Recreate home, mode selection, play lifecycle, and game surface using Android UI/lifecycle | Not ported |
| Story / Sandbox mode and save handling | Preserve the same mode semantics, save paths and resume behavior | Not ported |
| `GameFilesViewController.swift`, `GameFolder.swift`, `GameInstall.swift`, `GameInstaller/*` | SAF folder/file selection, resumable archive install, manifest/integrity checks, progress and recovery | Not ported |
| `GraphicsOptions.swift`, `GameSettings.swift` | Same presets and setting ranges; translate renderer options to Vulkan capabilities and Adreno memory budget | Not ported |
| `ModsViewController.swift`, `ModInstaller.swift`, `ModKeys*.swift` | Install/disable/remove mods, safe archive extraction, game-file-index placement, OIV handling and script-key UI | Import/list foundation only |
| `NativeCheats.swift` | Preserve supported native cheats through an Android engine bridge | Not ported |
| `GyroInput.swift`, touch/HUD layout, controller handling | Android SensorManager, touch layout editor, gamepad mapping and aim/steering parity | Not ported |
| `SelfRadio.swift` | Android media decoding and user music folder integration | Not ported |
| `LogUploader.swift` / run logs | Local logs, crash/previous-run diagnostics and share/export | Not ported |
| Metal renderer and precompiled shader assets | Vulkan backend: resource lifetime, descriptors, pipelines, barriers, command submission, sync/readback, shader conversion and presentation | Clear-frame Vulkan bootstrap only |

## Mod system details confirmed in iOS source

The iOS `ModInstaller.swift` is more than a file picker:
- It unpacks ZIP/RAR/TAR/GZ using the bundled archive reader shim; rejects unsafe traversal paths and password-protected archives.
- It plans mod files and maps loose files using the engine-generated `game-file-index.txt`.
- It handles OpenIV `.oiv` packages, Script Hook V `.asi` files, existing game-layout paths and add-on `dlc.rpf` packs.
- It keeps each mod in its own folder and supports enabled/disabled state plus removal; changes are applied on next game start.
- Script mod key definitions are surfaced by the Mods UI.

Android's current `AndroidModStore` only copies selected files into app-private storage and lists them. It does not yet unpack, install, index, activate, or execute mods. A Windows `.asi` binary cannot simply be loaded into Android; it needs a compatible Android native ABI/runtime bridge and must be treated as unsupported until that exists.

## Vulkan / Android engine gates

1. Keep the current Vulkan clear-frame smoke test as a bootstrap only.
2. Port the iOS engine's Metal command consumer to Vulkan before claiming game rendering: GPU resources, descriptor binding, pipeline/state mapping, barriers, copies, queries, fences/readbacks, frame pacing and swapchain lifecycle.
3. Port the engine build/runtime and Apple-specific platform calls to Android ARM64; connect the actual WASM/engine startup path and shader assets.
4. Port game file indexing/installation and mods only against the Android engine's real file-open and mod-loading interfaces.
5. Validate on Adreno 730 / Snapdragon 8+ Gen 1: long open-world streaming, memory pressure, texture residency, audio, thermals, suspend/resume and sustained frame pacing.

## Build policy

APK builds are manual-only. Ordinary source/documentation commits must not trigger repeated APK builds. Run the Android CI build deliberately when a meaningful integration milestone is ready, and publish an APK only after the build and artifact checks pass.
