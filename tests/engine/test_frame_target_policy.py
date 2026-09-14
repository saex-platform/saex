from pathlib import Path
import copy
import json
import sys
import unittest
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
import frame_target_policy as tool


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
        for key,value in [('schemaVersion',True),('extra',1),('stage','hook'),('id','"')]: self.rejects(key,value)
    def test_chain(self):
        for key in ['bootstrapPolicySha256','profileSourceSha256']: self.rejects(key,'0'*64)
        for index in [1,2,3]:
            changed=list(self.data); changed[index]+=b'\n'
            with self.assertRaises(ValueError): tool.compile_source(*changed)
    def test_hex(self):
        for key in ['callHex','targetPrefixHex']:
            for value in [True,'','gg',self.p[key]+'00',self.p[key].upper()]: self.rejects(key,value)
    def test_target(self):
        for key in ['callRva','targetRva']:
            for value in [True,-1,0,2**32,self.p[key]+1]: self.rejects(key,value)
        self.rejects('callHex','e95ad5ffff'); self.rejects('callHex','e800000080')
    def test_sources(self):
        self.rejects('sourceEvidence',[])
        for key,value in [('path','../CGame.cpp'),('sha256','0'*64),('extra',1)]:
            sources=copy.deepcopy(self.p['sourceEvidence']); sources[0][key]=value; self.rejects('sourceEvidence',sources)
    def test_review_duplicate_limit(self):
        self.rejects('reviewDocument','elsewhere.md')
        for data in [self.data[0].replace(b'"schemaVersion": 1', b'"schemaVersion": 1, "schemaVersion": 1'), b' '*65537]:
            with self.assertRaises(ValueError): tool.compile_source(data,*self.data[1:])


if __name__ == '__main__': unittest.main()
