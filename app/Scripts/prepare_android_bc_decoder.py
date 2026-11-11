#!/usr/bin/env python3
"""Fetch the pinned MIT bcdec header needed for Android BC texture fallback."""
import argparse
import pathlib
import urllib.request

COMMIT = "80859ed3b7afb1c527a2a99d70c61457bea72d0c"
URL = "https://raw.githubusercontent.com/iOrange/bcdec/" + COMMIT + "/bcdec.h"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(URL, headers={"User-Agent": "gta5-android-port-build"})
    with urllib.request.urlopen(request, timeout=30) as response:
        content = response.read()
    if b"bcdec_bc1" not in content or b"bcdec_bc7" not in content or len(content) < 10000:
        raise SystemExit("Pinned bcdec header failed integrity checks")
    output.write_bytes(content)
    print("Prepared pinned bcdec header (" + str(len(content)) + " bytes)")

if __name__ == "__main__":
    main()
