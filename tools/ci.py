"""GitHub documentation context and portable CI entry; never starts GTA."""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def documentation_base(event_name, event):
    if event_name == 'pull_request':
        value = event['pull_request']['base']['sha']
    elif event_name == 'push':
        value = event['before']
    elif event_name == 'workflow_dispatch':
        return None
    else:
        raise ValueError('Unsupported GitHub event')
    if not isinstance(value, str) or not re.fullmatch(r'[0-9a-f]{40}', value):
        raise ValueError('Invalid base commit SHA')
    if value == '0' * 40:
        if event_name == 'push':
            return None
        raise ValueError('Pull request base cannot be zero')
    return value


def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)


def docs():
    arguments = []
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        event = json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text(encoding='utf-8'))
        base = documentation_base(os.environ['GITHUB_EVENT_NAME'], event)
        if base:
            run('git', 'cat-file', '-e', base + '^{commit}')
            arguments = ['--base', base]
    run(sys.executable, 'tools/check_docs.py', *arguments)
    run(sys.executable, 'tests/tooling/test_doc_gate.py')
    run(sys.executable, 'tests/tooling/test_ci.py')


def linux():
    if sys.platform != 'linux':
        raise ValueError('Linux CI requires Linux')
    docs()
    run('dotnet', 'run', '--project', 'tools/ContractGen', '--', '.', '--check')
    for generator in ('engine_profiles', 'loader_policy', 'entry_policy', 'proxy_policy', 'startup_policy'):
        run(sys.executable, f'tools/{generator}.py', '--check')
    run('cmake', '--preset', 'linux-x64')
    run('cmake', '--build', '--preset', 'linux-x64', '--parallel', '2')
    run('ctest', '--preset', 'linux-x64')
    run('dotnet', 'run', '--project', 'tests/managed/Saex.Foundation.Tests', '--', '.',
        'out/linux-x64/saex_contract_probe')
    for suite in ('test_engine_profiles', 'test_loader_policy', 'test_entry_policy', 'test_proxy_policy', 'test_startup_policy'):
        run(sys.executable, f'tests/engine/{suite}.py')
    print('SAEX portable Linux core verified; Windows/GTA runtime is outside this job.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['docs', 'linux'])
    options = parser.parse_args()
    try:
        {'docs': docs, 'linux': linux}[options.mode]()
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        print(f'CI failed: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
