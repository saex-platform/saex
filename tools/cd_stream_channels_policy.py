"""Compile the pinned SetLastError / LPTR channel-storage boundary."""
import argparse,hashlib,json,re,struct
from pathlib import Path
import cd_stream_allocation_policy as parent
import loader_policy
from frame_target_policy import byte_array
ROOT=Path(__file__).resolve().parents[1]
SOURCE=Path('contracts/engine/cd-stream-channels-policy.json')
OUTPUT=Path('include/saex/engine/cd_stream_channels_policy.generated.hpp')
SOURCES=(SOURCE,*parent.SOURCES)
FIELDS=('errorIatRva','localIatRva','pointerRva','filenameRva','openRva','errorThunkSlotRva','localThunkSlotRva')
EXPORTS=('errorThunk','errorImplementation','localThunk','localImplementation')
TEMPLATE='83c40c6a008bf8ff15000000008b44241c8d0c40c1e104516a40c7050000000000000000a300000000ff15000000006a006800000000a300000000e800000000'
def compile_source(data,*parents):
    if max(map(len,(data,*parents)))>65536:raise ValueError('channels_size')
    parent.compile_source(*parents);p=json.loads(data,object_pairs_hook=loader_policy.strict_object)
    keys={'schemaVersion','id','stage','allocationPolicySha256','reviewDocument','bodyHex','filename','channelCount','channelStride','eventLimit','systemModules',*FIELDS,*EXPORTS}
    if type(p) is not dict or set(p)!=keys:raise ValueError('channels_keys')
    if type(p['schemaVersion']) is not int or p['schemaVersion']!=1 or not isinstance(p['id'],str) or not re.fullmatch('[a-z0-9.-]{1,96}',p['id']) or p['stage']!='before-stream-open':raise ValueError('channels_identity')
    if p['allocationPolicySha256']!=hashlib.sha256(parents[0]).hexdigest():raise ValueError('channels_parent')
    if p['reviewDocument']!='docs/development/d1-cd-stream-channels.md' or p['filename']!='MODELS\\GTA3.IMG':raise ValueError('channels_review')
    for k,v in [('channelCount',5),('channelStride',48),('eventLimit',160)]:
        if type(p[k]) is not int or p[k]!=v:raise ValueError('channels_budget')
    def src(name):return json.loads(parents[parent.SOURCES.index(Path(name))])
    layout=src('contracts/engine/observed-profile.json')['layout'];base=layout['imageBase'];size=layout['imageSize'];tables=src('contracts/engine/cd-stream-tables-policy.json')
    for field in FIELDS:
        v=p[field]
        if type(v) is not int or not 4096<=v<(16*1024*1024-4 if 'ThunkSlot' in field else size):raise ValueError('channels_range')
        if ('Iat' in field or 'Slot' in field or field=='pointerRva') and v%4:raise ValueError('channels_alignment')
    spans=[(tables['targetRva']+137,64,0x20000000),(p['pointerRva']-4,12,0xc0000000),(p['filenameRva'],16,0x40000000),(p['openRva'],5,0x20000000)]
    spans += [(p[k],4,0x40000000) for k in ('errorIatRva','localIatRva')]
    spans += [(tables['namesRva']-8,8,0xc0000000)]
    for i,(r,length,mask) in enumerate(spans):
        if not any(s['rva']<=r and r+length<=s['rva']+(s['virtualSize'] if mask==0xc0000000 else s['rawSize']) and s['characteristics']&mask==mask for s in layout['sections']):raise ValueError('channels_section')
        if any(r<a+b and a<r+length for a,b,_ in spans[:i]):raise ValueError('channels_overlap')
    body=bytearray.fromhex(TEMPLATE)
    for at,value in [(9,base+p['errorIatRva']),(28,base+tables['namesRva']-4),(37,base+tables['namesRva']-8),(43,base+p['localIatRva']),(50,base+p['filenameRva']),(55,base+p['pointerRva']),(60,(p['openRva']-tables['targetRva']-201)&0xffffffff)]:struct.pack_into('<I',body,at,value)
    if p['bodyHex']!=body.hex():raise ValueError('channels_body')
    modules=src('contracts/engine/loader-policy.json')['modules'];expected=[{k:next(m for m in modules if m['name']==name)[k] for k in ('name','bytes','sha256')} for name in ('kernel32.dll','kernelbase.dll','ntdll.dll')]
    if type(p['systemModules']) is not list or any(type(m) is not dict or type(m.get('bytes')) is not int for m in p['systemModules']) or p['systemModules']!=expected:raise ValueError('channels_modules')
    recipes=[]
    for key in EXPORTS:
        a=p[key]
        if type(a) is not dict or set(a)!={'rva','prefixHex','preferredBase','highlowOffsets'}:raise ValueError('channels_export')
        if type(a['rva']) is not int or not 4096<=a['rva']<16*1024*1024-20 or type(a['preferredBase']) is not int or not 0<a['preferredBase']<=0xffff0000 or a['preferredBase']%65536:raise ValueError('channels_export_range')
        if not isinstance(a['prefixHex'],str) or not re.fullmatch('[0-9a-f]{40}',a['prefixHex']):raise ValueError('channels_prefix')
        offsets=a['highlowOffsets']
        if type(offsets) is not list or any(type(v) is not int or not 0<=v<=16 for v in offsets) or offsets!=sorted(offsets) or any(b-a<4 for a,b in zip(offsets,offsets[1:])):raise ValueError('channels_relocations')
        if key in ('errorThunk','localThunk'):
            local=key=='localThunk';at=8 if local else 2;prefix=bytes.fromhex(a['prefixHex']);slot=p['localThunkSlotRva' if local else 'errorThunkSlotRva']
            if prefix[:at]!=(bytes.fromhex('8bff558bec5dff25') if local else bytes.fromhex('ff25')) or struct.unpack_from('<I',prefix,at)[0]!=a['preferredBase']+slot or offsets!=[at]:raise ValueError('channels_thunk')
        recipes.append(f'BootstrapExportSpec{{{a["rva"]}U,'+byte_array(a['prefixHex'])+f',{a["preferredBase"]}U,{sum(1<<x for x in offsets)}U'+'}')
    return '\n'.join(['// Generated by tools/cd_stream_channels_policy.py. Do not edit.','#pragma once','#include "saex/engine/cd_stream_channels.hpp"','#include "saex/engine/cd_stream_allocation_policy.generated.hpp"','namespace saex::engine {',f'static_assert(reviewed_cd_stream_allocation_digest=="{p["allocationPolicySha256"]}");',f'inline constexpr std::string_view reviewed_cd_stream_channels_id="{p["id"]}";',f'inline constexpr std::string_view reviewed_cd_stream_channels_digest="{hashlib.sha256(data).hexdigest()}";','inline constexpr CdStreamChannelsSpec reviewed_cd_stream_channels_spec{'+','.join(str(p[k])+'U' for k in FIELDS)+','+','.join(recipes)+'};','}',''])
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--check',action='store_true');args=ap.parse_args()
    paths=[ROOT/p for p in SOURCES]
    if any(p.stat().st_size>65536 for p in paths):raise ValueError('channels_size')
    result=compile_source(*(p.read_bytes() for p in paths))
    if args.check:
        if not (ROOT/OUTPUT).is_file() or (ROOT/OUTPUT).read_text(encoding='utf-8')!=result:raise ValueError('stale_channels_output')
    else:(ROOT/OUTPUT).write_text(result,encoding='utf-8',newline='\n')
    print('CdStream channels policy verified.' if args.check else 'CdStream channels policy generated.')
if __name__=='__main__':main()
