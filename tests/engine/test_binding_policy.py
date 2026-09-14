import copy
import json
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import binding_policy as policy


class BindingPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=[(ROOT/p).read_bytes() for p in policy.SOURCES]
        self.p=json.loads(self.data[0])
    def compile(self,p): return policy.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_output(self):
        self.assertEqual(policy.compile_source(*self.data),(ROOT/policy.OUTPUT).read_text(encoding='utf-8'))
    def test_keys_size_duplicate(self):
        with self.assertRaisesRegex(ValueError,'binding_policy_keys'): self.compile(dict(self.p,allowAll=True))
        with self.assertRaisesRegex(ValueError,'duplicate_json_key'): policy.compile_source(b'{"a":1,"a":2}',*self.data[1:])
        for i in range(7):
            data=self.data.copy(); data[i]=b' '*65537
            with self.assertRaisesRegex(ValueError,'binding_policy_size'): policy.compile_source(*data)
    def test_parent_chain(self):
        for i,reason in enumerate(['binding_policy_codec_drift','codec_policy_startup_drift','startup_policy_proxy_drift',
                                  'proxy_policy_entry_drift','entry_policy_base_drift','startup_policy_profile_drift'],1):
            data=self.data.copy(); data[i]+=b' '
            with self.assertRaisesRegex(ValueError,reason): policy.compile_source(*data)
    def test_identity_boundary(self):
        for key,value,reason in [('id','bad"id','id'),('schemaVersion',True,'stage'),('stage','run-all','stage'),
                                ('reviewDocument','other.md','review'),('stopHex','0'*32,'stop_bytes'),('stopHex','90','stop_bytes')]:
            with self.assertRaisesRegex(ValueError,'binding_policy_'+reason): self.compile(dict(self.p,**{key:value}))
        for key in ['stopRva','procSlotRva']:
            for value in [True,0,-1,2**32]:
                with self.assertRaisesRegex(ValueError,'binding_policy_rva'): self.compile(dict(self.p,**{key:value}))
        with self.assertRaisesRegex(ValueError,'binding_policy_rva'): self.compile(dict(self.p,procSlotRva=3))
    def test_binding_set(self):
        for value in [[],self.p['bindings'][:7],self.p['bindings']*2]:
            with self.assertRaisesRegex(ValueError,'binding_policy_count'): self.compile(dict(self.p,bindings=value))
        for key,value in [('name','other'),('extra',True)]:
            p=copy.deepcopy(self.p); p['bindings'][0][key]=value
            with self.assertRaisesRegex(ValueError,'binding_policy_entry'): self.compile(p)
        p=copy.deepcopy(self.p); p['bindings'].reverse()
        with self.assertRaisesRegex(ValueError,'binding_policy_entry'): self.compile(p)
    def test_binding_ranges(self):
        for key in ['slotRva','targetRva']:
            for value in [True,0,-1,2**32]:
                p=copy.deepcopy(self.p); p['bindings'][0][key]=value
                with self.assertRaisesRegex(ValueError,'binding_policy_rva'): self.compile(p)
        p=copy.deepcopy(self.p); p['bindings'][0]['slotRva']+=1
        with self.assertRaisesRegex(ValueError,'binding_policy_rva'): self.compile(p)
        p=copy.deepcopy(self.p); p['bindings'][1]['slotRva']=p['bindings'][0]['slotRva']
        with self.assertRaisesRegex(ValueError,'binding_policy_duplicate_slot'): self.compile(p)


if __name__=='__main__': unittest.main()
