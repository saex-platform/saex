from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import instance_startup_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('id','/'),('extra',0),('reviewDocument','no.md'),('suppressionPolicySha256','0'*64),('stage','activate-window')]:self.reject(lambda p:p.update({k:v}))
    def test_modules(self):
        self.reject(lambda p:p['systemModules'].reverse())
        self.reject(lambda p:p['systemModules'][0].update(sha256='0'*64))
        self.reject(lambda p:p['systemModules'].append(p['systemModules'][0]))
    def test_body(self):
        for i in range(91):
            def change(p):
                b=bytearray.fromhex(p['bodyHex']);b[i]^=0x80;p['bodyHex']=b.hex()
            if not any(o<=i<o+4 for o in tool.OFFSETS):self.reject(change)
        for v in ('e9aee1ffff','e8afe1ffff','',True):self.reject(lambda p:p.update(callHex=v))
    def test_ranges(self):
        for k in ('callRva','targetRva','nameRva','createThunkRva','createThunkSlotRva'):
            for v in (True,0,-1,2**32):self.reject(lambda p:p.update({k:v}))
        self.reject(lambda p:p.update(nameRva=0x4d6224))
        self.reject(lambda p:p.update(name='anything'))
    def test_function(self):
        for k,v in [('rva',0),('rva',True),('prefixHex','00'),('preferredBase',1),('highlowOffsets',[0])]:self.reject(lambda p:p['createFunction'].update({k:v}))
    def test_duplicate_size_parent(self):
        for data in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(data,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
