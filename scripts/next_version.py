#!/usr/bin/env python3
"""Computes the version of the next release from the git tags.

    next_version.py <major|minor|patch|build> [--config include/Config.h]

- The latest tag vX.Y.Z.W is incremented in the chosen part (the following
  parts go back to 0): build 45.4.0.0 -> 45.4.0.1, minor 45.4.2.1 -> 45.5.0.0
- No tag yet: FW_VERSION of include/Config.h is used as it is (first release)
- FW_VERSION in Config.h higher than the computed version: Config.h wins
  (to jump to a new version by hand, e.g. 46.0.0.0)

Prints "version=<version>" (GitHub Actions output format).
"""

import argparse
import os
import re
import subprocess
import sys

PARTS = ["major", "minor", "patch", "build"]
CONFIG = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "include", "Config.h")
TAG = re.compile(r"^v(\d+)\.(\d+)\.(\d+)\.(\d+)$")
CONFIG_VERSION = re.compile(r'#define\s+FW_VERSION\s+"(\d+(?:\.\d+){3})"')


def parse(version):
    return tuple(int(n) for n in version.split("."))


def fmt(numbers):
    return ".".join(str(n) for n in numbers)


def bump(numbers, part):
    numbers = list(numbers)
    i = PARTS.index(part)
    numbers[i] += 1
    for j in range(i + 1, 4):
        numbers[j] = 0
    return tuple(numbers)


def latest_tag(tags):
    versions = [tuple(int(n) for n in m.groups()) for m in (TAG.match(t.strip()) for t in tags) if m]
    return max(versions) if versions else None


def next_version(tags, config_version, part):
    config = parse(config_version)
    latest = latest_tag(tags)
    if latest is None:
        return fmt(config)
    candidate = bump(latest, part)
    return fmt(max(candidate, config))


def config_version(path):
    with open(path) as f:
        match = CONFIG_VERSION.search(f.read())
    if not match:
        sys.exit("FW_VERSION not found in " + path)
    return match.group(1)


def main():
    parser = argparse.ArgumentParser(description="Version of the next release")
    parser.add_argument("part", choices=PARTS)
    parser.add_argument("--config", default=CONFIG)
    args = parser.parse_args()

    tags = subprocess.run(["git", "tag", "--list", "v*"], check=True, capture_output=True, text=True).stdout.split()
    version = next_version(tags, config_version(args.config), args.part)
    if "v" + version in tags:
        sys.exit("Tag v%s already exists" % version)
    sys.stderr.write("Latest tag: %s -> next version %s\n" % (fmt(latest_tag(tags)) if latest_tag(tags) else "none", version))
    print("version=" + version)


if __name__ == "__main__":
    main()
