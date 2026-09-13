"""Shared, non-executed PE32 bytes for CLI rejection tests."""
import struct


def fixture():
    b = bytearray(1024)
    def u16(at, value): struct.pack_into("<H", b, at, value)
    def u32(at, value): struct.pack_into("<I", b, at, value)
    u16(0, 0x5a4d); u32(60, 64); u32(64, 0x4550); u16(68, 0x14c); u16(70, 1)
    u16(84, 224); u16(86, 0x102); u16(88, 0x10b); u32(104, 4096); u32(116, 0x400000)
    u32(120, 4096); u32(124, 512); u32(144, 8192); u32(148, 512); u32(180, 16)
    b[312:314] = b".x"; u32(320, 512); u32(324, 4096); u32(328, 512); u32(332, 512); u32(348, 0x60000020)
    # Same upstream version marker is insufficient: full hash remains unknown.
    b[512:517] = bytes.fromhex("e97b191601")
    return b
