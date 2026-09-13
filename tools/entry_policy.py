"""Compile an explicit entry-boundary supplement; no DLL discovery or automatic approval."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import loader_policy

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path('contracts/engine/entry-policy.json')
OUTPUT = Path('include/saex/engine/entry_policy.generated.hpp')


def compile_source(data, base_data):
    if len(data) > 65536 or len(base_data) > 65536: raise ValueError('entry_policy_source_size')
    policy = json.loads(data.decode('utf-8'), object_pairs_hook=loader_policy.strict_object)
    base = json.loads(base_data.decode('utf-8'), object_pairs_hook=loader_policy.strict_object)
    loader_policy.validate(base)
    keys = {'schemaVersion','id','stage','basePolicySha256','engineSha256','reviewDocument','additionalModules'}
    if type(policy) is not dict or set(policy) != keys: raise ValueError('entry_policy_keys')
    if type(policy['schemaVersion']) is not int or policy['schemaVersion'] != 1 or policy['stage'] != 'pe-entry-boundary':
        raise ValueError('entry_policy_stage')
    if not isinstance(policy['id'],str) or not re.fullmatch(r'[a-z0-9.-]{1,96}',policy['id']): raise ValueError('entry_policy_id')
    base_digest = hashlib.sha256(base_data).hexdigest()
    if policy['basePolicySha256'] != base_digest: raise ValueError('entry_policy_base_drift')
    if policy['engineSha256'] != base['engineSha256']: raise ValueError('entry_policy_engine_drift')
    if policy['reviewDocument'] != 'docs/development/d1-entry-boundary.md': raise ValueError('entry_policy_review')
    extra = policy['additionalModules']
    if type(extra) is not list or not 1 <= len(extra) <= 8: raise ValueError('entry_policy_additional_limit')
    if any(type(m) is not dict or m.get('origin') != 'system-x86' for m in extra): raise ValueError('entry_policy_additional_origin')
    # Reuse strict per-module/hash/size/order/budget validation; base entries cannot be overridden.
    supplement = dict(base, modules=extra)
    loader_policy.validate(supplement)
    combined = dict(base, id=policy['id'], modules=sorted(base['modules'] + extra, key=lambda m:m['name']))
    loader_policy.validate(combined)
    output = loader_policy.generate(combined,hashlib.sha256(data).hexdigest())
    output = output.replace('tools/loader_policy.py','tools/entry_policy.py').replace('reviewed_loader_', 'reviewed_entry_')
    output = output.replace('#include "saex/engine/loader_policy.hpp"', '#include "saex/engine/loader_policy.generated.hpp"')
    output = output.replace('namespace saex::engine {', 'namespace saex::engine {\n' +
        f'static_assert(reviewed_loader_policy_digest == "{base_digest}", "entry policy base drift");')
    return output


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--check',action='store_true'); args=parser.parse_args()
    source=ROOT/SOURCE; base=ROOT/loader_policy.SOURCE
    if source.stat().st_size > 65536 or base.stat().st_size > 65536: raise ValueError('entry_policy_source_size')
    data=source.read_bytes(); base_data=base.read_bytes()
    if not (ROOT/'docs/development/d1-entry-boundary.md').is_file(): raise ValueError('entry_policy_review_missing')
    observed=json.loads((ROOT/'contracts/engine/observed-profile.json').read_text(encoding='utf-8'))
    if json.loads(base_data)['engineSha256'] != observed['sha256']: raise ValueError('entry_policy_observed_engine_drift')
    output=compile_source(data,base_data)
    if args.check:
        if not (ROOT/OUTPUT).is_file() or (ROOT/OUTPUT).read_text(encoding='utf-8') != output:
            raise ValueError('stale_entry_policy_generated_output')
    else: (ROOT/OUTPUT).write_text(output,encoding='utf-8',newline='\n')
    print('Reviewed entry-boundary policy verified.' if args.check else 'Reviewed entry-boundary policy generated.')


if __name__ == '__main__': main()
