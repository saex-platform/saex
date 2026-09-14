from pathlib import Path
import copy
import json
import sys
import unittest
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
import bootstrap_lifecycle_policy as tool


class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.data = [(ROOT/p).read_bytes() for p in tool.SOURCES]
        self.p = json.loads(self.data[0])
    def rejects(self, key, value):
        p = copy.deepcopy(self.p); p[key] = value
        with self.assertRaises(ValueError): tool.compile_source(json.dumps(p).encode(), *self.data[1:])
    def test_generated(self):
        self.assertEqual(tool.compile_source(*self.data), (ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_shape(self):
        self.rejects('schemaVersion', True); self.rejects('extra', 1); self.rejects('stage', 'auto'); self.rejects('id', '"')
    def test_chain(self):
        self.rejects('asiPolicySha256', '0'*64)
        changed = list(self.data); changed[1] += b'\n'
        with self.assertRaises(ValueError): tool.compile_source(*changed)
    def test_sequence(self):
        self.rejects('callSequence', ['Initialize']); self.rejects('callSequence', [1,0,1,0,2,1,0,2])
    def test_module(self):
        for key, value in [('name','arbitrary.dll'), ('bytes',True), ('bytes',0), ('sha256','0'*64), ('extra',1)]:
            m = dict(self.p['cryptoModule']); m[key] = value; self.rejects('cryptoModule',m)
    def test_review_duplicate_and_limit(self):
        self.rejects('reviewDocument','elsewhere.md')
        for data in [self.data[0].replace(b'"schemaVersion": 1', b'"schemaVersion": 1, "schemaVersion": 1'), b' '*65537]:
            with self.assertRaises(ValueError): tool.compile_source(data,*self.data[1:])


if __name__ == '__main__': unittest.main()
