import hashlib
import json
from pathlib import Path
import sys
import unittest
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import startup_policy as policy
import proxy_policy
import entry_policy
import loader_policy


class StartupPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data = [(ROOT / p).read_bytes() for p in (policy.SOURCE, proxy_policy.SOURCE, entry_policy.SOURCE, loader_policy.SOURCE, policy.PROFILE)]
        self.p = json.loads(self.data[0])

    def compile(self, p): return policy.compile_source(json.dumps(p).encode(), *self.data[1:])

    def test_output(self):
        self.assertEqual(policy.compile_source(*self.data), (ROOT / policy.OUTPUT).read_text(encoding='utf-8'))
        self.assertNotEqual(self.compile(dict(self.p, id='fixture-new-id')), self.compile(self.p))

    def test_schema_stage(self):
        for key, value in [('schemaVersion', True), ('stage', 'run-main')]:
            p = dict(self.p); p[key] = value
            with self.assertRaisesRegex(ValueError, 'startup_policy_stage'): self.compile(p)

    def test_keys_duplicates_size(self):
        with self.assertRaisesRegex(ValueError, 'startup_policy_keys'): self.compile(dict(self.p, allowAll=True))
        with self.assertRaisesRegex(ValueError, 'duplicate_json_key'):
            policy.compile_source(b'{"id":1,"id":2}', *self.data[1:])
        for i in range(5):
            data = self.data.copy(); data[i] = b' ' * 65537
            with self.assertRaisesRegex(ValueError, 'startup_policy_size'): policy.compile_source(*data)

    def test_parent_chain(self):
        for i, reason in [(1,'startup_policy_proxy_drift'), (2,'proxy_policy_entry_drift'), (3,'entry_policy_base_drift'), (4,'startup_policy_profile_drift')]:
            data = self.data.copy(); data[i] += b' '
            with self.assertRaisesRegex(ValueError, reason): policy.compile_source(*data)

    def test_boundary_cannot_be_weakened(self):
        for key, values in [('callForm',['call-register','any']), ('stackArgumentBytes',[True,0,64,69]),
                            ('maxImageSamples',[True,0,5]), ('iatWriteWatch',[False,1,'true'])]:
            for value in values:
                p = dict(self.p); p[key] = value
                with self.assertRaisesRegex(ValueError,'startup_policy_boundary'): self.compile(p)

    def test_profile_engine_binding(self):
        profile = json.loads(self.data[4]); profile['sha256'] = '0' * 64
        self.data[4] = json.dumps(profile).encode()
        self.p['observedProfileSha256'] = hashlib.sha256(self.data[4]).hexdigest()
        with self.assertRaisesRegex(ValueError,'startup_policy_engine_drift'): self.compile(self.p)

    def test_id_review(self):
        with self.assertRaisesRegex(ValueError,'startup_policy_id'): self.compile(dict(self.p,id='bad"id'))
        with self.assertRaisesRegex(ValueError,'startup_policy_review'): self.compile(dict(self.p,reviewDocument='unreviewed.md'))


if __name__ == '__main__': unittest.main()
