import importlib.util
from pathlib import Path
import struct
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("bootstrap_audit", ROOT / "tools/check_bootstrap.py")
checker = importlib.util.module_from_spec(spec); spec.loader.exec_module(checker)
DLL = Path(sys.argv.pop(1)).read_bytes()
MAP = Path(sys.argv.pop(1)).read_bytes().decode("utf-8")


class BootstrapAuditTests(unittest.TestCase):
    @staticmethod
    def layout(data):
        pe = struct.unpack_from('<I',data,60)[0]; opt=pe+24
        table=opt+struct.unpack_from('<H',data,pe+20)[0]
        sections=[struct.unpack_from('<IIII',data,table+i*40+8) for i in range(struct.unpack_from('<H',data,pe+6)[0])]
        def offset(rva):
            return next(raw+rva-start for _,start,size,raw in sections if start<=rva<start+size)
        return opt,table,offset

    def test_actual_artifact(self):
        result = checker.audit(DLL, MAP)
        self.assertFalse(result["canAttach"])
        self.assertEqual(set(result["exports"]), checker.EXPORTS)

    def test_x64_refused(self):
        data = bytearray(DLL); pe = struct.unpack_from("<I", data, 60)[0]
        struct.pack_into("<H", data, pe + 4, 0x8664)
        with self.assertRaisesRegex(ValueError, "x86"): checker.audit(data, MAP)

    def test_tls_callback_directory_refused(self):
        data = bytearray(DLL); pe = struct.unpack_from("<I", data, 60)[0]
        struct.pack_into("<II", data, pe + 24 + 96 + 9 * 8, 4096, 24)
        with self.assertRaisesRegex(ValueError, "tls_directory"): checker.audit(data, MAP)

    def test_unexpected_dependency_refused(self):
        self.assertIn(b"bcrypt.dll\0", DLL)
        changed = DLL.replace(b"bcrypt.dll\0", b"unsafe.dll\0")
        with self.assertRaisesRegex(ValueError, "unexpected_import"): checker.audit(changed, MAP)

    def test_export_drift_refused(self):
        changed = DLL.replace(b"SaexBootstrapQuery\0", b"SaexBootstrapOther\0")
        self.assertEqual(len(changed), len(DLL))
        with self.assertRaisesRegex(ValueError, "unexpected_exports"): checker.audit(changed, MAP)

    def test_dynamic_initialization_refused(self):
        with self.assertRaisesRegex(ValueError, "dynamic_initializer_bucket"):
            checker.audit(DLL, MAP + "\n .CRT$XCU DATA\n")
        with self.assertRaisesRegex(ValueError, "dynamic_initializer_symbol"):
            checker.audit(DLL, MAP + "\n ??__Eunsafe@saex\n")

    def test_vendor_object_refused(self):
        with self.assertRaisesRegex(ValueError, "vendor_object"):
            checker.audit(DLL, MAP + "\n sdk:PluginBase.obj\n")

    def test_missing_map_refused(self):
        with self.assertRaises(ValueError): checker.audit(DLL, "")

    def test_non_null_crt_sentinel_refused(self):
        section, at = re.search(r"([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})\s+00000004H\s+\.CRT\$XCA", MAP).groups()
        data = bytearray(DLL); pe = struct.unpack_from("<I", data, 60)[0]
        table = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
        raw = struct.unpack_from("<I", data, table + (int(section, 16) - 1) * 40 + 20)[0]
        data[raw + int(at, 16)] = 1
        with self.assertRaisesRegex(ValueError, "crt_sentinel_not_null"): checker.audit(data, MAP)

    def test_truncated_artifact_refused(self):
        for end in [0, 63, 128, len(DLL) // 2]:
            with self.subTest(end=end), self.assertRaises(ValueError): checker.audit(DLL[:end], MAP)

    def test_non_executable_export(self):
        data=bytearray(DLL); _,table,_=self.layout(data)
        flags=struct.unpack_from('<I',data,table+36)[0]
        struct.pack_into('<I',data,table+36,flags & ~0x20000000)
        with self.assertRaisesRegex(ValueError,'export_not_executable'): checker.audit(data,MAP)

    def test_overlapping_and_forwarded_exports(self):
        opt,_,offset=self.layout(DLL); export=struct.unpack_from('<I',DLL,opt+96)[0]
        functions=struct.unpack_from('<I',DLL,offset(export)+28)[0]; at=offset(functions)
        for target,reason in [(struct.unpack_from('<I',DLL,at)[0],'export_target_overlap'),(export,'forwarded_export')]:
            data=bytearray(DLL); struct.pack_into('<I',data,at+4,target)
            with self.assertRaisesRegex(ValueError,reason): checker.audit(data,MAP)

    def test_partial_prefix_relocation(self):
        data=bytearray(DLL); opt,_,offset=self.layout(data)
        reloc=struct.unpack_from('<I',data,opt+96+5*8)[0]; at=offset(reloc)
        target=checker.audit(DLL,MAP)['exportEntries']['SaexBootstrapQuery']['rva']
        struct.pack_into('<I',data,at,target & ~4095)
        struct.pack_into('<H',data,at+8,0x3000 | ((target+17) & 4095))
        with self.assertRaisesRegex(ValueError,'export_prefix_partial_relocation'): checker.audit(data,MAP)

    def test_prefix_relocation_metadata(self):
        result=checker.audit(DLL,MAP)
        for entry in result['exportEntries'].values():
            self.assertTrue(all(0<=n<=16 for n in entry['highlowOffsets']))
            self.assertEqual(len(entry['highlowOffsets']),len(set(entry['highlowOffsets'])))
        self.assertEqual(result['preferredImageBase']%65536,0)

    def test_duplicate_and_unsupported_relocations(self):
        opt,_,offset=self.layout(DLL); reloc=struct.unpack_from('<I',DLL,opt+96+5*8)[0]; at=offset(reloc)
        first=struct.unpack_from('<H',DLL,at+8)[0]
        for delta,value,reason in [(10,first,'relocation_target_overlap'),(8,0x5000 | (first & 4095),'unsupported_relocation')]:
            data=bytearray(DLL); struct.pack_into('<H',data,at+delta,value)
            with self.assertRaisesRegex(ValueError,reason): checker.audit(data,MAP)

    def test_malformed_relocation_block(self):
        opt,_,offset=self.layout(DLL); reloc=struct.unpack_from('<I',DLL,opt+96+5*8)[0]
        for size in [0,7,0xffffffff]:
            data=bytearray(DLL); struct.pack_into('<I',data,offset(reloc)+4,size)
            with self.assertRaisesRegex(ValueError,'relocation_block'): checker.audit(data,MAP)


if __name__ == "__main__": unittest.main()
