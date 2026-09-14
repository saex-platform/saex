import copy
import json
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import asi_policy as policy


class AsiPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=[(ROOT/p).read_bytes() for p in policy.SOURCES]
        self.p=json.loads(self.data[0])
    def compile(self,p): return policy.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_output(self):
        self.assertEqual(policy.compile_source(*self.data),(ROOT/policy.OUTPUT).read_text(encoding='utf-8'))
    def test_keys_size_duplicate(self):
        with self.assertRaisesRegex(ValueError,'asi_policy_keys'): self.compile(dict(self.p,allowAll=True))
        with self.assertRaisesRegex(ValueError,'duplicate_json_key'): policy.compile_source(b'{"a":1,"a":2}',*self.data[1:])
        for i in range(8):
            data=self.data.copy(); data[i]=b' '*65537
            with self.assertRaisesRegex(ValueError,'asi_policy_size'): policy.compile_source(*data)
    def test_parent_chain(self):
        reasons=['asi_policy_binding_drift','binding_policy_codec_drift','codec_policy_startup_drift','startup_policy_proxy_drift',
                 'proxy_policy_entry_drift','entry_policy_base_drift','startup_policy_profile_drift']
        for i,reason in enumerate(reasons,1):
            data=self.data.copy(); data[i]+=b' '
            with self.assertRaisesRegex(ValueError,reason): policy.compile_source(*data)
    def test_identity(self):
        for key,value,reason in [('id','bad"id','id'),('schemaVersion',True,'stage'),('stage','run-all','stage'),
                                ('artifactName','arbitrary.asi','artifact'),('reviewDocument','other.md','review')]:
            with self.assertRaisesRegex(ValueError,'asi_policy_'+reason): self.compile(dict(self.p,**{key:value}))
    def test_boundaries(self):
        for key in ['callRva','endRva']:
            for value in [True,0,-1,2**32]:
                with self.assertRaisesRegex(ValueError,'asi_policy_rva'): self.compile(dict(self.p,**{key:value}))
        with self.assertRaisesRegex(ValueError,'asi_policy_rva'): self.compile(dict(self.p,endRva=self.p['callRva']+6))
        for key in ['returnHex','endHex']:
            for value in ['0'*32,'90',False,'a'*33]:
                with self.assertRaisesRegex(ValueError,'asi_policy_return_bytes'): self.compile(dict(self.p,**{key:value}))
    def test_closure_and_hashes(self):
        for key in ['commonModules','debugModules']:
            for value in [[],False,self.p[key]*2]:
                with self.assertRaisesRegex(ValueError,'asi_policy_closure'): self.compile(dict(self.p,**{key:value}))
            for field,value,reason in [('name','other.dll','module'),('origin','game-root','module'),('bytes',True,'bytes'),
                                      ('bytes',2**30,'bytes'),('sha256','0'*64,'hash'),('sha256','Z'*64,'hash')]:
                p=copy.deepcopy(self.p); p[key][0][field]=value
                with self.assertRaisesRegex(ValueError,'asi_policy_'+reason): self.compile(p)


if __name__=='__main__': unittest.main()
