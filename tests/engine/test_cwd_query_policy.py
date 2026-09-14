from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import cwd_query_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity_and_pin(self):
        for k,v in [('schemaVersion',True),('extra',0),('id','/'),('stage','copy'),('reviewDocument','no.md'),('acquirePolicySha256','0'*64)]:self.reject(lambda p:p.update({k:v}))
        for i in range(2):
            self.reject(lambda p:p['systemModules'][i].update(sha256='0'*64));self.reject(lambda p:p['systemModules'][i].update(bytes=True))
    def test_ranges_overlap(self):
        for k in ('helperRva','cookieRva','iatRva','thunkSlotRva'):
            for v in (True,0,4095,2**32):self.reject(lambda p:p.update({k:v}))
        self.reject(lambda p:p.update(cookieRva=p['iatRva']))
        self.reject(lambda p:p.update(iatRva=p['iatRva']+1))
    def test_body(self):
        for key in ('wrapperHex','helperHex','queryHex'):
            original=bytearray.fromhex(self.p[key])
            for i in range(len(original)):
                b=original.copy();b[i]^=1;self.reject(lambda p:p.update({key:b.hex()}))
        self.reject(lambda p:p.update(returnPrefixHex='zz'*16))
    def test_exports_and_fixups(self):
        for key in ('function','implementation'):
            for k,v in [('rva',True),('rva',2**32),('preferredBase',1),('highlowOffsets',[True]),('highlowOffsets',[19]),('highlowOffsets',[4,4]),('highlowOffsets',[8,4]),('highlowOffsets',[3,4]),('extra',1)]:self.reject(lambda p:p[key].update({k:v}))
        self.reject(lambda p:p['function'].update(highlowOffsets=[]))
        self.reject(lambda p:p.update(thunkSlotRva=p['thunkSlotRva']+4))
    def test_duplicate_size_parent(self):
        for value in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(value,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
    def test_every_ancestor_remains_exact(self):
        # Even an otherwise valid ancestor with only whitespace changed must be
        # reviewed through its dependent digest chain; the leaf never re-pins it.
        for index,path in enumerate(tool.SOURCES[1:],1):
            with self.subTest(source=str(path)):
                data=list(self.data);data[index]+=b'\n'
                with self.assertRaises(ValueError):tool.compile_source(*data)
    def test_mixed_system_versions_at_leaf(self):
        # A valid-looking alternate module hash/size is not a compatible OS
        # profile. Both kernel32 and kernelbase must match the reviewed root.
        for index in range(2):
            for field,value in [('sha256','1'*64),('bytes',self.p['systemModules'][index]['bytes']+1)]:
                with self.subTest(module=index,field=field):
                    p=copy.deepcopy(self.p);p['systemModules'][index][field]=value
                    with self.assertRaisesRegex(ValueError,'^cwd_query_module$'):
                        tool.compile_source(json.dumps(p).encode(),*self.data[1:])
if __name__=='__main__':unittest.main()
