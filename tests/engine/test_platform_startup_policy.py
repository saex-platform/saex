from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import platform_startup_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('id','/'),('stage','after-call'),('extra',0),('reviewDocument','no.md'),('applicationPolicySha256','0'*64)]:self.reject(lambda p:p.update({k:v}))
        self.reject(lambda p:p['systemModule'].update(sha256='0'*64))
    def test_abi(self):
        for k,v in [('stackBytes',148),('stackBytes',True),('arguments',[8193,0,0,3]),('arguments',[8193,False,0,2])]:self.reject(lambda p:p.update({k:v}))
    def test_ranges(self):
        for k in ('callRva','iatRva'):
            for v in (True,0,-1,2**32):self.reject(lambda p:p.update({k:v}))
        self.reject(lambda p:p.update(iatRva=p['callRva']&~3))
    def test_hex(self):
        for k in ('prologueHex','returnPrefixHex'):
            for v in (None,'00','gg','0'):self.reject(lambda p:p.update({k:v}))
        self.reject(lambda p:p.update(prologueHex='00'*257))
    def test_function(self):
        for k,v in [('rva',0),('rva',True),('prefixHex','00'),('preferredBase',1),('highlowOffsets',[0,1]),('highlowOffsets',[17]),('highlowOffsets',[True])]:self.reject(lambda p:p['function'].update({k:v}))
    def test_duplicate_size_parent(self):
        for data in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(data,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
