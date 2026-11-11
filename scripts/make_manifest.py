#!/usr/bin/env python3
"""Creates the OTA manifest read by the devices.

    make_manifest.py <version> <ota image url> <output file>

Output: {"version": "<version>", "url": "<ota image url>"}
"""

import json
import sys


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    version, url, out = sys.argv[1:]
    if not url.startswith(("http://", "https://")):
        sys.exit("The url must start with http:// or https://")
    with open(out, "w") as f:
        json.dump({"version": version, "url": url}, f, indent=2)
        f.write("\n")
    print("Manifest %s: version %s, url %s" % (out, version, url))


if __name__ == "__main__":
    main()
