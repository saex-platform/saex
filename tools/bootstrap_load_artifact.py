"""Bind this build's audited bootstrap DLL to its observer. Never reads installed GTA."""
import argparse
from pathlib import Path
import check_bootstrap
import loader_policy


def compile_artifact(data, link_map):
    result = check_bootstrap.audit(data, link_map)
    entries = []
    for name in ['SaexBootstrapInitialize', 'SaexBootstrapQuery', 'SaexBootstrapStop']:
        entry = result['exportEntries'][name]
        prefix = ', '.join(f'std::byte{{0x{entry["prefixHex"][i:i+2]}}}' for i in range(0,40,2))
        mask = sum(1 << offset for offset in entry['highlowOffsets'])
        entries.append(f'    BootstrapExportSpec{{{entry["rva"]}U, {{{prefix}}}, {result["preferredImageBase"]}U, {mask}U}},')
    return '\n'.join(['// Generated from this build only. Not a publisher signature.', '#pragma once',
        '#include "saex/engine/loader_policy.hpp"', 'namespace saex::engine {',
        f'inline constexpr std::string_view bootstrap_artifact_digest = "{result["sha256"]}";',
        f'inline constexpr std::string_view bootstrap_artifact_map_digest = "{result["mapSha256"]}";',
        'inline constexpr LoaderPinSpec bootstrap_artifact_pin{LoaderOrigin::game_root, "saex_bootstrap.asi",',
        f'    {len(data)}ULL, {loader_policy.byte_array(result["sha256"])}}};',
        'inline constexpr std::array<BootstrapExportSpec, 3> bootstrap_artifact_exports{{',
        *entries, '}};', '}', ''])


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('dll', type=Path); parser.add_argument('map', type=Path)
    parser.add_argument('output', type=Path); args = parser.parse_args()
    if args.dll.stat().st_size > 16*1024*1024 or args.map.stat().st_size > 2*1024*1024: raise ValueError('artifact_input_limit')
    output = compile_artifact(args.dll.read_bytes(), args.map.read_bytes().decode('utf-8'))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.is_file() or args.output.read_text(encoding='utf-8') != output:
        args.output.write_text(output, encoding='utf-8', newline='\n')
    print('Audited bootstrap artifact bound to observer.')


if __name__ == '__main__': main()
