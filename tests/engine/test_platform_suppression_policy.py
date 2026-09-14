from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import platform_suppression_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('id','/'),('extra',0),('reviewDocument','no.md'),('platformPolicySha256','0'*64)]:self.reject(lambda p:p.update({k:v}))
        self.reject(lambda p:p['lastErrorModule'].update(sha256='0'*64))
    def test_behavior(self):
        for k,v in [('method','api-hook'),('returnValue',True),('returnValue',1),('preserveLastError',1),('preserveLastError',False)]:self.reject(lambda p:p.update({k:v}))
    def test_ranges(self):
        for v in (True,0,-1,2**32,4555372):self.reject(lambda p:p.update(lastErrorIatRva=v))
    def test_function(self):
        for k,v in [('rva',0),('rva',True),('prefixHex','00'),('prefixHex','64a130000000c3'+'cc'*13),('preferredBase',1),('highlowOffsets',[0])]:self.reject(lambda p:p['lastErrorFunction'].update({k:v}))
    def test_duplicate_size_parent(self):
        for data in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(data,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
