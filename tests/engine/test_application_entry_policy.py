from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import application_entry_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,k,v):
        p=copy.deepcopy(self.p);p[k]=v
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):
        self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_shape(self):
        for k,v in [('schemaVersion',True),('id','/'),('stage','after-body'),('extra',0),('reviewDocument','no.md')]:self.reject(k,v)
    def test_parent(self):
        self.reject('crtPolicySha256','0'*64)
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
    def test_ranges(self):
        for key in ('secondCallRva','onceRva','reentryRva'):
            for v in (True,0,-1,2**32):self.reject(key,v)
        crt=json.loads(self.data[1]);self.reject('secondCallRva',crt['initialize']['callRva']);self.reject('onceRva',self.p['reentryRva'])
    def test_hex(self):
        for k in ('initializerBodyHex','initializerReturnHex','secondReturnHex','reentryNormalizedHex'):
            for v in (None,'gg','0','00'):self.reject(k,v)
        self.reject('initializerBodyHex','00'*257)
        self.reject('initializerBodyHex','00'*16)
    def test_guard(self):
        for i in (0,5,11,19,27):
            b=bytearray.fromhex(self.p['reentryNormalizedHex']);b[i]^=1;self.reject('reentryNormalizedHex',b.hex())
        b=bytearray.fromhex(self.p['reentryNormalizedHex']);b[13:17]=(-17).to_bytes(4,'little',signed=True);self.reject('reentryNormalizedHex',b.hex())
    def test_duplicate_size(self):
        for data in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(data,*self.data[1:])
if __name__=='__main__':unittest.main()
