from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import startup_return_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def rejects(self,key,value):
        p=copy.deepcopy(self.p);p[key]=value
        with self.assertRaises(ValueError): tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):
        self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_shape(self):
        for k,v in [('schemaVersion',True),('id','/'),('stage','frame'),('extra',0),('reviewDocument','none.md')]: self.rejects(k,v)
    def test_chain(self):
        self.rejects('framePolicySha256','0'*64)
        for i in [1,2,3]:
            p=list(self.data);p[i]+=b'\n'
            with self.assertRaises(ValueError): tool.compile_source(*p)
    def test_ranges(self):
        self.rejects('resultProtections',[64]);self.rejects('resultProtections',[64,32]);self.rejects('resultProtections',[128,64])
        self.rejects('resultProtections',[64.0,128])
        for k in ['protectCallRva','protectSlotRva','startupSlotRva','forwardRva']:
            for v in [True,0,-1,2**32]: self.rejects(k,v)
        self.rejects('startupSlotRva',self.p['protectSlotRva']);self.rejects('protectSlotRva',1)
    def test_functions(self):
        for k in ['protectFunction','startupFunction']:
            for field,value in [('rva',0),('prefixHex','00'),('preferredBase',1),('highlowOffsets',[8,9]),('highlowOffsets',[17]),('highlowOffsets',[8,8]),('highlowOffsets',[True])]:
                f=dict(self.p[k]);f[field]=value;self.rejects(k,f)
    def test_module_prefix(self):
        for field,value in [('name','other.dll'),('sha256','0'*64),('bytes',True),('bytes',float(self.p['systemModule']['bytes']))]:
            f=dict(self.p['systemModule']);f[field]=value;self.rejects('systemModule',f)
        self.rejects('protectReturnHex','gg')
    def test_duplicate_size(self):
        for data in [self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537]:
            with self.assertRaises(ValueError): tool.compile_source(data,*self.data[1:])
if __name__=='__main__':unittest.main()
