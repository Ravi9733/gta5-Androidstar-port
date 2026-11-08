#!/usr/bin/env python3
"""Run the portable WASM generation and non-Apple engine preparation stages.

This deliberately stops before Apple-only Metal/Blink/Xcode builds and before Android NDK
compilation. The player's local game mirror is read in place; proprietary files are not copied
into the repository or uploaded by this script.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

PATCH_STAGES = [
    "prepare_plain_engine.py:off",
    "prepare_heap_diagnostics.py",
    "prepare_memory_pressure.py",
    "prepare_virtual_pad.py",
    "prepare_movie_wait.py",
    "prepare_tv_without_video.py",
    "prepare_bink_queue.py",
    "prepare_menu_shadow.py",
    "prepare_settings_restart.py",
    "prepare_missing_text.py",
    "prepare_help_label_check.py",
    "prepare_clip_offsets.py",
    "prepare_handler_frame.py",
    "prepare_netlog_flush.py",
    "prepare_input_exclusive.py",
    "prepare_restore_flush.py",
    "prepare_destroyed_phases.py",
    "prepare_heap_sizes.py",
    "prepare_dependency_guard.py",
    "prepare_script_request_guard.py",
    "prepare_missing_txd.py",
    "prepare_missing_scaleform.py",
    "prepare_mod_archives.py",
    "prepare_asi_natives.py",
    "prepare_asi_script_tick.py",
    "prepare_unchecked_memory.py",
    "prepare_trap_context.py",
    "prepare_scenario_blockers.py",
    "prepare_level_index.py",
    "prepare_fixup_guard.py",
    "prepare_format_strings.py",
    "prepare_video_memory.py",
    "prepare_plain_dsp.py",
    "prepare_cash_stats.py",
    "prepare_native_opus.py",
    "prepare_native_inflate.py",
    "prepare_render_list_guard.py",
    "prepare_stored_vehicle_mods.py",
    "prepare_hud_layout.py",
    "prepare_stale_texture_slots.py",
    "prepare_control_trace.py",
    "prepare_pause_ready.py",
    "prepare_streaming_health.py",
    "prepare_evict_protect.py",
    "prepare_mesh_release.py",
    "prepare_txd_parent.py",
    "prepare_stream_alloc_trace.py",
    "prepare_request_trace.py",
    "prepare_heap_return.py",
    "prepare_user_music.py",
    "prepare_no_debug_text.py",
    "prepare_scaled_look.py",
    "prepare_thread_tables.py",
    "prepare_frame_limit_sleep.py",
    "prepare_script_fault_abort.py",
    "prepare_occlusion_late.py",
    "prepare_d3d_storage_stats.py",
    "prepare_d3d_lazy_copies.py",
    "prepare_memory_file_create.py",
    "prepare_longjmp.py",
    "prepare_trap_guards.py",
    "prepare_stale_cbuffers.py",
    "prepare_plain_engine.py",
    "prepare_memory_base.py",
    "prepare_shared_tables.py",
]


def run(command: list[str], cwd: Path) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mirror", type=Path, required=True,
                        help="local game mirror containing b/8b0b5899ed/game.wasm and data/manifest.json")
    parser.add_argument("--tools-dir", type=Path, default=Path("Tools/android-wasm2c"))
    args = parser.parse_args()

    app = Path(__file__).resolve().parents[1]
    mirror = args.mirror.expanduser().resolve()
    wasm = mirror / "b/8b0b5899ed/game.wasm"
    manifest = mirror / "data/manifest.json"
    if not wasm.is_file():
        raise SystemExit(f"Missing engine input: {wasm}")
    if not manifest.is_file():
        raise SystemExit(f"Missing game data manifest: {manifest}")

    generated = app / "Generated"
    generated.mkdir(parents=True, exist_ok=True)
    tools = args.tools_dir.expanduser().resolve()

    run([
        sys.executable, str(app / "Scripts/prepare_android_wasm2c.py"),
        "--engine-wasm", str(wasm),
        "--output-dir", str(generated),
        "--tools-dir", str(tools),
    ], app)

    # These two stages must run against the generated game headers before any engine patcher.
    run([sys.executable, str(app / "Scripts/engine_names.py"), "--wasm", str(wasm)], app)
    shutil.copy2(manifest, generated.parent / "Resources/manifest.json")
    run([sys.executable, str(app / "Scripts/generate_imports.py")], app)

    for stage in PATCH_STAGES:
        name, separator, argument = stage.partition(":")
        script = app / "Scripts" / name
        if not script.is_file():
            raise SystemExit(f"Required upstream preparation stage is missing: {script}")
        command = [sys.executable, str(script)]
        if separator:
            command.append(argument)
        run(command, app)

    expected = [
        generated / "game.h",
        generated / "engine-functions.h",
        generated / "Imports.c",
        generated / "funcnames.tsv",
        generated / "wasm-rt-impl.c",
        generated / "wasm-rt-impl.h",
        generated / "game-plain-impl.h",
    ]
    missing = [str(path) for path in expected if not path.is_file()]
    if missing:
        raise SystemExit("Engine preparation incomplete; missing outputs:\n" + "\n".join(missing))

    print("Portable engine generation and upstream patch sequence completed.")
    print("Not yet done: Android NDK compilation, Android platform adapters, Vulkan renderer, audio/input, gameplay tests.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
