"""Entry permission is explicit, bounded and bound to the unchanged mapping recipe."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import entry_policy
import loader_policy


class EntryPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/entry_policy.SOURCE).read_bytes()
        self.base=(ROOT/loader_policy.SOURCE).read_bytes()
        self.policy=json.loads(self.data)

    def compile(self,policy): return entry_policy.compile_source(json.dumps(policy).encode(),self.base)

    def test_exact_output_and_base_unchanged(self):
        self.assertEqual(entry_policy.compile_source(self.data,self.base),(ROOT/entry_policy.OUTPUT).read_text(encoding='utf-8'))
        self.assertEqual(json.loads(self.base)['stage'],'first-exception-only')
        changed=copy.deepcopy(self.policy); changed['additionalModules'][0]['sha256']='1'*64
        self.assertNotEqual(self.compile(changed),self.compile(self.policy))

    def test_base_and_engine_drift(self):
        with self.assertRaisesRegex(ValueError,'entry_policy_base_drift'):
            entry_policy.compile_source(self.data,self.base+b' ')
        self.policy['engineSha256']='1'*64
        with self.assertRaisesRegex(ValueError,'entry_policy_engine_drift'): self.compile(self.policy)

    def test_stage_and_boolean_schema(self):
        for key,value in [('stage','initialized'),('stage','first-exception-only'),('schemaVersion',True)]:
            p=copy.deepcopy(self.policy); p[key]=value
            with self.assertRaisesRegex(ValueError,'entry_policy_stage'): self.compile(p)

    def test_unknown_duplicate_keys_and_size(self):
        self.policy['allowAll']=True
        with self.assertRaisesRegex(ValueError,'entry_policy_keys'): self.compile(self.policy)
        with self.assertRaisesRegex(ValueError,'duplicate_json_key'):
            entry_policy.compile_source(b'{"stage":1,"stage":2}',self.base)
        with self.assertRaisesRegex(ValueError,'entry_policy_source_size'):
            entry_policy.compile_source(b' '*65537,self.base)

    def test_supplement_cannot_override_base_or_load_game_dll(self):
        self.policy['additionalModules']=[json.loads(self.base)['modules'][0]]
        with self.assertRaisesRegex(ValueError,'policy_duplicate_module'): self.compile(self.policy)
        self.policy['additionalModules'][0]['origin']='game-root'
        with self.assertRaisesRegex(ValueError,'entry_policy_additional_origin'): self.compile(self.policy)

    def test_module_bounds_hash_and_path(self):
        for key,value in [('bytes',True),('bytes',0),('sha256','0'*64),('name','../imm32.dll')]:
            p=copy.deepcopy(self.policy); p['additionalModules'][0][key]=value
            with self.assertRaises(ValueError): self.compile(p)

    def test_supplement_count_and_order(self):
        for extra in [[],self.policy['additionalModules']*9]:
            p=copy.deepcopy(self.policy); p['additionalModules']=extra
            with self.assertRaisesRegex(ValueError,'entry_policy_additional_limit'): self.compile(p)
        second=dict(self.policy['additionalModules'][0],name='aaa.dll')
        self.policy['additionalModules'].append(second)
        with self.assertRaisesRegex(ValueError,'policy_module_order'): self.compile(self.policy)

    def test_id_and_review(self):
        self.policy['id']='bad"id'
        with self.assertRaisesRegex(ValueError,'entry_policy_id'): self.compile(self.policy)
        self.policy=json.loads(self.data); self.policy['reviewDocument']='unchecked.md'
        with self.assertRaisesRegex(ValueError,'entry_policy_review'): self.compile(self.policy)


if __name__=='__main__': unittest.main()
