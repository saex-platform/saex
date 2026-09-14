from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import cwd_return_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity_parent(self):
        for k,v in [('schemaVersion',True),('id','/'),('stage','helper-return'),('copyPolicySha256','1'*64),('extra',0),('reviewDocument','no.md')]:self.reject(lambda p:p.update({k:v}))
    def test_code_ranges_overlap(self):
        for value in (True,0,4095,2**32):self.reject(lambda p:p.update(checkerRva=value))
        q=json.loads(self.data[2]);self.reject(lambda p:p.update(checkerRva=q['helperRva']))
        self.reject(lambda p:p.update(checkerRva=q['cookieRva']))
    def test_every_admitted_byte(self):
        for key in ('cleanupHex','epilogueHex','checkerHex'):
            b=bytearray.fromhex(self.p[key])
            for i in range(len(b)):
                changed=b.copy();changed[i]^=1
                with self.subTest(segment=key,byte=i):self.reject(lambda p:p.update({key:changed.hex()}))
        for value in ('zz'*16,'00'*15,None):self.reject(lambda p:p.update(returnPrefixHex=value))
    def test_duplicate_size(self):
        for data in (b' '*65537,self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1')):
            with self.assertRaises(ValueError):tool.compile_source(data,*self.data[1:])
    def test_all_ancestors_stay_pinned(self):
        for index in range(1,len(self.data)):
            data=list(self.data);data[index]+=b'\n'
            with self.subTest(source=str(tool.SOURCES[index])):
                with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
