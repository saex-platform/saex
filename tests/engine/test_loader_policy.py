"""Policy compiler rejects scope escalation, drift and ambiguous source data."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('loader_policy', ROOT / 'tools/loader_policy.py')
policy_tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy_tool)


class LoaderPolicyTests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT / policy_tool.SOURCE).read_bytes()
        self.policy = json.loads(self.data)

    def rejected(self, value, why):
        with self.assertRaisesRegex(ValueError, '^' + why + '$'): policy_tool.validate(value)

    def test_exact_generated_source(self):
        output = policy_tool.generate(self.policy, hashlib.sha256(self.data).hexdigest())
        self.assertEqual(output, (ROOT / policy_tool.OUTPUT).read_text(encoding='utf-8'))
        changed = copy.deepcopy(self.policy); changed['modules'][0]['sha256'] = '1' * 64
        self.assertNotEqual(output, policy_tool.generate(changed, '0' * 64))

    def test_scope_escalation(self):
        for stage in ('initialized','active',None):
            p=copy.deepcopy(self.policy); p['stage']=stage; self.rejected(p,'policy_must_remain_mapping_only')
        self.policy['schemaVersion']=True; self.rejected(self.policy,'policy_must_remain_mapping_only')

    def test_unknown_and_duplicate_keys(self):
        self.policy['canAttach']=True; self.rejected(self.policy,'policy_keys')
        with self.assertRaisesRegex(ValueError,'duplicate_json_key'):
            json.loads('{"stage":"first-exception-only","stage":"active"}',object_pairs_hook=policy_tool.strict_object)

    def test_name_paths_and_case(self):
        for name in ('../bad.dll','C:/bad.dll','Bad.dll','x.dll:stream','x.asi','x".dll','x' * 128 + '.dll'):
            p=copy.deepcopy(self.policy); p['modules'][0]['name']=name; self.rejected(p,'policy_module_name')

    def test_hashes_and_numbers(self):
        for key,value,why in [('bytes',True,'policy_module_bytes'),('bytes',0,'policy_module_bytes'),('bytes',64*1024*1024+1,'policy_module_bytes'),('sha256','0'*64,'policy_module_hash'),('sha256','F'*64,'policy_module_hash')]:
            p=copy.deepcopy(self.policy); p['modules'][0][key]=value; self.rejected(p,why)

    def test_origin_and_duplicate_basename(self):
        p=copy.deepcopy(self.policy); p['modules'][0]['origin']='anywhere'; self.rejected(p,'policy_origin')
        p=copy.deepcopy(self.policy); p['modules'][1]['name']=p['modules'][0]['name']; self.rejected(p,'policy_duplicate_module')

    def test_order_and_collection_limits(self):
        p=copy.deepcopy(self.policy); p['modules'].reverse(); self.rejected(p,'policy_module_order')
        for modules in ([], [self.policy['modules'][0]]*65):
            p=copy.deepcopy(self.policy); p['modules']=modules; self.rejected(p,'policy_module_limit')
        p=copy.deepcopy(self.policy)
        for m in p['modules']: m['bytes']=64*1024*1024
        self.rejected(p,'policy_total_bytes')

    def test_review_and_engine_binding(self):
        p=copy.deepcopy(self.policy); p['reviewDocument']='unreviewed.md'; self.rejected(p,'policy_review_document')
        p=copy.deepcopy(self.policy); p['engineSha256']=''; self.rejected(p,'policy_engine_hash')
        p=copy.deepcopy(self.policy); p['modules'][0]['basis']='trust-me'; self.rejected(p,'policy_module_basis')


if __name__ == '__main__': unittest.main()
