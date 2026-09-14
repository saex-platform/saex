"""Negative coverage for the nonexecuting image experiment, without requiring GTA."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/native'))
import image_protection_probe as tool


def fixture():
    data = bytearray(1024)
    data[:2] = b'MZ'
    for at, value, kind in ((60, 128, 'I'), (128, 0x4550, 'I'), (132, 0x14c, 'H'),
                            (134, 1, 'H'), (148, 224, 'H'), (150, 2, 'H'),
                            (152, 0x10b, 'H'), (184, 4096, 'I'), (208, 8192, 'I'),
                            (212, 512, 'I')):
        struct.pack_into('<' + kind, data, at, value)
    return bytes(data)


def region(base=0x400000, size=8192, protect=2):
    return dict(base=base, allocation=0x400000, bytes=size, state=tool.MEM_COMMIT,
                type=tool.MEM_IMAGE, protect=protect)


class FakeImage:
    def __init__(self):
        self.phase = 0
        self.cleanup = {}
        self.closed = False
        self.read_count = 0
        self.copy = True
        self.change_input = False
        self.bad_old = False
        self.bad_transition = False
        self.cleanup_failure = False
        self.query_failure = False

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.closed = True
        if self.cleanup_failure:
            raise RuntimeError('cleanup failed')
        self.cleanup = dict(viewUnmapped=True, mappingClosed=True, fileClosed=True)

    def read(self):
        self.read_count += 1
        data = fixture()
        return data[:-1] + b'X' if self.change_input and self.read_count == 2 else data

    def map(self):
        return 0x400000

    def protect(self, size):
        if size != 8192:
            raise AssertionError('incorrect image extent')
        self.phase = 1
        return 0x20 if self.bad_old else 2

    def query(self, address):
        if self.query_failure:
            raise ValueError('query failed')
        protection = 2 if not self.phase else tool.WRITE_COPY if self.copy else tool.READ_WRITE
        if self.phase == 2 and not self.bad_transition:
            protection = tool.READ_WRITE
        return region(protect=protection)

    def touch_copy(self, address):
        if address != 0x400000:
            raise AssertionError('unexpected write address')
        self.phase = 2


class LayoutTests(unittest.TestCase):
    def test_valid_minimal_consumed_layout(self):
        self.assertEqual(tool.fixture_layout(fixture()), 8192)

    def test_truncation_and_dos_bounds(self):
        for data in (b'', b'MZ', fixture()[:350], b'x' * 1024):
            with self.subTest(length=len(data)), self.assertRaises(ValueError):
                tool.fixture_layout(data)
        data = bytearray(fixture())
        struct.pack_into('<I', data, 60, 0xffffffff)
        with self.assertRaisesRegex(ValueError, 'bounds'):
            tool.fixture_layout(data)

    def test_dll_x64_and_oversized_image_rejected(self):
        cases = ((132, 0x8664, 'H'), (150, 0x2002, 'H'), (152, 0x20b, 'H'),
                 (148, 64, 'H'), (134, 97, 'H'), (184, 512, 'I'),
                 (208, 4097, 'I'), (208, tool.MAX_IMAGE + 4096, 'I'), (212, 128, 'I'))
        for at, value, kind in cases:
            with self.subTest(at=at, value=value):
                data = bytearray(fixture())
                struct.pack_into('<' + kind, data, at, value)
                with self.assertRaises(ValueError):
                    tool.fixture_layout(data)


class ScanTests(unittest.TestCase):
    def scan(self, row, *, writable=True):
        return tool.scan_regions(lambda _: row, 0x400000, 8192, writable=writable)

    def test_copy_and_private_regions_cover_whole_image(self):
        rows = [region(size=4096, protect=0x80), region(base=0x401000, size=4096, protect=0x40)]
        result = tool.scan_regions(lambda address: rows[(address - 0x400000) // 4096], 0x400000, 8192, writable=True)
        self.assertEqual(result, [dict(rva=0, bytes=4096, protect=0x80), dict(rva=4096, bytes=4096, protect=0x40)])

    def test_read_execute_only_and_modifiers_rejected_after_change(self):
        for value in (0, 1, 2, 4, 8, 0x10, 0x20, 0xc0, 0x140, 0x180, 0x240, 0x480, 0x40000040):
            with self.subTest(protect=value), self.assertRaises(ValueError):
                self.scan(region(protect=value))

    def test_wrong_allocation_state_and_type_rejected(self):
        for key, value in (('allocation', 0x500000), ('state', 0x2000), ('type', 0x20000)):
            row = region(protect=0x80)
            row[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.scan(row)

    def test_gap_zero_overrun_and_nonprogress_rejected(self):
        for key, value in (('base', 0x400001), ('base', 0x3fffff), ('bytes', 0), ('bytes', -1), ('bytes', 8193)):
            row = region(protect=0x80)
            row[key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                self.scan(row)
        # Reusing the first region for the next address must not loop indefinitely.
        with self.assertRaisesRegex(ValueError, 'bounds'):
            self.scan(region(size=4096, protect=0x80))

    def test_region_budget(self):
        def query(address):
            return region(base=address, size=4096, protect=0x80)
        with self.assertRaisesRegex(ValueError, 'region_limit'):
            tool.scan_regions(query, 0x400000, 129 * 4096, writable=True)

    def test_image_budget_checked_before_query(self):
        for size in (0, 4095, tool.MAX_IMAGE + 4096):
            with self.subTest(size=size), self.assertRaisesRegex(ValueError, 'scan_bounds'):
                tool.scan_regions(lambda _: self.fail('unexpected query'), 0x400000, size)


class ObservationTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.image = FakeImage()

    def observe(self):
        def factory(path):
            self.assertEqual(path, self.root / 'out/windows-x86/Debug/saex_startup_return_fixture_normal.exe')
            return self.image
        return tool.observe('Debug', root=self.root, image_factory=factory)

    def test_copy_transition_keeps_runtime_capability_closed(self):
        result = self.observe()
        self.assertTrue(result['verified'])
        self.assertTrue(result['inputUnchanged'])
        self.assertTrue(result['privateWrite']['performed'])
        self.assertEqual(result['afterChange'][0]['protect'], 0x80)
        self.assertEqual(result['afterPrivateWrite'][0]['protect'], 0x40)
        self.assertFalse(result['runtimeEligible'])
        self.assertFalse(result['canAttach'])
        self.assertTrue(all(result['cleanup'].values()))

    def test_already_private_does_not_claim_copy_transition(self):
        self.image.copy = False
        result = self.observe()
        self.assertTrue(result['verified'])
        self.assertFalse(result['privateWrite']['performed'])
        self.assertIsNone(result['afterPrivateWrite'])

    def test_incorrect_old_protection_fails_and_closes(self):
        self.image.bad_old = True
        with self.assertRaisesRegex(ValueError, 'old_protection'):
            self.observe()
        self.assertTrue(self.image.closed)

    def test_missing_copy_transition_fails_and_closes(self):
        self.image.bad_transition = True
        with self.assertRaisesRegex(ValueError, 'transition_missing'):
            self.observe()
        self.assertTrue(self.image.closed)

    def test_input_change_cannot_be_success(self):
        self.image.change_input = True
        with self.assertRaisesRegex(ValueError, 'input_changed'):
            self.observe()
        self.assertTrue(self.image.closed)

    def test_query_and_cleanup_failures_cannot_be_success(self):
        self.image.query_failure = True
        with self.assertRaisesRegex(ValueError, 'query failed'):
            self.observe()
        self.assertTrue(self.image.closed)
        self.image.query_failure = False
        self.image.cleanup_failure = True
        with self.assertRaisesRegex(RuntimeError, 'cleanup failed'):
            self.observe()

    def test_caller_cannot_supply_executable_or_pid(self):
        for value in ('../../game', 'gta_sa.exe', '1234', 'debug'):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'configuration'):
                tool.observe(value, root=self.root, image_factory=lambda _: self.fail('unexpected open'))

    def test_windows_cleanup_attempts_all_releases(self):
        image = object.__new__(tool.WindowsImage)
        image.base, image.mapping, image.file = 1, 2, 3
        image.cleanup = {}
        released = []
        image.unmap = lambda value: released.append(value) or False
        image.close_handle = lambda value: released.append(value) or True
        with self.assertRaisesRegex(RuntimeError, 'viewUnmapped'):
            image.__exit__(None, None, None)
        self.assertEqual(released, [1, 2, 3])
        self.assertTrue(image.cleanup['mappingClosed'])
        self.assertTrue(image.cleanup['fileClosed'])


if __name__ == '__main__':
    unittest.main()
