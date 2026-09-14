import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import file_manager_ready_policy as policy
class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.data=[(ROOT/p).read_bytes() for p in policy.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,key,value):
        p=copy.deepcopy(self.p);p[key]=value
        with self.assertRaises(ValueError):policy.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(policy.compile_source(*self.data),(ROOT/policy.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('stage','before-return'),('id','../unsafe'),('reviewDocument','x.md'),('extra',1)]:self.reject(k,v)
    def test_parent(self):self.reject('returnPolicySha256','0'*64)
    def test_addresses(self):
        for k in ('unlockRva','iatRva','epilogueRva'):
            for v in (True,-1,0,4095,0xffffffff):self.reject(k,v)
        self.reject('iatRva',self.p['iatRva']+1);self.reject('epilogueRva',self.p['unlockRva'])
    def test_executable_bodies(self):
        for k in ('wrapperHex','cleanupHex','unlockHex','epilogueHex'):
            self.reject(k,'90'+self.p[k][2:]);self.reject(k,self.p[k][:-2])
    def test_system_identity(self):
        p=copy.deepcopy(self.p['systemModule']);p['sha256']='0'*64;self.reject('systemModule',p)
        p=copy.deepcopy(self.p['function']);p['highlowOffsets']=[1];self.reject('function',p)
        p=copy.deepcopy(self.p['function']);p['rva']=True;self.reject('function',p)
    def test_duplicate(self):
        with self.assertRaises(ValueError):policy.compile_source(b'{"id":1,"id":2}',*self.data[1:])
if __name__=='__main__':unittest.main()
