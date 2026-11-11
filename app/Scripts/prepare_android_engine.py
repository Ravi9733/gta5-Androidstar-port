#!/usr/bin/env python3
"""Apply Android diagnostics hooks to the generated WABT C runtime.

The generated runtime remains upstream WABT code apart from a small callback
immediately before its normal trap abort.  The Android host provides
pg_record_trap() and can record the trap code in logcat before the process
terminates.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


_TRAP_SIGNATURE = re.compile(
    r"(?m)^(?P<indent>[ \t]*)void[ \t]+wasm_rt_trap[ \t]*"
    r"\([ \t]*wasm_rt_trap_t[ \t]+code[ \t]*\)[ \t]*\{"
)
_CALLBACK = "pg_record_trap(code)"


def install_trap_callback(runtime_path: Path | str) -> None:
    """Insert the Android trap callback once, failing safely on unknown WABT."""
    path = Path(runtime_path)
    try:
        source = path.read_text(encoding="utf-8")
    except OSError as error:
        raise SystemExit(f"Cannot read generated WABT runtime: {error}") from error

    if _CALLBACK in source:
        return

    match = _TRAP_SIGNATURE.search(source)
    if not match:
        raise SystemExit(
            "Cannot install native trap diagnostics: wasm_rt_trap(wasm_rt_trap_t code) "
            "was not found in the generated WABT runtime."
        )

    # Keep the callback inside wasm_rt_trap and before abort(), preserving the
    # runtime's existing trap/termination semantics.
    # The generated WABT runtime is C; declare the Android host callback before
    # the first call so strict C11 compilers do not reject an implicit function.
    declaration = "void pg_record_trap(wasm_rt_trap_t code);\\n"
    if "pg_record_trap(wasm_rt_trap_t code);" not in source:
        source = source[: match.start()] + declaration + source[match.start() :]
        match = _TRAP_SIGNATURE.search(source)
        assert match is not None

    opening_brace = match.end() - 1
    insertion = (
        "\\n"
        f"{match.group('indent')}  pg_record_trap(code);"
    )
    patched = source[: opening_brace + 1] + insertion + source[opening_brace + 1 :]
    if patched.count(_CALLBACK) != 1:
        raise SystemExit("Cannot install native trap diagnostics: ambiguous runtime.")

    path.write_text(patched, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path, help="path to generated wasm-rt-impl.c")
    args = parser.parse_args(argv)
    install_trap_callback(args.runtime)
    print(f"Installed Android trap diagnostics in {args.runtime}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
