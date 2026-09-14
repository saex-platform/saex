"""Bounded PE/import/export/TLS and linker-map audit of our x86 bootstrap artifact.

Not a transitive CRT audit, signature verification, or proof of runtime behavior.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

EXPORTS = {"SaexBootstrapInitialize", "SaexBootstrapQuery", "SaexBootstrapStop"}
IMPORTS = {"kernel32.dll", "bcrypt.dll", "msvcp140d.dll", "msvcp140.dll",
           "vcruntime140d.dll", "vcruntime140.dll", "ucrtbased.dll", "ucrtbase.dll",
           "api-ms-win-crt-runtime-l1-1-0.dll", "api-ms-win-crt-heap-l1-1-0.dll",
           "api-ms-win-crt-string-l1-1-0.dll", "api-ms-win-crt-stdio-l1-1-0.dll",
           "api-ms-win-crt-convert-l1-1-0.dll"}


def audit(data, link_map):
    def require(ok, why):
        if not ok: raise ValueError(why)
    def number(at, fmt="I"):
        size = struct.calcsize("<" + fmt)
        require(0 <= at <= len(data) - size, "pe_bounds")
        return struct.unpack_from("<" + fmt, data, at)[0]
    require(64 <= len(data) <= 16 * 1024 * 1024 and data[:2] == b"MZ", "artifact_size_or_mz")
    pe = number(60)
    require(number(pe) == 0x4550 and number(pe + 4, "H") == 0x14c, "artifact_must_be_x86")
    require(number(pe + 22, "H") & 0x2000, "artifact_must_be_dll")
    count = number(pe + 6, "H"); optional = pe + 24
    require(1 <= count <= 96 and number(optional, "H") == 0x10b, "artifact_layout")
    require(number(pe + 20, "H") >= 224 and number(optional + 92) == 16, "artifact_directories")
    table = optional + number(pe + 20, "H")
    sections = []
    executable_sections = []
    for i in range(count):
        p = table + i * 40
        size, rva, raw_size, raw = (number(p + off) for off in [8, 12, 16, 20])
        require(raw + raw_size <= len(data), "section_bounds")
        sections.append((rva, raw_size, raw))
        if number(p + 36) & 0x20000000: executable_sections.append((rva, raw_size))
    def offset(rva, length):
        matches = [raw + rva - start for start, size, raw in sections
                   if start <= rva and rva + length <= start + size]
        require(length > 0 and len(matches) == 1, "rva_not_unique_file_backed")
        return matches[0]
    def rnum(rva, fmt="I"):
        return number(offset(rva, struct.calcsize("<" + fmt)), fmt)
    def string(rva):
        result = bytearray()
        for i in range(128):
            byte = data[offset(rva + i, 1)]
            if byte == 0: return result.decode("ascii")
            result.append(byte)
        raise ValueError("artifact_string_limit")
    def directory(index):
        return number(optional + 96 + index * 8), number(optional + 100 + index * 8)
    for i, why in [(9, "tls_directory_not_allowed"), (13, "delay_import_not_allowed"), (14, "clr_not_allowed")]:
        require(directory(i) == (0, 0), why)
    imports_rva, imports_size = directory(1)
    require(20 <= imports_size <= 20 * 33, "import_directory_size")
    imports = []
    terminated = False
    for at in range(0, imports_size - 19, 20):
        fields = [rnum(imports_rva + at + n * 4) for n in range(5)]
        if not any(fields): terminated = True; break
        name = string(fields[3]).lower()
        require(name in IMPORTS and name not in imports, "unexpected_import: " + name)
        imports.append(name)
    require(terminated and {"kernel32.dll", "bcrypt.dll"} <= set(imports), "import_set_or_termination")
    exports_rva, exports_size = directory(0)
    require(40 <= exports_size <= 4096, "export_directory_size")
    require(rnum(exports_rva + 20) == 3 and rnum(exports_rva + 24) == 3, "export_count")
    functions, names, ordinals = (rnum(exports_rva + n) for n in [28, 32, 36])
    exports = []
    export_entries = {}
    for i in range(3):
        exports.append(string(rnum(names + 4 * i)))
        ordinal = rnum(ordinals + 2 * i, "H")
        require(ordinal < 3, "export_ordinal")
        target = rnum(functions + 4 * ordinal)
        require(not exports_rva <= target < exports_rva + exports_size, "forwarded_export")
        require(any(start <= target and target + 20 <= start + size for start, size in executable_sections),
                "export_not_executable")
        require(all(abs(target - item["rva"]) >= 20 for item in export_entries.values()), "export_target_overlap")
        at = offset(target, 20)
        export_entries[exports[-1]] = {"rva": target, "prefixHex": data[at:at+20].hex(), "highlowOffsets": []}
    require(set(exports) == EXPORTS, "unexpected_exports")
    # Only exact PE32 HIGHLOW fixups wholly inside a prefix can be normalized.
    reloc_rva, reloc_size = directory(5)
    require(0 < reloc_size <= 1024 * 1024, "relocation_directory_size")
    consumed = 0
    relocation_targets = []
    while consumed < reloc_size:
        require(reloc_size - consumed >= 8, "relocation_header")
        page, size = rnum(reloc_rva + consumed), rnum(reloc_rva + consumed + 4)
        require(size >= 8 and size % 4 == 0 and size <= reloc_size - consumed and page % 4096 == 0, "relocation_block")
        for i in range(8, size, 2):
            value = rnum(reloc_rva + consumed + i, "H")
            kind, target = value >> 12, page + (value & 4095)
            if kind == 0: continue
            require(kind == 3, "unsupported_relocation")
            require(target <= number(optional + 56) - 4, "relocation_target")
            relocation_targets.append(target)
            for entry in export_entries.values():
                if target + 4 <= entry['rva'] or target >= entry['rva'] + 20: continue
                require(entry['rva'] <= target and target + 4 <= entry['rva'] + 20, 'export_prefix_partial_relocation')
                entry['highlowOffsets'].append(target - entry['rva'])
        consumed += size
    ordered = sorted(relocation_targets)
    require(all(b >= a + 4 for a,b in zip(ordered,ordered[1:])), 'relocation_target_overlap')
    preferred_base = number(optional + 28)
    require(preferred_base > 0 and preferred_base % 65536 == 0, 'preferred_image_base')
    for entry in export_entries.values(): entry['highlowOffsets'].sort()
    require(0 < len(link_map) <= 2 * 1024 * 1024, "map_size")
    require("_DllMain@12" in link_map and "bootstrap_module.obj" in link_map and "bootstrap_session.obj" in link_map,
            "map_missing_bootstrap_objects")
    # Reject user dynamic initializer buckets. CRT sentinels/RTC helpers are inventoried, not hidden.
    buckets = set(re.findall(r"\.CRT\$[A-Za-z0-9_]+", link_map))
    sentinels = {".CRT$XCA", ".CRT$XCZ", ".CRT$XIA", ".CRT$XIZ", ".CRT$XPA", ".CRT$XPZ", ".CRT$XTA", ".CRT$XTZ"}
    require(buckets == sentinels, "dynamic_initializer_bucket")
    records = re.findall(r"^\s*([0-9A-Fa-f]{4}):([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})H\s+(\.CRT\$\w+)\s+DATA", link_map, re.M)
    require(len(records) == len(sentinels), "crt_map_records")
    for section, at, length, name in records:
        index = int(section, 16) - 1
        # Release linker may append four bytes of null alignment padding to XTZ.
        size = int(length, 16)
        require(0 <= index < len(sections) and (size == 4 or (name == ".CRT$XTZ" and size == 8)) and name in sentinels, "crt_map_range")
        for delta in range(0, size, 4):
            require(rnum(sections[index][0] + int(at, 16) + delta) == 0, "crt_sentinel_not_null")
    require("??__E" not in link_map and "??__F" not in link_map, "dynamic_initializer_symbol")
    require(not re.search(r"plugin[_-]?sdk|PluginBase\.obj|safetyhook|injector", link_map, re.I), "vendor_object_in_bootstrap")
    return {"scope": "bootstrap-artifact-audit", "sha256": hashlib.sha256(data).hexdigest(),
            "mapSha256": hashlib.sha256(link_map.encode("utf-8")).hexdigest(),
            "imports": sorted(imports), "exports": sorted(exports), "exportEntries": export_entries,
            "preferredImageBase": preferred_base, "tlsDirectory": False,
            "crtBuckets": sorted(buckets), "canAttach": False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dll", type=Path)
    parser.add_argument("map", type=Path)
    parser.add_argument("--record", type=Path)
    args = parser.parse_args()
    if args.dll.stat().st_size > 16 * 1024 * 1024 or args.map.stat().st_size > 2 * 1024 * 1024:
        raise ValueError("artifact_input_limit")
    result = audit(args.dll.read_bytes(), args.map.read_bytes().decode("utf-8"))
    output = json.dumps(result, ensure_ascii=True, sort_keys=True)
    if args.record:
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_text(output + "\n", encoding="utf-8")
    print(output)


if __name__ == "__main__": main()
