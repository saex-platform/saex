import copy
import json
from pathlib import Path
import sys
import unittest
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import codec_policy as policy


class CodecPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data = [(ROOT / p).read_bytes() for p in policy.SOURCES]
        self.p = json.loads(self.data[0])

    def compile(self, p): return policy.compile_source(json.dumps(p).encode(), *self.data[1:])

    def test_output(self):
        self.assertEqual(policy.compile_source(*self.data), (ROOT / policy.OUTPUT).read_text(encoding='utf-8'))
        self.assertIn('reviewed_entry_specs.size() + 3', self.compile(self.p))

    def test_keys_size_duplicates(self):
        with self.assertRaisesRegex(ValueError, 'codec_policy_keys'): self.compile(dict(self.p, allowAll=True))
        with self.assertRaisesRegex(ValueError, 'duplicate_json_key'): policy.compile_source(b'{"a":1,"a":2}', *self.data[1:])
        for i in range(6):
            data = self.data.copy(); data[i] = b' ' * 65537
            with self.assertRaisesRegex(ValueError, 'codec_policy_size'): policy.compile_source(*data)

    def test_parent_chain(self):
        for i, reason in [(1, 'codec_policy_startup_drift'), (2, 'startup_policy_proxy_drift'),
                          (3, 'proxy_policy_entry_drift'), (4, 'entry_policy_base_drift'), (5, 'startup_policy_profile_drift')]:
            data = self.data.copy(); data[i] += b' '
            with self.assertRaisesRegex(ValueError, reason): policy.compile_source(*data)

    def test_recipe(self):
        for key, value, reason in [('schemaVersion', True, 'stage'), ('stage', 'asi-load', 'stage'),
                                   ('id', 'bad"id', 'id'), ('reviewDocument', 'other.md', 'review'),
                                   ('callForm', 'any', 'call_form'), ('loadSlotRva', 3, 'alignment')]:
            with self.assertRaisesRegex(ValueError, 'codec_policy_' + reason): self.compile(dict(self.p, **{key: value}))
        for key in ['sequenceRva', 'nameRva', 'loadSlotRva']:
            for value in [True, 0, -1, 2**32]:
                with self.assertRaisesRegex(ValueError, 'codec_policy_rva'): self.compile(dict(self.p, **{key: value}))

    def test_closure_cannot_expand(self):
        for value in [[], self.p['modules'][:2], self.p['modules'] * 2]:
            with self.assertRaisesRegex(ValueError, 'codec_policy_closure'): self.compile(dict(self.p, modules=value))
        with self.assertRaisesRegex(ValueError, 'codec_policy_closure'): self.compile(dict(self.p, requestedName='../other'))
        for key, value in [('name', 'other.dll'), ('origin', 'system-x86')]:
            p = copy.deepcopy(self.p); p['modules'][0][key] = value
            with self.assertRaisesRegex(ValueError, 'codec_policy_module'): self.compile(p)
        p = copy.deepcopy(self.p); p['modules'].reverse()
        with self.assertRaisesRegex(ValueError, 'codec_policy_module'): self.compile(p)

    def test_content_identity(self):
        for key, values, reason in [('bytes', [True, 0, 67108865], 'bytes'),
                                   ('sha256', ['0' * 64, 'a' * 63, 'A' * 64, None], 'hash')]:
            for value in values:
                p = copy.deepcopy(self.p); p['modules'][0][key] = value
                with self.assertRaisesRegex(ValueError, 'codec_policy_' + reason): self.compile(p)


if __name__ == '__main__': unittest.main()
