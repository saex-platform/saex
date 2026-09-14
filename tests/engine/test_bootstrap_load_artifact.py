from pathlib import Path
import hashlib
import sys
import unittest
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import bootstrap_load_artifact as tool
DLL, MAP = map(Path,sys.argv[1:3]); del sys.argv[1:3]


class ArtifactBindingTests(unittest.TestCase):
    def test_current_identity(self):
        data=DLL.read_bytes(); link_map=MAP.read_bytes().decode('utf-8')
        output=tool.compile_artifact(data,link_map)
        self.assertIn(hashlib.sha256(data).hexdigest(),output)
        self.assertIn(hashlib.sha256(MAP.read_bytes()).hexdigest(),output)
        self.assertIn(f'{len(data)}ULL',output)
        self.assertIn('"saex_bootstrap.asi"',output)
        self.assertIn('bootstrap_artifact_exports',output)
        result=tool.check_bootstrap.audit(data,link_map)
        for entry in result['exportEntries'].values():
            self.assertIn(f'BootstrapExportSpec{{{entry["rva"]}U',output)
        self.assertEqual(output,tool.compile_artifact(data,link_map))
    def test_audit_cannot_be_skipped(self):
        with self.assertRaises(ValueError): tool.compile_artifact(b'bad',MAP.read_bytes().decode('utf-8'))
        with self.assertRaises(ValueError): tool.compile_artifact(DLL.read_bytes(),'unrelated map')
        with self.assertRaises(ValueError): tool.compile_artifact(DLL.read_bytes(),MAP.read_bytes().decode('utf-8')+'\n.CRT$XCU\n')
    def test_artifact_drift_rebinds(self):
        data=DLL.read_bytes(); link_map=MAP.read_bytes().decode('utf-8')
        # Overlay changes keep the structural audit valid but MUST change the observer pin.
        self.assertNotEqual(tool.compile_artifact(data,link_map),tool.compile_artifact(data+b'overlay',link_map))


if __name__=='__main__': unittest.main()
