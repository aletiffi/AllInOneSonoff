#!/usr/bin/env python3
"""Unit tests of the release scripts: python3 -m unittest discover -s scripts -p 'test_*.py'"""

import json
import os
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import next_version  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))


class NextVersionTest(unittest.TestCase):
    def test_first_release_uses_config(self):
        self.assertEqual(next_version.next_version([], "45.4.0.0", "build"), "45.4.0.0")
        self.assertEqual(next_version.next_version(["old-tag", "v1.2"], "45.4.0.0", "minor"), "45.4.0.0")

    def test_bump_latest_tag(self):
        tags = ["v45.4.0.0", "v45.4.0.2", "v45.4.0.10", "v45.3.9.9"]
        self.assertEqual(next_version.next_version(tags, "45.4.0.0", "build"), "45.4.0.11")
        self.assertEqual(next_version.next_version(tags, "45.4.0.0", "patch"), "45.4.1.0")
        self.assertEqual(next_version.next_version(tags, "45.4.0.0", "minor"), "45.5.0.0")
        self.assertEqual(next_version.next_version(tags, "45.4.0.0", "major"), "46.0.0.0")

    def test_numeric_not_alphabetical(self):
        self.assertEqual(next_version.latest_tag(["v45.9.0.0", "v45.10.0.0"]), (45, 10, 0, 0))

    def test_config_higher_wins(self):
        self.assertEqual(next_version.next_version(["v45.4.0.3"], "46.0.0.0", "build"), "46.0.0.0")
        self.assertEqual(next_version.next_version(["v45.4.0.3"], "45.4.0.0", "build"), "45.4.0.4")

    def test_config_parsing(self):
        with tempfile.NamedTemporaryFile("w", suffix=".h", delete=False) as f:
            f.write('#ifndef FW_VERSION\n#define FW_VERSION                "45.4.0.0"\n#endif\n')
        try:
            self.assertEqual(next_version.config_version(f.name), "45.4.0.0")
        finally:
            os.unlink(f.name)

    def test_real_config(self):
        version = next_version.config_version(next_version.CONFIG)
        self.assertEqual(len(next_version.parse(version)), 4)


class MakeManifestTest(unittest.TestCase):
    def test_manifest(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, "manifest.json")
            subprocess.run([sys.executable, os.path.join(HERE, "make_manifest.py"), "45.4.0.1",
                            "https://github.com/o/r/releases/download/v45.4.0.1/f.ota.bin.gz", out], check=True,
                           capture_output=True)
            with open(out) as f:
                self.assertEqual(json.load(f), {"version": "45.4.0.1",
                                                "url": "https://github.com/o/r/releases/download/v45.4.0.1/f.ota.bin.gz"})

    def test_invalid_url(self):
        r = subprocess.run([sys.executable, os.path.join(HERE, "make_manifest.py"), "1", "ftp://x", "/dev/null"],
                           capture_output=True)
        self.assertNotEqual(r.returncode, 0)


if __name__ == "__main__":
    unittest.main()
