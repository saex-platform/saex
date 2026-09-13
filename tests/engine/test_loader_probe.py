"""Unknown input must fail before even the first loader continuation or child creation."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from pe_fixture import fixture

PROBE = str(Path(sys.argv.pop(1)).resolve())


class LoaderProbeTests(unittest.TestCase):
    def inspect(self, data, reason):
        with tempfile.TemporaryDirectory(prefix="saex loader ret ") as directory:
            path = Path(directory) / "gta_sa.exe"
            path.write_bytes(data)
            result = subprocess.run([PROBE, "--observe-loader", str(path)], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 1, result.stderr)
            output = json.loads(result.stdout)
            self.assertEqual(output["scope"], "bounded-loader-mapping-observation")
            self.assertEqual(output["reason"], reason)
            for key in ("canAttach", "childCreated", "childExitConfirmed", "loaderAdvanced", "initializationVerified", "breakpointCandidate"):
                self.assertIs(output[key], False, key)
            self.assertEqual(output["modules"], [])
            self.assertEqual(output["eventCount"], 0)
            self.assertEqual(output["lastEventCode"], 0)
            self.assertEqual(output["lastEventThreadId"], 0)
            self.assertEqual(output["lastEventAddress"], 0)
            self.assertEqual(output["activeModuleCount"], 0)
            self.assertEqual(output["unloadCount"], 0)
            self.assertEqual(path.read_bytes(), data)

    def test_unknown_fingerprint(self):
        self.inspect(fixture(), "unknown_fingerprint")

    def test_reviewed_mode_rejects_unknown_before_policy_or_child(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'gta_sa.exe'; path.write_bytes(fixture())
            result = subprocess.run([PROBE, '--observe-reviewed-loader', str(path)], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 1)
            output = json.loads(result.stdout)
            self.assertEqual(output['reason'], 'unknown_fingerprint')
            self.assertIs(output['childCreated'], False)
            self.assertIs(output['loaderAdvanced'], False)
            self.assertRegex(output['policySourceDigest'], r'^[0-9a-f]{64}$')
            self.assertEqual(output['failedPolicyModule'], '')

    def test_wrong_architecture(self):
        data = fixture(); struct.pack_into("<H", data, 68, 0x8664)
        self.inspect(data, "unsupported_engine_architecture")

    def test_truncated(self):
        self.inspect(fixture()[:-1], "section_bounds")

    def test_invalid_header(self):
        self.inspect(bytes(128), "dos_signature")

    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([PROBE, "--observe-loader", str(Path(directory) / "missing.exe")], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 1)
            output = json.loads(result.stdout)
            self.assertEqual(output["reason"], "file_open_failed")
            self.assertIs(output["childCreated"], False)

    def test_explicit_opt_in(self):
        for args in ([], ["gta_sa.exe"], ["--attach", "gta_sa.exe"], ["--observe-loader"], ["--observe-loader", "file", "--allow-all"],
                     ["--observe-context-loader", "file"], ["--observe-context-loader", "file", "cwd", "extra"],
                     ["--observe-entry-boundary", "file"], ["--observe-entry-boundary", "file", "cwd", "extra"]):
            result = subprocess.run([PROBE, *args], capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, b"")
            self.assertIn(b"Usage:", result.stderr)

    def test_entry_mode_keeps_unknown_engine_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'gta_sa.exe'; data = fixture(); path.write_bytes(data)
            result = subprocess.run([PROBE, '--observe-entry-boundary', str(path), directory], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 1)
            output = json.loads(result.stdout)
            self.assertEqual(output['scope'], 'bounded-entry-boundary-observation')
            self.assertEqual(output['reason'], 'unknown_fingerprint')
            for key in ['childCreated', 'loaderAdvanced', 'canAttach', 'initializationVerified']:
                self.assertFalse(output[key])
            for key in ['breakpointArmed', 'initialBreakpointContinued', 'boundaryReached', 'bytesRead', 'bytesMatch']:
                self.assertFalse(output['entryObservation'][key])
            self.assertEqual(path.read_bytes(), data)

    def test_entry_mode_requires_explicit_valid_context(self):
        result = subprocess.run([PROBE, '--observe-entry-boundary', 'unused.exe', 'relative'], capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 1)
        output = json.loads(result.stdout)
        self.assertEqual(output['reason'], 'launch_directory_input')
        self.assertFalse(output['childCreated'])
        self.assertFalse(output['entryObservation']['initialBreakpointContinued'])

    def test_context_rejects_invalid_directory_before_child(self):
        with tempfile.TemporaryDirectory() as directory:
            for path, reason in [('relative', 'launch_directory_input'),
                                 (str(Path(directory) / 'missing'), 'launch_directory_unavailable')]:
                result = subprocess.run([PROBE, '--observe-context-loader', 'unused.exe', path], capture_output=True, timeout=10, check=False)
                self.assertEqual(result.returncode, 1)
                output = json.loads(result.stdout)
                self.assertEqual(output['reason'], reason)
                self.assertFalse(output['childCreated'])
                self.assertFalse(output['loaderAdvanced'])
                self.assertFalse(output['launchContext']['prepared'])

    def test_context_redacts_environment_and_keeps_engine_gate(self):
        with tempfile.TemporaryDirectory(prefix='saex context çığ ') as directory:
            path = Path(directory) / 'gta_sa.exe'
            path.write_bytes(fixture())
            environment = dict(os.environ, SAEX_TEST_PRIVATE_VALUE='synthetic-secret-never-print')
            result = subprocess.run([PROBE, '--observe-context-loader', str(path), directory], env=environment,
                                    capture_output=True, timeout=10, check=False)
            self.assertEqual(result.returncode, 1)
            self.assertNotIn(b'synthetic-secret-never-print', result.stdout + result.stderr)
            self.assertNotIn(b'SAEX_TEST_PRIVATE_VALUE', result.stdout + result.stderr)
            output = json.loads(result.stdout)
            self.assertEqual(output['reason'], 'unknown_fingerprint')
            self.assertFalse(output['childCreated'])
            self.assertFalse(output['initializationVerified'])
            context = output['launchContext']
            self.assertTrue(context['prepared'])
            self.assertTrue(Path(context['directory']).samefile(directory))
            self.assertRegex(context['environmentSha256'], r'^[0-9a-f]{64}$')
            self.assertGreater(context['environmentEntries'], 0)
            self.assertLessEqual(context['environmentCodeUnits'], 65536)


if __name__ == "__main__": unittest.main()
