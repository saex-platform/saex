from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import crt_startup_policy as tool

class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):
        self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_shape(self):
        for k,v in [('schemaVersion',True),('id','/'),('stage','after-initializers'),('extra',0),('reviewDocument','no.md')]:
            self.reject(lambda p:p.update({k:v}))
    def test_parent(self):
        self.reject(lambda p:p.update(startupReturnPolicySha256='0'*64))
        for i in (1,2,3):
            data=list(self.data);data[i]+=b'\n'
            with self.assertRaises(ValueError):tool.compile_source(*data)
    def test_calls(self):
        for k in ('io','initialize','application'):
            for field,value in [('callRva',True),('callRva',0),('callRva',2**32),('targetRva',1),('callHex','e900000000'),('targetPrefixHex','00')]:
                self.reject(lambda p:p[k].update({field:value}))
        self.reject(lambda p:p['application'].update(p['initialize']))
    def test_stack_prefix(self):
        for value in (True,0,15,17,4097,88.0):self.reject(lambda p:p.update(ioReturnStackOffset=value))
        self.reject(lambda p:p.update(ioReturnHex='0'*31))
    def test_tables(self):
        self.reject(lambda p:p.update(tables=[]))
        for field,value in [('name','other'),('rva',True),('rva',1),('rva',2**32),('targets',[]),('targets',[True]),('targets',[-1]),('targets',[0]*2049),('targets',[1]),('targets',[2**32])]:
            self.reject(lambda p:p['tables'][0].update({field:value}))
        self.reject(lambda p:p['tables'][0].update(rva=p['tables'][1]['rva']))
        self.reject(lambda p:p['tables'][0].update(rva=p['io']['targetRva']))
        self.reject(lambda p:p['tables'][0].update(targets=[p['tables'][1]['rva']]))
    def test_duplicate_and_size(self):
        for data in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(data,*self.data[1:])
if __name__=='__main__':unittest.main()
