import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import proxy_policy
import entry_policy
import loader_policy


class ProxyPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT / proxy_policy.SOURCE).read_bytes()
        self.entry = (ROOT / entry_policy.SOURCE).read_bytes()
        self.base = (ROOT / loader_policy.SOURCE).read_bytes()
        self.policy = json.loads(self.data)

    def compile(self, policy):
        return proxy_policy.compile_source(json.dumps(policy).encode(), self.entry, self.base)

    def test_exact_generated_output(self):
        self.assertEqual(proxy_policy.compile_source(self.data, self.entry, self.base),
                         (ROOT / proxy_policy.OUTPUT).read_text(encoding='utf-8'))
        changed = dict(self.policy, thunkRva=7521)
        self.assertNotEqual(self.compile(changed), self.compile(self.policy))

    def test_stage_schema(self):
        for key, value in [('schemaVersion', True), ('stage', 'initialize-all')]:
            p = dict(self.policy); p[key] = value
            with self.assertRaisesRegex(ValueError, 'proxy_policy_stage'): self.compile(p)

    def test_unknown_duplicate_and_size(self):
        with self.assertRaisesRegex(ValueError, 'proxy_policy_keys'): self.compile(dict(self.policy, allowAll=True))
        with self.assertRaisesRegex(ValueError, 'duplicate_json_key'):
            proxy_policy.compile_source(b'{"id":1,"id":2}', self.entry, self.base)
        with self.assertRaisesRegex(ValueError, 'proxy_policy_size'):
            proxy_policy.compile_source(b' ' * 65537, self.entry, self.base)

    def test_parent_chain_drift(self):
        with self.assertRaisesRegex(ValueError, 'proxy_policy_entry_drift'):
            proxy_policy.compile_source(self.data, self.entry + b' ', self.base)
        with self.assertRaisesRegex(ValueError, 'entry_policy_base_drift'):
            proxy_policy.compile_source(self.data, self.entry, self.base + b' ')

    def test_module_hash_origin_binding(self):
        for key, value in [('module', 'kernel32.dll'), ('module', '../vorbisfile.dll'), ('moduleSha256', '0' * 64)]:
            p = dict(self.policy); p[key] = value
            with self.assertRaisesRegex(ValueError, 'proxy_policy_module_binding'): self.compile(p)

    def test_rva_types_bounds_alignment(self):
        for key in ['thunkRva', 'callTargetRva', 'returnSlotRva', 'iatRva', 'iatTargetRva']:
            for value in [True, 0, -1, 268435456, '7520', None]:
                p = dict(self.policy); p[key] = value
                with self.assertRaisesRegex(ValueError, 'proxy_policy_rva'): self.compile(p)
        for key in ['returnSlotRva', 'iatRva']:
            p = copy.deepcopy(self.policy); p[key] += 1
            with self.assertRaisesRegex(ValueError, 'proxy_policy_alignment'): self.compile(p)

    def test_id_and_review(self):
        with self.assertRaisesRegex(ValueError, 'proxy_policy_id'): self.compile(dict(self.policy, id='bad"id'))
        with self.assertRaisesRegex(ValueError, 'proxy_policy_review'): self.compile(dict(self.policy, reviewDocument='elsewhere.md'))


if __name__ == '__main__': unittest.main()
