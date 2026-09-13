"""Real CLI rejection tests use synthetic PE bytes, never launch a game."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from pe_fixture import fixture

PROBE = str(Path(sys.argv.pop(1)).resolve())


class EngineProbeTests(unittest.TestCase):
    def inspect(self, data, reason):
        with tempfile.TemporaryDirectory(prefix="saex-probe-test-") as directory:
            path = Path(directory) / "gta_sa.exe"; path.write_bytes(data)
            result = subprocess.run([PROBE, str(path)], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertLess(len(result.stdout), 1024)
            self.assertEqual(json.loads(result.stdout), {"scope": "non-executing-image-observation", "canAttach": False, "reason": reason})
            self.assertEqual(path.read_bytes(), data, "inspector changed file")

    def test_version_marker_does_not_authorize_mapping(self):
        self.inspect(fixture(), "unknown_fingerprint")

    def test_wrong_architecture(self):
        b = fixture(); b[68:70] = (0x8664).to_bytes(2, "little")
        self.inspect(b, "unsupported_engine_architecture")

    def test_truncated_section(self):
        self.inspect(fixture()[:-1], "section_bounds")

    def test_broken_dos_header(self):
        self.inspect(bytes(128), "dos_signature")


if __name__ == "__main__": unittest.main()
