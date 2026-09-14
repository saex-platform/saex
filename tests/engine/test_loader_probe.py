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
            self.assertIsNone(output['asiObservation'])
            self.assertEqual(output["unloadCount"], 0)
            self.assertEqual(path.read_bytes(), data)

    def test_cwd_acquire_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-cwd-acquire','--observe-application-routing'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-cwd-acquire':self.assertIsNone(output['cwdAcquireObservation']);continue
                self.assertEqual(output['scope'],'bounded-cwd-acquire')
                self.assertFalse(output['cwdAcquireObservation']['verified'])
                self.assertFalse(output['cwdAcquireObservation']['directoryApiAllowed'])
                self.assertTrue(output['gamePreludeObservation']['fileManagerCallAllowed'])

    def test_cwd_acquire_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-cwd-acquire',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-cwd-acquire','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_cwd_lock_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-cwd-lock','--observe-application-routing'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-cwd-lock':self.assertIsNone(output['cwdLockObservation']);continue
                self.assertEqual(output['scope'],'bounded-cwd-lock')
                self.assertFalse(output['cwdLockObservation']['verified'])
                self.assertFalse(output['cwdLockObservation']['branchAllowed'])
                self.assertTrue(output['gamePreludeObservation']['fileManagerCallAllowed'])

    def test_cwd_lock_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-cwd-lock',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-cwd-lock','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_cwd_seh_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-cwd-seh','--observe-application-routing'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-cwd-seh':self.assertIsNone(output['cwdSehObservation']);continue
                self.assertEqual(output['scope'],'bounded-cwd-seh')
                self.assertFalse(output['cwdSehObservation']['verified'])
                self.assertFalse(output['cwdSehObservation']['lockPathAllowed'])
                self.assertTrue(output['gamePreludeObservation']['fileManagerCallAllowed'])

    def test_cwd_seh_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-cwd-seh',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-cwd-seh','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_file_manager_entry_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-file-manager-entry','--observe-application-routing'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-file-manager-entry':self.assertIsNone(output['fileManagerEntryObservation']);continue
                self.assertEqual(output['scope'],'bounded-file-manager-entry')
                self.assertFalse(output['fileManagerEntryObservation']['verified'])
                self.assertFalse(output['fileManagerEntryObservation']['cwdCallAllowed'])
                self.assertTrue(output['gamePreludeObservation']['fileManagerCallAllowed'])

    def test_file_manager_entry_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-file-manager-entry',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-file-manager-entry','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

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
                     ["--observe-entry-boundary", "file"], ["--observe-entry-boundary", "file", "cwd", "extra"],
                     ["--observe-proxy-return", "file"], ["--observe-proxy-return", "file", "cwd", "extra"],
                     ["--observe-startup-call", "file"], ["--observe-startup-call", "file", "cwd", "extra"],
                     ["--observe-codec-return", "file"], ["--observe-codec-return", "file", "cwd", "extra"],
                     ["--observe-codec-bindings", "file"], ["--observe-codec-bindings", "file", "cwd", "extra"]):
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

    def test_proxy_unknown_engine_does_not_create_child(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'unknown.exe'; path.write_bytes(fixture())
            result = subprocess.run([PROBE, '--observe-proxy-return', str(path), directory], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 1)
            output = json.loads(result.stdout)
            self.assertEqual(output['scope'], 'bounded-proxy-return-observation')
            self.assertEqual(output['reason'], 'unknown_fingerprint')
            for key in ['childCreated', 'canAttach', 'initializationVerified', 'loaderAdvanced']:
                self.assertFalse(output[key])
            for key in ['validated', 'breakpointArmed', 'continued', 'returnReached', 'entryRestored', 'iatVerified']:
                self.assertFalse(output['proxyObservation'][key])
            self.assertRegex(output['proxyObservation']['executionPolicySourceDigest'], r'^[0-9a-f]{64}$')

    def test_proxy_requires_explicit_context(self):
        result = subprocess.run([PROBE, '--observe-proxy-return', 'unused.exe', 'relative'], capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 1)
        output = json.loads(result.stdout)
        self.assertEqual(output['reason'], 'launch_directory_input')
        self.assertFalse(output['childCreated'])
        self.assertFalse(output['proxyObservation']['continued'])

    def test_startup_unknown_engine_stays_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'unknown.exe'; path.write_bytes(fixture())
            result = subprocess.run([PROBE, '--observe-startup-call', str(path), directory], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 1)
            output = json.loads(result.stdout)
            self.assertEqual(output['scope'], 'bounded-startup-call-observation')
            self.assertEqual(output['reason'], 'unknown_fingerprint')
            self.assertFalse(output['childCreated'])
            self.assertFalse(output['canAttach'])
            self.assertFalse(output['initializationVerified'])
            startup = output['startupObservation']
            for key in ['breakpointArmed','continued','reached','iatWriteObserved','targetStable','callsiteVerified','argumentValid']:
                self.assertFalse(startup[key])
            self.assertEqual(startup['samples'], [])
            self.assertRegex(startup['executionPolicySourceDigest'], r'^[0-9a-f]{64}$')

    def test_startup_invalid_context_before_child(self):
        result = subprocess.run([PROBE, '--observe-startup-call', 'unused.exe', 'relative'], capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 1)
        output = json.loads(result.stdout)
        self.assertEqual(output['reason'], 'launch_directory_input')
        self.assertFalse(output['childCreated'])
        self.assertFalse(output['startupObservation']['continued'])

    def test_codec_unknown_engine_stays_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'unknown.exe'; path.write_bytes(fixture())
            result = subprocess.run([PROBE, '--observe-codec-return', str(path), directory], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 1)
            output = json.loads(result.stdout)
            self.assertEqual(output['scope'], 'bounded-codec-return-observation')
            self.assertEqual(output['reason'], 'unknown_fingerprint')
            for key in ['childCreated', 'initializationVerified', 'canAttach']: self.assertFalse(output[key])
            codec = output['codecObservation']
            for key in ['breakpointArmed', 'continued', 'reached', 'shapeVerified', 'modulesVerified']: self.assertFalse(codec[key])
            self.assertEqual(codec['mappingIds'], [0, 0, 0])
            self.assertRegex(codec['executionPolicySourceDigest'], r'^[0-9a-f]{64}$')

    def test_codec_requires_explicit_context(self):
        result = subprocess.run([PROBE, '--observe-codec-return', 'unused.exe', 'relative'], capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 1)
        output = json.loads(result.stdout)
        self.assertEqual(output['reason'], 'launch_directory_input')
        self.assertFalse(output['childCreated'])
        self.assertFalse(output['codecObservation']['continued'])

    def test_bindings_unknown_engine_stays_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'unknown.exe'; path.write_bytes(fixture())
            result=subprocess.run([PROBE,'--observe-codec-bindings',str(path),directory],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,1)
            output=json.loads(result.stdout)
            self.assertEqual(output['scope'],'bounded-codec-bindings-observation')
            self.assertEqual(output['reason'],'unknown_fingerprint')
            self.assertFalse(output['childCreated'])
            self.assertFalse(output['canAttach'])
            self.assertFalse(output['initializationVerified'])
            for key in ['breakpointArmed','continued','reached','verified']: self.assertFalse(output['bindingObservation'][key])
            self.assertEqual(output['bindingObservation']['slots'],[])
            self.assertRegex(output['bindingObservation']['executionPolicySourceDigest'],r'^[0-9a-f]{64}$')

    def test_bindings_requires_explicit_context(self):
        result=subprocess.run([PROBE,'--observe-codec-bindings','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1)
        output=json.loads(result.stdout)
        self.assertEqual(output['reason'],'launch_directory_input')
        self.assertFalse(output['childCreated'])
        self.assertFalse(output['bindingObservation']['continued'])

    def test_asi_unknown_engine_stays_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'unknown.exe'; path.write_bytes(fixture())
            result=subprocess.run([PROBE,'--observe-asi-return',str(path),directory],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,1)
            output=json.loads(result.stdout)
            self.assertEqual(output['scope'],'bounded-asi-return-observation')
            self.assertEqual(output['reason'],'unknown_fingerprint')
            for key in ['childCreated','initializationVerified','canAttach']: self.assertFalse(output[key])
            asi=output['asiObservation']
            for key in ['callArmed','scanContinued','callReached','pathVerified','returnArmed','loadContinued','returnReached','verified','bootstrapExportsCalled']:
                self.assertFalse(asi[key])
            for key in ['executionPolicySourceDigest','artifactSha256','artifactMapSha256']:
                self.assertRegex(asi[key],r'^[0-9a-f]{64}$')

    def test_asi_requires_explicit_context(self):
        result=subprocess.run([PROBE,'--observe-asi-return','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1)
        output=json.loads(result.stdout)
        self.assertEqual(output['reason'],'launch_directory_input')
        self.assertFalse(output['childCreated'])
        self.assertFalse(output['asiObservation']['loadContinued'])
        for arguments in [[],['unused.exe'],['unused.exe','relative','extra']]:
            result=subprocess.run([PROBE,'--observe-asi-return',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)

    def test_bootstrap_unknown_engine_never_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'unknown.exe'; path.write_bytes(fixture())
            result=subprocess.run([PROBE,'--observe-bootstrap-lifecycle',str(path),directory],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,1)
            output=json.loads(result.stdout)
            self.assertEqual(output['scope'],'bounded-bootstrap-lifecycle-observation')
            self.assertEqual(output['reason'],'unknown_fingerprint')
            self.assertFalse(output['childCreated'])
            self.assertFalse(output['canAttach'])
            self.assertFalse(output['initializationVerified'])
            state=output['bootstrapObservation']
            self.assertTrue(state['bootstrapExecutionAllowed'])
            self.assertFalse(state['stackWriteAttempted'])
            self.assertFalse(state['stackWritten'])
            self.assertFalse(state['verified'])
            self.assertEqual(state['calls'],[])
            self.assertEqual(state['callsArmed'],0)
            self.assertRegex(state['executionPolicySourceDigest'],r'^[0-9a-f]{64}$')

    def test_bootstrap_context_and_legacy(self):
        result=subprocess.run([PROBE,'--observe-bootstrap-lifecycle','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1)
        output=json.loads(result.stdout)
        self.assertEqual(output['reason'],'launch_directory_input')
        self.assertFalse(output['bootstrapObservation']['stackWritten'])
        for arguments in [[],['unused.exe'],['unused.exe','relative','extra']]:
            result=subprocess.run([PROBE,'--observe-bootstrap-lifecycle',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'unknown.exe'; path.write_bytes(fixture())
            result=subprocess.run([PROBE,'--observe-asi-return',str(path),directory],capture_output=True,timeout=10)
            output=json.loads(result.stdout)
            self.assertIsNone(output['bootstrapObservation'])
            self.assertFalse(output['asiObservation']['bootstrapExportsCalled'])

    def test_frame_unknown_host(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'unknown.exe'; path.write_bytes(fixture())
            result=subprocess.run([PROBE,'--observe-frame-target',str(path),directory],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,1)
            output=json.loads(result.stdout)
            self.assertEqual(output['scope'],'bounded-frame-target-observation')
            self.assertEqual(output['reason'],'unknown_fingerprint')
            self.assertFalse(output['childCreated'])
            state=output['frameTargetObservation']
            for key in ['nativeFunctionCalled','hookInstalled','runtimeAbiVerified','verified']: self.assertFalse(state[key])
            self.assertEqual([s['phase'] for s in state['samples']],['create-process','asi-return','bootstrap-terminal'])
            self.assertTrue(all(not s['attempted'] for s in state['samples']))

    def test_frame_arguments_and_legacy(self):
        for arguments in [[],['unused.exe'],['unused.exe','relative','extra']]:
            result=subprocess.run([PROBE,'--observe-frame-target',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        for mode in ['--observe-frame-target','--observe-bootstrap-lifecycle']:
            result=subprocess.run([PROBE,mode,'unused.exe','relative'],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,1)
            output=json.loads(result.stdout)
            self.assertEqual(output['reason'],'launch_directory_input')
            self.assertFalse(output['childCreated'])
            if mode=='--observe-bootstrap-lifecycle': self.assertIsNone(output['frameTargetObservation'])

    def test_startup_return_unknown_and_legacy(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'unknown.exe'; path.write_bytes(fixture())
            for mode in ['--observe-startup-return','--observe-frame-target','--observe-asi-return']:
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1); output=json.loads(result.stdout)
                self.assertFalse(output['childCreated'])
                self.assertEqual(output['reason'],'unknown_fingerprint')
                if mode!='--observe-startup-return': self.assertIsNone(output['startupReturnObservation']); continue
                state=output['startupReturnObservation'];self.assertTrue(state['loaderProtectionChangeAllowed'])
                for key in ['bootstrapInvoked','protectContinued','verified','startupReturnReached']: self.assertFalse(state[key])
                self.assertIsNone(output['bootstrapObservation'])

    def test_platform_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-platform-startup','--observe-application-entry'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertFalse(output['childCreated']);self.assertEqual(output['reason'],'unknown_fingerprint')
                if mode!='--observe-platform-startup':self.assertIsNone(output['platformStartupObservation']);continue
                state=output['platformStartupObservation'];self.assertTrue(state['prologueExecutionAllowed'])
                for k in ('hostSettingCallAllowed','hostSettingCallExecuted','continued','callReached','verified'):self.assertFalse(state[k])
                self.assertFalse(output['applicationEntryObservation']['applicationBodyExecuted'])

    def test_platform_arguments(self):
        for arguments in ([],['unused.exe'],['unused.exe','relative','extra']):
            result=subprocess.run([PROBE,'--observe-platform-startup',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        result=subprocess.run([PROBE,'--observe-platform-startup','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1);self.assertEqual(json.loads(result.stdout)['reason'],'launch_directory_input')

    def test_game_prelude_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-game-prelude','--observe-application-routing'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-game-prelude':self.assertIsNone(output['gamePreludeObservation']);continue
                self.assertEqual(output['scope'],'bounded-game-prelude')
                self.assertFalse(output['gamePreludeObservation']['verified'])
                self.assertFalse(output['gamePreludeObservation']['fileManagerCallAllowed'])

    def test_game_prelude_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-game-prelude',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-game-prelude','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_application_routing_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-application-routing','--observe-event-dispatch'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-application-routing':self.assertIsNone(output['applicationRoutingObservation']);continue
                self.assertEqual(output['scope'],'bounded-application-routing')
                self.assertFalse(output['applicationRoutingObservation']['verified'])
                self.assertFalse(output['applicationRoutingObservation']['initializerCallAllowed'])

    def test_application_routing_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-application-routing',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-application-routing','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_event_dispatch_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-event-dispatch','--observe-instance-startup'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-event-dispatch':self.assertIsNone(output['eventDispatchObservation']);continue
                self.assertEqual(output['scope'],'bounded-event-dispatch')
                self.assertFalse(output['eventDispatchObservation']['verified'])
                self.assertFalse(output['eventDispatchObservation']['applicationHandlerCallAllowed'])

    def test_event_dispatch_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-event-dispatch',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-event-dispatch','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_instance_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-instance-startup','--observe-platform-suppression'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
                if mode!='--observe-instance-startup':self.assertIsNone(output['instanceStartupObservation']);continue
                self.assertEqual(output['scope'],'bounded-instance-startup')
                self.assertFalse(output['instanceStartupObservation']['verified'])
                self.assertFalse(output['instanceStartupObservation']['windowActivationAllowed'])

    def test_instance_arguments(self):
        for args in ([],['unused.exe'],['unused.exe','relative','extra']):
            r=subprocess.run([PROBE,'--observe-instance-startup',*args],capture_output=True,timeout=10)
            self.assertEqual(r.returncode,2)
        r=subprocess.run([PROBE,'--observe-instance-startup','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(r.returncode,1);self.assertEqual(json.loads(r.stdout)['reason'],'launch_directory_input')

    def test_suppression_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-platform-suppression','--observe-application-entry'):
                result=subprocess.run([PROBE,mode,str(path),directory],capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertFalse(output['childCreated']);self.assertEqual(output['reason'],'unknown_fingerprint')
                if mode!='--observe-platform-suppression':self.assertIsNone(output['platformSuppressionObservation']);continue
                state=output['platformSuppressionObservation'];self.assertTrue(state['contextWriteAllowed'])
                for k in ('hostSettingCallExecuted','naturalApiReturn','writeAttempted','continued','returnReached','verified'):self.assertFalse(state[k])
                self.assertFalse(output['applicationEntryObservation']['applicationBodyExecuted'])

    def test_suppression_arguments(self):
        for arguments in ([],['unused.exe'],['unused.exe','relative','extra']):
            result=subprocess.run([PROBE,'--observe-platform-suppression',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        result=subprocess.run([PROBE,'--observe-platform-suppression','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1);self.assertEqual(json.loads(result.stdout)['reason'],'launch_directory_input')

    def test_application_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            for mode in ('--observe-application-entry','--observe-crt-startup','--observe-loader'):
                args=[PROBE,mode,str(path)]+([] if mode=='--observe-loader' else [directory])
                result=subprocess.run(args,capture_output=True,timeout=10)
                self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
                self.assertFalse(output['childCreated'])
                if mode!='--observe-application-entry':self.assertIsNone(output['applicationEntryObservation']);continue
                self.assertEqual(output['reason'],'unknown_fingerprint')
                state=output['applicationEntryObservation'];self.assertTrue(state['initializerExecutionAllowed'])
                for key in ('continued','initializerReturned','secondReturned','entryReached','verified','applicationBodyExecuted'):self.assertFalse(state[key])
                self.assertFalse(output['canAttach']);self.assertFalse(output['initializationVerified'])

    def test_application_arguments(self):
        for arguments in ([],['unused.exe'],['unused.exe','relative','extra']):
            result=subprocess.run([PROBE,'--observe-application-entry',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        result=subprocess.run([PROBE,'--observe-application-entry','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1);self.assertEqual(json.loads(result.stdout)['reason'],'launch_directory_input')

    def test_crt_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'gta_sa.exe';path.write_bytes(fixture())
            result=subprocess.run([PROBE,'--observe-crt-startup',str(path),directory],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,1);output=json.loads(result.stdout)
            self.assertEqual(output['reason'],'unknown_fingerprint');self.assertFalse(output['childCreated'])
            self.assertFalse(output['crtStartupObservation']['continued'])
            self.assertFalse(output['crtStartupObservation']['initializerExecutionAllowed'])
            self.assertFalse(output['crtStartupObservation']['verified'])

    def test_crt_arguments(self):
        for arguments in ([],['unused.exe'],['unused.exe','relative','extra']):
            result=subprocess.run([PROBE,'--observe-crt-startup',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        result=subprocess.run([PROBE,'--observe-crt-startup','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1);self.assertEqual(json.loads(result.stdout)['reason'],'launch_directory_input')

    def test_startup_return_arguments(self):
        for arguments in [[],['unused.exe'],['unused.exe','relative','extra']]:
            result=subprocess.run([PROBE,'--observe-startup-return',*arguments],capture_output=True,timeout=10)
            self.assertEqual(result.returncode,2)
        result=subprocess.run([PROBE,'--observe-startup-return','unused.exe','relative'],capture_output=True,timeout=10)
        self.assertEqual(result.returncode,1); self.assertEqual(json.loads(result.stdout)['reason'],'launch_directory_input')

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
