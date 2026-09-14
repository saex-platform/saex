from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import cwd_lock_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('extra',0),('id','/'),('stage','locked'),('reviewDocument','no.md'),('sehPolicySha256','0'*64)]:self.reject(lambda p:p.update({k:v}))
    def test_ranges(self):
        for v in (True,0,4095,2**32,self.p['tableRva']+1,self.p['selector']['targetRva']):self.reject(lambda p:p.update(tableRva=v))
        for k,v in [('callRva',True),('targetRva',2**32),('callHex','e800000000'),('extra',1)]:self.reject(lambda p:p['selector'].update({k:v}))
    def test_body_chain(self):
        for i in range(17):
            b=bytearray.fromhex(self.p['bodyHex']);b[i]^=1;self.reject(lambda p:p.update(bodyHex=b.hex()))
        self.reject(lambda p:p.update(stopPrefixHex='00'*16))
        self.reject(lambda p:p['selector'].update(targetPrefixHex='00'*16))
        self.reject(lambda p:p['selector'].update(callRva=p['selector']['callRva']+1))
    def test_duplicate_size_parent(self):
        for value in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(value,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
