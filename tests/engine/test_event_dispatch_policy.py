from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import event_dispatch_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('extra',0),('id','/'),('stage','run-app'),('reviewDocument','no.md'),('instancePolicySha256','0'*64)]:self.reject(lambda p:p.update({k:v}))
    def test_ranges(self):
        for frame in ('dispatcher','application'):
            for field in ('callRva','targetRva'):
                for value in (True,0,-1,2**32):self.reject(lambda p:p[frame].update({field:value}))
    def test_frame_and_sample(self):
        for frame in ('dispatcher','application'):
            for field,value in [('callHex','e800000000'),('callHex','00'),('targetPrefixHex','00'),('extra',0)]:self.reject(lambda p:p[frame].update({field:value}))
        for field,value in [('applicationPrefixHex','00'*21),('branchHex','85c0740f536a18')]:self.reject(lambda p:p.update({field:value}))
        self.reject(lambda p:p['application'].update(callRva=p['application']['callRva']+1))
    def test_duplicate_size_parent(self):
        for value in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(value,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
