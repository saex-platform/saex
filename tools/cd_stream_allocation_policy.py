"""Compile the normal HeapAlloc path; SBH and new-handler branches stay closed."""
import argparse,hashlib,json,re,struct
from pathlib import Path
import cd_stream_disk_policy as parent
import loader_policy
from frame_target_policy import byte_array
ROOT=Path(__file__).resolve().parents[1]
SOURCE=Path('contracts/engine/cd-stream-allocation-policy.json')
OUTPUT=Path('include/saex/engine/cd_stream_allocation_policy.generated.hpp')
SOURCES=(SOURCE,*parent.SOURCES)
FIELDS=('mallocRva','nhRva','heapRva','scopeRva','newModeRva','thresholdRva','heapHandleRva','heapModeRva','iatRva','scopeCleanupRva')
TEMPLATES={
    'aligned':('allocatorRva',0,'8b442404568b74240c03c650e8000000008bc88d043183c4044ef7d623c68948fc5ec3',[(13,'mallocRva',True)]),
    'malloc':('mallocRva',0,'ff3500000000ff742408e8000000005959c3',[(2,'newModeRva',False),(11,'nhRva',True)]),
    'nh':('nhRva',0,'837c2404e07722ff742404e80000000085c0597516',[(12,'heapRva',True)]),
    'heapEntry':('heapRva',0,'6a0c6800000000e8000000008b7508833d0000000003752e3b35000000007726',[(3,'scopeRva',False),(8,'prologueRva',True),(17,'heapModeRva',False),(26,'thresholdRva',False)]),
    'heapTail':('heapRva',70,'85f6750146833d0000000001740683c60f83e6f0566a00ff3500000000ff1500000000e800000000c3',[(7,'heapModeRva',False),(25,'heapHandleRva',False),(31,'iatRva',False),(36,'epilogueRva',True)])
}
def compile_source(data,*parents):
    if max(map(len,(data,*parents)))>65536:raise ValueError('allocation_size')
    parent.compile_source(*parents);p=json.loads(data,object_pairs_hook=loader_policy.strict_object)
    keys={'schemaVersion','id','stage','diskPolicySha256','reviewDocument','function','systemModule','heapExport','eventLimit',*FIELDS,*(x+'Hex' for x in TEMPLATES)}
    if type(p) is not dict or set(p)!=keys:raise ValueError('allocation_keys')
    if type(p['schemaVersion']) is not int or p['schemaVersion']!=1 or not isinstance(p['id'],str) or not re.fullmatch('[a-z0-9.-]{1,96}',p['id']) or p['stage']!='after-aligned-allocation':raise ValueError('allocation_identity')
    if p['diskPolicySha256']!=hashlib.sha256(parents[0]).hexdigest():raise ValueError('allocation_parent')
    if p['reviewDocument']!='docs/development/d1-cd-stream-allocation.md' or p['heapExport']!='NTDLL.RtlAllocateHeap' or type(p['eventLimit']) is not int or p['eventLimit']!=160:raise ValueError('allocation_review')
    def src(name):return json.loads(parents[parent.SOURCES.index(Path(name))])
    layout=src('contracts/engine/observed-profile.json')['layout'];base=layout['imageBase'];size=layout['imageSize']
    disk=json.loads(parents[0]);seh=src('contracts/engine/cwd-seh-policy.json');ready=src('contracts/engine/file-manager-ready-policy.json')
    values={**{k:p[k] for k in FIELDS},'allocatorRva':disk['allocatorRva'],'prologueRva':seh['prologue']['targetRva'],'epilogueRva':ready['epilogueRva']}
    if any(type(v) is not int or not 4096<=v<size for v in values.values()) or any(p[k]%4 for k in ('scopeRva','newModeRva','thresholdRva','heapHandleRva','heapModeRva','iatRva')):raise ValueError('allocation_range')
    spans=[(values['allocatorRva'],35,0x20000000),(p['mallocRva'],18,0x20000000),(p['nhRva'],44,0x20000000),(p['heapRva'],111,0x20000000),(p['scopeRva'],12,0x40000000),(p['scopeCleanupRva'],1,0x20000000),(p['iatRva'],4,0x40000000)]
    spans += [(p[k],4,0xc0000000) for k in ('newModeRva','thresholdRva','heapHandleRva','heapModeRva')]
    for i,(r,length,mask) in enumerate(spans):
        if not any(s['rva']<=r and r+length<=s['rva']+(s['virtualSize'] if mask==0xc0000000 else s['rawSize']) and s['characteristics']&mask==mask for s in layout['sections']):raise ValueError('allocation_section')
        if any(r<a+b and a<r+length for a,b,_ in spans[:i]):raise ValueError('allocation_overlap')
    for name,(field,delta,hx,patches) in TEMPLATES.items():
        body=bytearray.fromhex(hx);start=values[field]+delta
        for offset,target,relative in patches:struct.pack_into('<I',body,offset,(values[target]-start-offset-4 if relative else base+values[target])&0xffffffff)
        if p[name+'Hex']!=body.hex():raise ValueError('allocation_code')
    module=p['systemModule'];expected=next(m for m in src('contracts/engine/loader-policy.json')['modules'] if m['name']=='ntdll.dll')
    if type(module) is not dict or set(module)!={'name','bytes','sha256'} or type(module['bytes']) is not int or module!={k:expected[k] for k in module}:raise ValueError('allocation_module')
    a=p['function']
    if type(a) is not dict or set(a)!={'rva','prefixHex','preferredBase','highlowOffsets'}:raise ValueError('allocation_function')
    if type(a['rva']) is not int or not 4096<=a['rva']<16*1024*1024-20 or type(a['preferredBase']) is not int or not 0<a['preferredBase']<=0xffff0000 or a['preferredBase']%65536:raise ValueError('allocation_function_range')
    if not isinstance(a['prefixHex'],str) or not re.fullmatch('[0-9a-f]{40}',a['prefixHex']):raise ValueError('allocation_function_prefix')
    offsets=a['highlowOffsets']
    if type(offsets) is not list or any(type(x) is not int or not 0<=x<=16 for x in offsets) or offsets!=sorted(offsets) or any(b-a<4 for a,b in zip(offsets,offsets[1:])):raise ValueError('allocation_function_relocations')
    function=f'BootstrapExportSpec{{{a["rva"]}U,'+byte_array(a['prefixHex'])+f',{a["preferredBase"]}U,{sum(1<<x for x in offsets)}U'+'}'
    return '\n'.join(['// Generated by tools/cd_stream_allocation_policy.py. Do not edit.','#pragma once','#include "saex/engine/cd_stream_allocation.hpp"','#include "saex/engine/cd_stream_disk_policy.generated.hpp"','namespace saex::engine {',
        f'static_assert(reviewed_cd_stream_disk_digest=="{p["diskPolicySha256"]}");',f'inline constexpr std::string_view reviewed_cd_stream_allocation_id="{p["id"]}";',f'inline constexpr std::string_view reviewed_cd_stream_allocation_digest="{hashlib.sha256(data).hexdigest()}";',
        'inline constexpr CdStreamAllocationSpec reviewed_cd_stream_allocation_spec{'+','.join(str(p[k])+'U' for k in FIELDS)+','+function+'};','}',''])
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args();paths=[ROOT/p for p in SOURCES]
    if any(p.stat().st_size>65536 for p in paths):raise ValueError('allocation_size')
    result=compile_source(*(p.read_bytes() for p in paths))
    if args.check:
        if not (ROOT/OUTPUT).is_file() or (ROOT/OUTPUT).read_text(encoding='utf-8')!=result:raise ValueError('stale_allocation_output')
    else:(ROOT/OUTPUT).write_text(result,encoding='utf-8',newline='\n')
    print('CdStream allocation policy verified.' if args.check else 'CdStream allocation policy generated.')
if __name__=='__main__':main()
