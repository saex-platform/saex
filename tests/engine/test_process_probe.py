"""The actual process CLI must reject untrusted bytes before creating any child."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from pe_fixture import fixture

PROBE = str(Path(sys.argv.pop(1)).resolve())


class ProcessProbeTests(unittest.TestCase):
    def inspect(self, data, reason):
        with tempfile.TemporaryDirectory(prefix="saex process ret ") as directory:
            path = Path(directory) / "gta_sa.exe"
            path.write_bytes(data)
            result = subprocess.run([PROBE, "--observe-suspended", str(path)], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertEqual(json.loads(result.stdout), {
                "scope": "created-suspended-process-observation", "canAttach": False,
                "childCreated": False, "childExitConfirmed": False, "systemError": 0, "reason": reason})
            self.assertEqual(path.read_bytes(), data)

    def test_same_marker_unknown_hash_never_creates_child(self):
        self.inspect(fixture(), "unknown_fingerprint")

    def test_wrong_architecture(self):
        b = fixture(); struct.pack_into("<H", b, 68, 0x8664)
        self.inspect(b, "unsupported_engine_architecture")

    def test_truncated_image(self):
        self.inspect(fixture()[:-1], "section_bounds")

    def test_invalid_header(self):
        self.inspect(bytes(128), "dos_signature")

    def test_explicit_opt_in_required(self):
        for args in ([], ["gta_sa.exe"], ["--attach", "gta_sa.exe"], ["--observe-suspended"]):
            result = subprocess.run([PROBE, *args], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, b"")
            self.assertIn(b"Usage:", result.stderr)


if __name__ == "__main__": unittest.main()
