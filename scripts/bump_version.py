#!/usr/bin/env python3
"""Increments FW_VERSION in include/Config.h.

Version format: major.minor.patch.build (e.g. 45.4.0.0). The chosen part is
incremented and the following ones are reset to 0:

    bump_version.py minor   45.4.2.1 -> 45.5.0.0
    bump_version.py patch   45.4.2.1 -> 45.4.3.0
    bump_version.py build   45.4.2.1 -> 45.4.2.2
    bump_version.py major   45.4.2.1 -> 46.0.0.0

Prints "version=<new version>" (GitHub Actions output format).
Use --dry-run to only print the new version.
"""

import argparse
import os
import re
import sys

PARTS = ["major", "minor", "patch", "build"]
CONFIG = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "include", "Config.h")
PATTERN = re.compile(r'(#define\s+FW_VERSION\s+")(\d+(?:\.\d+){3})(")')


def bump(version, part):
    numbers = [int(n) for n in version.split(".")]
    if len(numbers) != 4:
        raise ValueError("Version must have 4 numbers: " + version)
    i = PARTS.index(part)
    numbers[i] += 1
    for j in range(i + 1, 4):
        numbers[j] = 0
    return ".".join(str(n) for n in numbers)


def main():
    parser = argparse.ArgumentParser(description="Increments the firmware version")
    parser.add_argument("part", choices=PARTS)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--config", default=CONFIG)
    args = parser.parse_args()

    with open(args.config) as f:
        text = f.read()
    match = PATTERN.search(text)
    if not match:
        sys.exit("FW_VERSION not found in " + args.config)

    new_version = bump(match.group(2), args.part)
    if not args.dry_run:
        with open(args.config, "w") as f:
            f.write(PATTERN.sub(lambda m: m.group(1) + new_version + m.group(3), text, count=1))
    sys.stderr.write("FW_VERSION %s -> %s\n" % (match.group(2), new_version))
    print("version=" + new_version)


if __name__ == "__main__":
    main()
