# Android mod subsystem — first adaptation stage

This directory is the Android-side starting point for the Five Star Port mod system. It does not copy the iOS Swift/Objective-C runtime into Android.

## Included in this stage

- SAF content URI import of ZIP mod packages.
- Safe extraction into an app-private staging directory, with path-traversal and extraction-size limits.
- Per-mod directory plus a JSON manifest.
- Enable/disable state recorded in the manifest.
- Original PC-style paths preserved, including update/x64/dlcpacks/<pack>/dlc.rpf.

## Not implemented yet

- RAR/7z/OIV extraction.
- RPF parsing, archive overlay, game file-index integration, or actual asset substitution.
- Vulkan/game-engine hook that consumes the enabled mod list.
- Blink/Windows x64 ASI execution and Script Hook V API.
- Trainer key/touch-input bridge.

A mod appearing in this manager does not mean it is active in-game. The engine-side overlay must be implemented and tested before claiming vehicle, texture, map, or script mods work.

## Integration

Add AndroidModImporter.kt to the Android app's Kotlin source set. Pass an app-private directory such as context.filesDir/mods as the store root. Launch the game only after the engine-side mod overlay consumes the enabled mod list.

The importer accepts a ZIP URI selected through Android's Storage Access Framework, rather than arbitrary filesystem paths.
