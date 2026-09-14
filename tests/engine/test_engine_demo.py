"""Runner input failures produce honest local reports without starting GTA."""
import json,os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
@unittest.skipUnless(os.name=='nt','Windows PowerShell runner')
class EngineDemoTests(unittest.TestCase):
    def run_failed(self,directory):
        shell=shutil.which('pwsh') or shutil.which('powershell')
        self.assertIsNotNone(shell)
        r=subprocess.run([shell,'-NoProfile','-File',str(ROOT/'tools/Run-SAEX.ps1'),'-GameDirectory',str(directory)],
            capture_output=True,timeout=20,creationflags=subprocess.CREATE_NO_WINDOW)
        self.assertEqual(r.returncode,1,r.stderr.decode(errors='replace'))
        line=next(v for v in r.stdout.decode('utf-8-sig').splitlines() if v.startswith('Rapor: '))
        report=Path(line[len('Rapor: '):])
        self.assertTrue(report.resolve().is_relative_to((ROOT/'out/local-demo').resolve()))
        v=json.loads(report.with_suffix('.json').read_text(encoding='utf-8'))
        self.assertEqual(v['status'],'failed');self.assertFalse(v['playable']);self.assertFalse(v['multiplayerReady'])
        self.assertIsNone(v['probeExitCode']);self.assertTrue(v['failure'])
        self.assertFalse((report.parent/'game').exists());self.assertFalse((report.parent/'engine-trace.json').exists())
        html=report.read_text(encoding='utf-8')
        self.assertNotIn('href="engine-trace.json"',html);self.assertIn('Doğrulama durdu.',html)
        return v
    def test_missing_directory_report(self):
        # Reports persist as local evidence; the test owns only its temporary input directory.
        with tempfile.TemporaryDirectory(prefix='saex-demo-missing-') as folder:
            v=self.run_failed(Path(folder)/'missing')
            self.assertFalse(v['inputHashesPreserved'])
    def test_unknown_executable_preserved(self):
        with tempfile.TemporaryDirectory(prefix='saex-demo-unknown-') as folder:
            path=Path(folder)/'gta_sa.exe';data=b'not a reviewed executable';path.write_bytes(data)
            v=self.run_failed(folder)
            self.assertTrue(v['inputHashesPreserved']);self.assertEqual(path.read_bytes(),data)
if __name__=='__main__':unittest.main()
