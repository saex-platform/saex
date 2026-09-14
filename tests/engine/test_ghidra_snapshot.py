"""Offline corpus tests; no Java, Ghidra, bridge, game or network required."""
import argparse,copy,json,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/research'))
import ghidra_snapshot as snapshot
class SnapshotTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.path=Path(self.temp.name)
        lock=snapshot.load(snapshot.LOCK);address=lock['roots'][0]
        self.address=address
        self.write(address+'.json',{'address':address,'decompiled':'void fixture() {}','assembly':['fixture']})
        self.write('_index.json',{address:{'address':address}})
        self.record={'schemaVersion':1,'executableSha256':lock['executableSha256'],'lockSha256':snapshot.digest(snapshot.LOCK),
            'roots':lock['roots'],'addresses':[address],'missingRoots':sorted(set(lock['roots'])-{address}),
            'runtimePermission':False,'completeCallClosure':False,'issues':[],
            'files':{p.name:snapshot.digest(p) for p in self.path.glob('*.json')}}
        self.save()
    def write(self,name,data):(self.path/name).write_text(json.dumps(data),encoding='utf-8')
    def save(self):self.write('snapshot.json',self.record)
    def rejected(self):
        self.save()
        with self.assertRaises(ValueError):snapshot.verify(self.path)
    def test_partial_explicit(self):self.assertEqual(snapshot.verify(self.path)['missingRoots'],self.record['missingRoots'])
    def test_identity(self):
        for key,value in [('schemaVersion',True),('executableSha256','0'*64),('lockSha256','0'*64)]:
            old=self.record[key];self.record[key]=value;self.rejected();self.record[key]=old
    def test_no_permission_claim(self):
        for key in ('runtimePermission','completeCallClosure'):
            self.record[key]=True;self.rejected();self.record[key]=False
    def test_missing_roots_and_duplicate(self):
        old=self.record['missingRoots'];self.record['missingRoots']=[];self.rejected();self.record['missingRoots']=old
        self.record['addresses']*=2;self.rejected()
    def test_mutated_export(self):
        self.write(self.address+'.json',{'changed':True})
        with self.assertRaisesRegex(ValueError,'integrity'):snapshot.verify(self.path)
    def test_path_and_missing_file(self):
        self.record['files']['../escape.json']='0'*64;self.rejected();del self.record['files']['../escape.json']
        (self.path/(self.address+'.json')).unlink();self.rejected()
    def test_duplicate_json(self):
        (self.path/'snapshot.json').write_text('{"schemaVersion":1,"schemaVersion":2}',encoding='utf-8')
        with self.assertRaisesRegex(ValueError,'duplicate'):snapshot.verify(self.path)
    def test_index_mismatch(self):
        self.write('_index.json',{});self.record['files']['_index.json']=snapshot.digest(self.path/'_index.json');self.rejected()
    def test_export_malformed(self):
        self.write(self.address+'.json',{'address':self.address,'decompiled':5,'assembly':[]})
        self.record['files'][self.address+'.json']=snapshot.digest(self.path/(self.address+'.json'));self.rejected()
    def test_unknown_exe_before_import(self):
        path=self.path/'unknown.exe';path.write_bytes(b'MZ invalid fixture')
        with self.assertRaisesRegex(ValueError,'executable_identity'):snapshot.export(argparse.Namespace(executable=path))
if __name__=='__main__':unittest.main()
