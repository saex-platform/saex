import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import cd_stream_allocation_policy as policy
class PolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.inputs=[(ROOT/p).read_bytes() for p in policy.SOURCES];cls.source=json.loads(cls.inputs[0])
    def compile(self,p):return policy.compile_source(json.dumps(p).encode(),*self.inputs[1:])
    def rejected(self,p):
        with self.assertRaises(ValueError):self.compile(p)
    def test_generated(self):self.assertEqual(policy.compile_source(*self.inputs),(ROOT/policy.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for field,value in [('schemaVersion',True),('stage','ready'),('id','X'),('diskPolicySha256','0'*64),('reviewDocument','other.md'),('heapExport','HeapAlloc'),('eventLimit',159),('eventLimit',161),('eventLimit',True)]:
            p=copy.deepcopy(self.source);p[field]=value;self.rejected(p)
    def test_ranges(self):
        for field in policy.FIELDS:
            for value in (0,True,-1,0xffffffff,'4096'):
                p=copy.deepcopy(self.source);p[field]=value;self.rejected(p)
        p=copy.deepcopy(self.source);p['heapModeRva']=p['heapHandleRva'];self.rejected(p)
    def test_body(self):
        for name in policy.TEMPLATES:
            field=name+'Hex'
            for i in range(len(bytes.fromhex(self.source[field]))):
                p=copy.deepcopy(self.source);b=bytearray.fromhex(p[field]);b[i]^=1;p[field]=b.hex();self.rejected(p)
    def test_unknown_missing_duplicate(self):
        p=copy.deepcopy(self.source);p['unknown']=1;self.rejected(p)
        for key in self.source:
            p=copy.deepcopy(self.source);del p[key];self.rejected(p)
        with self.assertRaises(ValueError):policy.compile_source(self.inputs[0].replace(b'{',b'{"schemaVersion":1,',1),*self.inputs[1:])
    def test_parent_and_budget(self):
        parents=list(self.inputs[1:]);parents[0]+=b' '
        with self.assertRaises(ValueError):policy.compile_source(self.inputs[0],*parents)
        with self.assertRaises(ValueError):policy.compile_source(b' '*65537,*self.inputs[1:])
    def test_system_pin(self):
        for key,value in [('sha256','0'*64),('bytes',True),('name','other.dll')]:
            p=copy.deepcopy(self.source);p['systemModule'][key]=value;self.rejected(p)
    def test_export_recipe(self):
        for field,value in [('rva',True),('rva',0xffffffff),('preferredBase',1),('highlowOffsets',[2,3]),('highlowOffsets',[True]),('prefixHex','0'),('prefixHex',0)]:
            p=copy.deepcopy(self.source);p['function'][field]=value;self.rejected(p)
if __name__=='__main__':unittest.main()
