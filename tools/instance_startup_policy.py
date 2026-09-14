"""Compile the exact named-event helper; never authorize its window activation branch."""
import argparse, hashlib, json, re
from pathlib import Path
import platform_suppression_policy as parent
import loader_policy
from frame_target_policy import byte_array
ROOT=Path(__file__).resolve().parents[1]
SOURCE=Path('contracts/engine/instance-startup-policy.json')
OUTPUT=Path('include/saex/engine/instance_startup_policy.generated.hpp')
SOURCES=(SOURCE,*parent.SOURCES)
OFFSETS=(1,14,20,33,39,47,58,69,78)
SHAPE='a100000000506a016a006a00ff1500000000ff15000000003db700000075398b0d000000008b15000000005152ff150000000085c0740d50ff1500000000b801000000c3a1000000008b0851ff1500000000b801000000c333c0c3'

def compile_source(data,*parents):
    if max(map(len,(data,*parents)))>65536:raise ValueError('instance_size')
    parent.compile_source(*parents)
    p=json.loads(data,object_pairs_hook=loader_policy.strict_object)
    keys={'schemaVersion','id','stage','suppressionPolicySha256','reviewDocument','callRva','targetRva','callHex','bodyHex','nameRva','name','createThunkRva','createThunkSlotRva','createFunction','systemModules'}
    if type(p) is not dict or set(p)!=keys:raise ValueError('instance_keys')
    if type(p['schemaVersion']) is not int or p['schemaVersion']!=1 or p['stage']!='named-event-return-before-caller-test' or not isinstance(p['id'],str) or not re.fullmatch('[a-z0-9.-]{1,96}',p['id']):raise ValueError('instance_identity')
    if p['suppressionPolicySha256']!=hashlib.sha256(parents[0]).hexdigest():raise ValueError('instance_parent')
    if p['reviewDocument']!='docs/development/d1-instance-startup.md':raise ValueError('instance_review')
    def src(name):return json.loads(parents[parent.SOURCES.index(Path(name))])
    layout=src('contracts/engine/observed-profile.json')['layout'];base=layout['imageBase'];size=layout['imageSize']
    platform=src('contracts/engine/platform-startup-policy.json');suppression=json.loads(parents[0])
    def region(r,n,code,bss=False):
        return type(r) is int and 4096<=r<=size-n and any(s['rva']<=r and r+n<=s['rva']+(s['virtualSize'] if bss else s['rawSize']) and bool(s['characteristics']&0x20000000)==code for s in layout['sections'])
    if not region(p['callRva'],5,True) or not region(p['targetRva'],91,True) or p['callRva']!=platform['callRva']+6:raise ValueError('instance_code_range')
    if not isinstance(p['callHex'],str) or not re.fullmatch('e8[0-9a-f]{8}',p['callHex']) or p['callHex']!=platform['returnPrefixHex'][:10] or p['callRva']+5+int.from_bytes(bytes.fromhex(p['callHex'])[1:],'little',signed=True)!=p['targetRva']:raise ValueError('instance_call')
    if not isinstance(p['bodyHex'],str) or not re.fullmatch('[0-9a-f]{182}',p['bodyHex']):raise ValueError('instance_body')
    body=bytearray.fromhex(p['bodyHex']);rvas=[]
    for offset in OFFSETS:
        r=int.from_bytes(body[offset:offset+4],'little')-base
        if r%4 or not region(r,4,False,bss=offset in (33,69)):raise ValueError('instance_operand')
        rvas.append(r);body[offset:offset+4]=bytes(4)
    if body.hex()!=SHAPE or rvas[0]!=rvas[4] or rvas[6]!=rvas[8] or rvas[2]!=suppression['lastErrorIatRva']:raise ValueError('instance_shape')
    if p['name']!='Grand theft auto San Andreas' or not region(p['nameRva'],len(p['name'])+1,False):raise ValueError('instance_name')
    spans=[(p['targetRva'],91),(platform['callRva']-len(platform['prologueHex'])//2,len(platform['prologueHex'])//2+22),(p['nameRva'],len(p['name'])+1),*((r,4) for r in sorted(set(rvas)))]
    for i,(r,n) in enumerate(spans):
        if any(r<b+m and b<r+n for b,m in spans[:i]):raise ValueError('instance_overlap')
    modules=p['systemModules'];expected=src('contracts/engine/loader-policy.json')['modules']
    if type(modules) is not list or len(modules)!=2:raise ValueError('instance_modules')
    for m,name in zip(modules,('kernel32.dll','kernelbase.dll')):
        e=next(x for x in expected if x['name']==name)
        if type(m) is not dict or set(m)!={'name','bytes','sha256'} or type(m['bytes']) is not int or m!={k:e[k] for k in m}:raise ValueError('instance_module')
    for k,n in [('createThunkRva',6),('createThunkSlotRva',4)]:
        if type(p[k]) is not int or not 4096<=p[k]<16*1024*1024-n:raise ValueError('instance_thunk_range')
    a,b=p['createThunkRva'],p['createThunkSlotRva']
    if b%4 or (a<b+4 and b<a+6):raise ValueError('instance_thunk_slot')
    f=p['createFunction']
    if type(f) is not dict or set(f)!={'rva','prefixHex','preferredBase','highlowOffsets'}:raise ValueError('instance_function')
    if type(f['rva']) is not int or not 4096<=f['rva']<16*1024*1024-20 or type(f['preferredBase']) is not int or not 0<f['preferredBase']<=0xffff0000 or f['preferredBase']%65536 or f['highlowOffsets']!=[] or not isinstance(f['prefixHex'],str) or not re.fullmatch('[0-9a-f]{40}',f['prefixHex']):raise ValueError('instance_function_shape')
    frame=f'FrameTargetSpec{{{base}U,{size}U,{p["callRva"]}U,{p["targetRva"]}U,'+byte_array(p['callHex'])+','+byte_array(p['bodyHex'][:32])+'}'
    return '\n'.join(['// Generated by tools/instance_startup_policy.py. Do not edit.','#pragma once','#include "saex/engine/instance_startup.hpp"','#include "saex/engine/platform_suppression_policy.generated.hpp"','namespace saex::engine {',
        f'static_assert(reviewed_suppression_digest == "{p["suppressionPolicySha256"]}");',f'inline constexpr std::string_view reviewed_instance_id="{p["id"]}";',f'inline constexpr std::string_view reviewed_instance_digest="{hashlib.sha256(data).hexdigest()}";',
        'inline constexpr InstanceStartupSpec reviewed_instance_spec{'+frame+','+byte_array(body.hex())+',{'+','.join(str(r)+'U' for r in rvas)+'},'+f'{p["nameRva"]}U,{a}U,{b}U,"{p["name"]}",BootstrapExportSpec{{{f["rva"]}U,'+byte_array(f['prefixHex'])+f',{f["preferredBase"]}U,0U'+'}};','}',''])

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');a=parser.parse_args()
    paths=[ROOT/p for p in SOURCES]
    if any(p.stat().st_size>65536 for p in paths):raise ValueError('instance_size')
    if not (ROOT/'docs/development/d1-instance-startup.md').is_file():raise ValueError('instance_review_missing')
    result=compile_source(*(p.read_bytes() for p in paths))
    if a.check:
        if not (ROOT/OUTPUT).is_file() or (ROOT/OUTPUT).read_text(encoding='utf-8')!=result:raise ValueError('stale_instance_output')
    else:(ROOT/OUTPUT).write_text(result,encoding='utf-8',newline='\n')
    print('Instance startup policy verified.' if a.check else 'Instance startup policy generated.')
if __name__=='__main__':main()
