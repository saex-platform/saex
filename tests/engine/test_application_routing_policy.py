from pathlib import Path
import copy,json,sys,unittest
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import application_routing_policy as tool
class PolicyTests(unittest.TestCase):
    def setUp(self):self.data=[(ROOT/p).read_bytes() for p in tool.SOURCES];self.p=json.loads(self.data[0])
    def reject(self,modify):
        p=copy.deepcopy(self.p);modify(p)
        with self.assertRaises(ValueError):tool.compile_source(json.dumps(p).encode(),*self.data[1:])
    def test_generated(self):self.assertEqual(tool.compile_source(*self.data),(ROOT/tool.OUTPUT).read_text(encoding='utf-8'))
    def test_identity(self):
        for k,v in [('schemaVersion',True),('extra',0),('id','/'),('stage','run-init'),('reviewDocument','no.md'),('dispatchPolicySha256','0'*64)]:self.reject(lambda p:p.update({k:v}))
    def test_ranges_and_operands(self):
        for k in ('detourRva','indexRva','tableRva'):
            for v in (True,0,-1,2**32):self.reject(lambda p:p.update({k:v}))
        for k,n in [('detourHex',12),('indirectHex',7)]:
            for v in ('00','00'*n):self.reject(lambda p:p.update({k:v}))
        for k,v in [('callRva',True),('targetRva',2**32),('callHex','e800000000'),('targetPrefixHex','00'),('extra',1)]:self.reject(lambda p:p['initializer'].update({k:v}))
    def test_selection_and_tables(self):
        for v in ('00','ff'*39,'00'*39):self.reject(lambda p:p.update(indicesHex=v))
        for v in ([],[0]*11,[True]*11,[2**32]*11):self.reject(lambda p:p.update(targetRvas=v))
        self.reject(lambda p:p['targetRvas'].__setitem__(5,p['targetRvas'][6]))
        self.reject(lambda p:p['targetRvas'].__setitem__(10,p['targetRvas'][9]))
        self.reject(lambda p:p.update(tableRva=p['indexRva']))
    def test_duplicate_size_parent(self):
        for value in (self.data[0].replace(b'"schemaVersion": 1',b'"schemaVersion": 1, "schemaVersion": 1'),b' '*65537):
            with self.assertRaises(ValueError):tool.compile_source(value,*self.data[1:])
        data=list(self.data);data[1]+=b'\n'
        with self.assertRaises(ValueError):tool.compile_source(*data)
if __name__=='__main__':unittest.main()
