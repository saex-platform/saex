"""Measure Windows image copy-on-write on a built SAEX fixture without executing it.

This opt-in experiment opens only out/windows-x86/<configuration>/
saex_startup_return_fixture_normal.exe. It never starts GTA or loads DLL code.
"""
from __future__ import annotations

import argparse
import ctypes as ct
from ctypes import wintypes as wt
import hashlib
import json
from pathlib import Path
import struct
import sys

from dependency import safe_path

ROOT = Path(__file__).resolve().parents[2]
MAX_FILE = 16 * 1024 * 1024
MAX_IMAGE = 16 * 1024 * 1024
MAX_REGIONS = 128
MEM_IMAGE = 0x1000000
MEM_COMMIT = 0x1000
READ_WRITE = 0x40
WRITE_COPY = 0x80


def fixture_layout(data: bytes) -> int:
    """Validate the bounded fields used here, not every PE directory or signature."""
    def number(offset, kind='I'):
        length = struct.calcsize('<' + kind)
        if not 0 <= offset <= len(data) - length:
            raise ValueError('fixture_header_bounds')
        return struct.unpack_from('<' + kind, data, offset)[0]

    if not 64 <= len(data) <= MAX_FILE or data[:2] != b'MZ':
        raise ValueError('fixture_size_or_mz')
    pe = number(60)
    if pe < 64 or number(pe) != 0x4550 or number(pe + 4, 'H') != 0x14c:
        raise ValueError('fixture_pe32_x86_required')
    flags = number(pe + 22, 'H')
    if not flags & 2 or flags & 0x2000:
        raise ValueError('fixture_executable_required')
    optional = pe + 24
    optional_size = number(pe + 20, 'H')
    sections = number(pe + 6, 'H')
    if (optional_size < 224 or number(optional, 'H') != 0x10b or
            not 1 <= sections <= 96 or optional + optional_size + sections * 40 > len(data)):
        raise ValueError('fixture_optional_header')
    size = number(optional + 56)
    headers = number(optional + 60)
    if (number(optional + 32) != 4096 or not 4096 <= size <= MAX_IMAGE or size % 4096 or
            not optional + optional_size + sections * 40 <= headers <= min(len(data), size)):
        raise ValueError('fixture_image_bounds')
    return size


def scan_regions(query, base: int, size: int, *, writable=False) -> list[dict]:
    """Require full, gap-free coverage of this mapping; never mask protection flags."""
    if not base or not 4096 <= size <= MAX_IMAGE or size % 4096:
        raise ValueError('image_scan_bounds')
    cursor = base
    end = base + size
    result = []
    while cursor < end:
        if len(result) == MAX_REGIONS:
            raise ValueError('image_region_limit')
        region = query(cursor)
        start = region['base']
        length = region['bytes']
        protection = region['protect']
        if (region['allocation'] != base or region['state'] != MEM_COMMIT or
                region['type'] != MEM_IMAGE or protection not in (2, 4, 8, 0x10, 0x20, READ_WRITE, WRITE_COPY)):
            raise ValueError('image_region_identity_or_protection')
        if writable and protection not in (READ_WRITE, WRITE_COPY):
            raise ValueError('image_not_execute_writable')
        if start < base or start > cursor or length <= 0 or start + length <= cursor or start + length > end:
            raise ValueError('image_region_bounds')
        next_cursor = start + length
        result.append({'rva': cursor - base, 'bytes': next_cursor - cursor, 'protect': protection})
        cursor = next_cursor
    return result


class MemoryInformation(ct.Structure):
    _fields_ = [('BaseAddress', ct.c_void_p), ('AllocationBase', ct.c_void_p),
                ('AllocationProtect', wt.DWORD), ('RegionSize', ct.c_size_t),
                ('State', wt.DWORD), ('Protect', wt.DWORD), ('Type', wt.DWORD)]


class WindowsImage:
    def __init__(self, path: Path):
        if sys.platform != 'win32':
            raise ValueError('windows_required')
        self.path = path
        self.file = self.mapping = self.base = None
        self.cleanup = {'viewUnmapped': False, 'mappingClosed': False, 'fileClosed': False}
        self.kernel = ct.WinDLL('kernel32', use_last_error=True)
        self.create_file = self.api('CreateFileW', [wt.LPCWSTR, wt.DWORD, wt.DWORD, ct.c_void_p, wt.DWORD, wt.DWORD, wt.HANDLE], wt.HANDLE)
        self.file_size = self.api('GetFileSizeEx', [wt.HANDLE, ct.POINTER(ct.c_longlong)], wt.BOOL)
        self.seek = self.api('SetFilePointerEx', [wt.HANDLE, ct.c_longlong, ct.c_void_p, wt.DWORD], wt.BOOL)
        self.read_file = self.api('ReadFile', [wt.HANDLE, ct.c_void_p, wt.DWORD, ct.POINTER(wt.DWORD), ct.c_void_p], wt.BOOL)
        self.create_mapping = self.api('CreateFileMappingW', [wt.HANDLE, ct.c_void_p, wt.DWORD, wt.DWORD, wt.DWORD, wt.LPCWSTR], wt.HANDLE)
        self.map_view = self.api('MapViewOfFile', [wt.HANDLE, wt.DWORD, wt.DWORD, wt.DWORD, ct.c_size_t], ct.c_void_p)
        self.virtual_query = self.api('VirtualQuery', [ct.c_void_p, ct.POINTER(MemoryInformation), ct.c_size_t], ct.c_size_t)
        self.virtual_protect = self.api('VirtualProtect', [ct.c_void_p, ct.c_size_t, wt.DWORD, ct.POINTER(wt.DWORD)], wt.BOOL)
        self.unmap = self.api('UnmapViewOfFile', [ct.c_void_p], wt.BOOL)
        self.close_handle = self.api('CloseHandle', [wt.HANDLE], wt.BOOL)

    def api(self, name, args, result):
        function = getattr(self.kernel, name)
        function.argtypes, function.restype = args, result
        return function

    @staticmethod
    def checked(value):
        if not value or value == ct.c_void_p(-1).value:
            raise ct.WinError(ct.get_last_error())
        return value

    def __enter__(self):
        # Retain a read-only handle that denies concurrent writes and deletion.
        self.file = self.checked(self.create_file(str(self.path), 0x80000000, 1, None, 3, 0x80, None))
        return self

    def read(self):
        size = ct.c_longlong()
        self.checked(self.file_size(self.file, ct.byref(size)))
        if not 64 <= size.value <= MAX_FILE:
            raise ValueError('fixture_file_size')
        self.checked(self.seek(self.file, 0, None, 0))
        buffer = ct.create_string_buffer(size.value)
        read = wt.DWORD()
        self.checked(self.read_file(self.file, buffer, size.value, ct.byref(read), None))
        if read.value != size.value:
            raise ValueError('fixture_short_read')
        return buffer.raw

    def map(self):
        self.mapping = self.checked(self.create_mapping(self.file, None, MEM_IMAGE | 2, 0, 0, None))
        self.base = self.checked(self.map_view(self.mapping, 4, 0, 0, 0))
        return self.base

    def query(self, address):
        info = MemoryInformation()
        if self.virtual_query(address, ct.byref(info), ct.sizeof(info)) != ct.sizeof(info):
            raise ValueError('image_query_failed')
        return dict(base=info.BaseAddress, allocation=info.AllocationBase, bytes=info.RegionSize,
                    state=info.State, type=info.Type, protect=info.Protect)

    def protect(self, size):
        old = wt.DWORD()
        self.checked(self.virtual_protect(self.base, size, READ_WRITE, ct.byref(old)))
        return old.value

    def touch_copy(self, address):
        # No instruction is called. Rewrite one byte to itself only in our own view.
        byte = ct.c_ubyte.from_address(address)
        original = byte.value
        byte.value = original
        if byte.value != original:
            raise ValueError('private_byte_changed')

    def __exit__(self, *_):
        failures = []
        for value, function, key in ((self.base, self.unmap, 'viewUnmapped'),
                                     (self.mapping, self.close_handle, 'mappingClosed'),
                                     (self.file, self.close_handle, 'fileClosed')):
            if value:
                self.cleanup[key] = bool(function(value))
                if not self.cleanup[key]:
                    failures.append(key)
        if failures:
            raise RuntimeError('image_cleanup_failed: ' + ','.join(failures))


def observe(configuration: str, *, root=ROOT, image_factory=WindowsImage) -> dict:
    if configuration not in ('Debug', 'Release'):
        raise ValueError('fixture_configuration')
    name = f'out/windows-x86/{configuration}/saex_startup_return_fixture_normal.exe'
    path = safe_path(root, name)
    with image_factory(path) as image:
        data = image.read()
        size = fixture_layout(data)
        digest = hashlib.sha256(data).hexdigest()
        base = image.map()
        before = scan_regions(image.query, base, size)
        old = image.protect(size)
        if old != before[0]['protect']:
            raise ValueError('old_protection_mismatch')
        after = scan_regions(image.query, base, size, writable=True)
        copy = next((row for row in after if row['protect'] == WRITE_COPY), None)
        private_write = {'performed': False, 'rva': None, 'bytePreserved': False}
        after_write = None
        if copy is not None:
            image.touch_copy(base + copy['rva'])
            after_write = scan_regions(image.query, base, size, writable=True)
            changed = next(row for row in after_write if row['rva'] <= copy['rva'] < row['rva'] + row['bytes'])
            if changed['protect'] != READ_WRITE:
                raise ValueError('copy_on_write_transition_missing')
            private_write = {'performed': True, 'rva': copy['rva'], 'bytePreserved': True}
        if image.read() != data:
            raise ValueError('fixture_input_changed')
    # Reaching here requires successful unmap and handle cleanup, too.
    return dict(schemaVersion=1, scope='owned-fixture-image-protection', verified=True,
                runtimeEligible=False, canAttach=False, configuration=configuration,
                fixture=name, fixtureSha256=digest, fixtureBytes=len(data), imageBytes=size,
                requestedProtection=READ_WRITE, oldProtection=old, beforeChange=before,
                afterChange=after, afterPrivateWrite=after_write, privateWrite=private_write,
                inputUnchanged=True, cleanup=image.cleanup)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=['Debug', 'Release'], default='Debug')
    args = parser.parse_args(argv)
    try:
        result = observe(args.configuration)
        version = sys.getwindowsversion()
        result['windowsVersion'] = dict(major=version.major, minor=version.minor, build=version.build)
    except (OSError, ValueError, RuntimeError) as error:
        print(json.dumps(dict(scope='owned-fixture-image-protection', verified=False,
                              runtimeEligible=False, canAttach=False, reason=str(error))))
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
