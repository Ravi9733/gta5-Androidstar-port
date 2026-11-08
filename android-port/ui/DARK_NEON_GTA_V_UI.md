# Five Star Port Android — Dark Neon Gaming UI

Status: approved visual specification; implementation pending Android app module discovery.

## Visual direction
- Keep the approved Dark Neon Gaming UI layout and interaction model.
- Use GTA V / Los Santos imagery only. Do not use Vice City artwork, logos, skyline, or color treatment.
- Landscape-first, touch-friendly, dark charcoal surfaces with restrained purple and neon-green accents.
- Preserve the original Five Star Port identity and GTA V branding; do not invent a new game title.

## Main screen layout
1. Left navigation rail: Home, Story Mode, Sandbox, Performance, Mods & Patches, Self Radio, Settings.
2. Center hero area: GTA V Los Santos cinematic artwork, game title, concise status, and a prominent Play action.
3. Right information panel: detected game build, game-file validation, installed mod count, and current mode.
4. Bottom device strip: RAM use, GPU/device name, Vulkan availability, frame-rate readout.

## Interaction requirements
- Story Mode and Sandbox remain separate launch choices.
- Play is disabled until required game files are detected and validated.
- Mods & Patches opens the Android mod manager; import uses Android's Storage Access Framework and never assumes broad storage permission.
- Performance exposes only settings that the Android renderer actually supports.
- Vulkan status must come from runtime capability detection, never a hard-coded green indicator.
- UI status values must be real measurements or explicitly shown as unavailable.

## Motion and accessibility
- Subtle fades and short transitions; avoid expensive full-screen blur and continuous animated backgrounds.
- Large landscape touch targets, readable contrast, scalable text, and clear selected-navigation state.
- Provide reduced-motion behavior and avoid blocking the game launch path with decorative animation.

## Android implementation gate
Before adding UI source files, identify the actual Android application module, Gradle configuration, manifest, navigation/activity entry point, and renderer bridge in this repository. The current branch contains standalone mod-import/overlay starter files, but no verified Android app module has been located yet. Do not create a disconnected demo app or claim this specification is an integrated UI.
