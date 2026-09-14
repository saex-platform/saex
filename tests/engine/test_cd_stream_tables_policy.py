import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import cd_stream_tables_policy as policy
class PolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.inputs=[(ROOT/p).read_bytes() for p in policy.SOURCES];cls.source=json.loads(cls.inputs[0])
    def compile(self,p):return policy.compile_source(json.dumps(p).encode(),*self.inputs[1:])
    def test_generated(self):self.assertEqual(policy.compile_source(*self.inputs),(ROOT/policy.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for field,value in [('schemaVersion',True),('stage','ready'),('id','X'),('readyPolicySha256','0'*64),('reviewDocument','other.md'),('diskImport','ReadFile')]:
            with self.subTest(field=field):
                p=copy.deepcopy(self.source);p[field]=value
                with self.assertRaises(ValueError):self.compile(p)
    def test_ranges(self):
        for field in ('targetRva','handlesRva','namesRva','diskIatRva'):
            for value in (0,True,-1,0xffffffff,'4096'):
                with self.subTest(field=field,value=value):
                    p=copy.deepcopy(self.source);p[field]=value
                    with self.assertRaises(ValueError):self.compile(p)
    def test_body(self):
        for i in range(74):
            p=copy.deepcopy(self.source);b=bytearray.fromhex(p['bodyHex']);b[i]^=1;p['bodyHex']=b.hex()
            with self.assertRaises(ValueError):self.compile(p)
    def test_unknown_missing_duplicate(self):
        p=copy.deepcopy(self.source);p['unknown']=1
        with self.assertRaises(ValueError):self.compile(p)
        for key in self.source:
            p=copy.deepcopy(self.source);del p[key]
            with self.assertRaises(ValueError):self.compile(p)
        with self.assertRaises(ValueError):policy.compile_source(self.inputs[0].replace(b'{',b'{"schemaVersion":1,',1),*self.inputs[1:])
    def test_parent_and_budget(self):
        parents=list(self.inputs[1:]);parents[0]+=b' '
        with self.assertRaises(ValueError):policy.compile_source(self.inputs[0],*parents)
        with self.assertRaises(ValueError):policy.compile_source(b' '*65537,*self.inputs[1:])
if __name__=='__main__':unittest.main()
