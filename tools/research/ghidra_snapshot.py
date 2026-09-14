"""Bounded local Ghidra/bridge export with explicit gaps; never grants native permission."""
import argparse
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[2]
LOCK=ROOT/'contracts/research/ghidra-tools.json'
MAX_FILE=8*1024*1024

def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def load(path):
    if path.stat().st_size>MAX_FILE:raise ValueError('research_file_limit')
    def pairs(items):
        result={}
        for key,value in items:
            if key in result:raise ValueError('research_duplicate_key')
            result[key]=value
        return result
    return json.loads(path.read_text(encoding='utf-8'),object_pairs_hook=pairs)

def verify(directory):
    """Integrity and scope checks, not authenticity or native equivalence proof."""
    directory=directory.resolve();record=load(directory/'snapshot.json');lock=load(LOCK)
    if type(record.get('schemaVersion')) is not int or record['schemaVersion']!=1 or record.get('executableSha256')!=lock['executableSha256'] or record.get('lockSha256')!=digest(LOCK):raise ValueError('research_identity')
    if record.get('runtimePermission') is not False or record.get('completeCallClosure') is not False:raise ValueError('research_permission_claim')
    files=record.get('files');roots=record.get('roots');missing=record.get('missingRoots');addresses=record.get('addresses')
    if not isinstance(files,dict) or not 1<=len(files)<=65 or roots!=lock['roots'] or not isinstance(missing,list) or not isinstance(addresses,list):raise ValueError('research_scope')
    if len(set(addresses))!=len(addresses) or len(set(missing))!=len(missing) or any(not re.fullmatch('[0-9a-f]{8}',a) for a in addresses+missing):raise ValueError('research_addresses')
    if set(missing)!=(set(roots)-set(addresses)) or set(files)!={'_index.json',*(a+'.json' for a in addresses)}:raise ValueError('research_incomplete_inventory')
    for name,expected in files.items():
        if not re.fullmatch(r'(?:_index|[0-9a-f]{8})\.json',name):raise ValueError('research_path')
        path=directory/name
        if path.is_symlink() or not path.resolve().is_relative_to(directory) or not path.is_file() or path.stat().st_size>MAX_FILE or digest(path)!=expected:raise ValueError('research_file_integrity')
    index=load(directory/'_index.json')
    if set(index)!=set(addresses):raise ValueError('research_index')
    for address in addresses:
        data=load(directory/(address+'.json'))
        if data.get('address')!=address or not isinstance(data.get('decompiled'),str) or not isinstance(data.get('assembly'),list):raise ValueError('research_export_shape')
    return record

def export(args):
    lock=load(LOCK)
    if digest(args.executable)!=lock['executableSha256']:raise ValueError('research_executable_identity')
    if args.output.exists():raise ValueError('research_output_exists')
    # Verify the installed bridge source as well as its package version.
    import ghidra_ai_bridge
    package=Path(ghidra_ai_bridge.__file__).parent
    for name,expected in lock['bridgeFiles'].items():
        if digest(package/name)!=expected:raise ValueError('research_bridge_source')
    for package_name,version in lock['packages'].items():
        if importlib.metadata.version(package_name)!=version:raise ValueError('research_tool_version')
    if digest(args.ghidra/'Ghidra/application.properties')!=lock['ghidraPropertiesSha256'] or digest(args.java/'release')!=lock['javaReleaseSha256']:raise ValueError('research_runtime_version')
    # Explicit local paths; no global environment or model-provider configuration.
    os.environ['GHIDRA_INSTALL_DIR']=str(args.ghidra.resolve());os.environ['JAVA_HOME']=str(args.java.resolve());os.environ['JAVA_HOME_OVERRIDE']=os.environ['JAVA_HOME']
    import pyghidra
    from ghidra_ai_bridge.exporters.runner import export_decompiled
    args.output.mkdir(parents=True)
    with pyghidra.open_program(None,project_location=args.project_directory.resolve(),project_name=args.project_name,
            program_name='gta_sa.exe',analyze=False,nested_project_location=False) as api:
        program=api.getCurrentProgram()
        if str(program.getExecutableSHA256())!=lock['executableSha256']:raise ValueError('research_project_identity')
        fm=program.getFunctionManager();space=program.getAddressFactory().getDefaultAddressSpace()
        from ghidra.util.task import ConsoleTaskMonitor
        selected={};missing=[];frontier=[int(a,16) for a in lock['roots']]
        truncated=False
        for depth in range(2):
            following=set()
            for address in frontier:
                function=fm.getFunctionAt(space.getAddress(address))
                if function is None or function.isExternal():
                    if depth==0:missing.append(f'{address:08x}')
                    continue
                selected[address]=function
                for callee in function.getCalledFunctions(ConsoleTaskMonitor()):
                    if not callee.isExternal() and callee.getEntryPoint().getOffset() not in selected:following.add(callee.getEntryPoint().getOffset())
            frontier=sorted(following)
            if len(selected)+len(frontier)>64:truncated=True;break
        class BoundedFunctions:
            def getFunctions(self,forward):return list(selected.values())
            def __getattr__(self,name):return getattr(fm,name)
        export_decompiled(program,BoundedFunctions(),program.getReferenceManager(),str(args.output),program.getListing())
        addresses=[f'{a:08x}' for a in selected]
        issues=[]
        for address in addresses:
            data=load(args.output/(address+'.json'))
            if data.get('pcode_errors') or data.get('cfg_errors') or not data.get('assembly') or not data.get('pcode') or not data.get('cfg') or data['decompiled'].lstrip().startswith('//'):
                issues.append({'address':address,'kind':'incomplete_export'})
            if data.get('is_thunk'):issues.append({'address':address,'kind':'thunk_requires_target_review'})
            if data.get('calling_convention')=='unknown':issues.append({'address':address,'kind':'abi_not_proven'})
        record={'schemaVersion':1,'executableSha256':lock['executableSha256'],'lockSha256':digest(LOCK),'roots':lock['roots'],
            'addresses':addresses,'missingRoots':sorted(set(lock['roots'])-set(addresses)),
            'depthLimit':2,'functionLimit':64,'truncated':truncated,'issues':issues,'runtimePermission':False,'completeCallClosure':False,
            'files':{p.name:digest(p) for p in sorted(args.output.glob('*.json'))}}
        (args.output/'snapshot.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    if digest(args.executable)!=lock['executableSha256']:raise ValueError('research_executable_changed')
    return verify(args.output)

def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    v=sub.add_parser('verify');v.add_argument('directory',type=Path)
    e=sub.add_parser('export')
    for name in ('executable','ghidra','java','project-directory','output'):e.add_argument('--'+name,type=Path,required=True)
    e.add_argument('--project-name',required=True)
    args=parser.parse_args()
    try:
        record=verify(args.directory) if args.command=='verify' else export(args)
        print(json.dumps({'integrityVerified':True,'functions':len(record['addresses']),'missingRoots':record['missingRoots'],
            'issues':record['issues'],'runtimePermission':False,'completeCallClosure':False},indent=2))
        return 3 # A collected static snapshot is never runtime success.
    except (ValueError,OSError,KeyError,TypeError,ImportError) as error:
        print('Ghidra research rejected: '+str(error),file=sys.stderr);return 1
if __name__=='__main__':raise SystemExit(main())
