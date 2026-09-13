"""Structural documentation and source/document co-change gate; not a semantic proof."""
from pathlib import Path
import argparse
import fnmatch
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {'.git', 'out', 'bin', 'obj', '.vs', '__pycache__'}


def inventory(root):
    # Walk only project surfaces; never traverse compiler output/third-party tool trees.
    result = {}
    def visit(directory):
        for path in directory.iterdir():
            if path.is_symlink() or path.name in EXCLUDED:
                continue
            if path.is_dir():
                visit(path)
            elif path.is_file():
                result[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    visit(root)
    return result


def validate_docs(root, paths):
    errors = []
    links = 0
    blocks = 0
    for relative in sorted(p for p in paths if p.endswith('.md')):
        path = root / relative
        try:
            text = path.read_text(encoding='utf-8-sig')
        except UnicodeError:
            errors.append(f'{relative}: invalid UTF-8')
            continue
        if not text.startswith('# ') or '\ufffd' in text or '\x00' in text:
            errors.append(f'{relative}: invalid title/encoding')
        fence = None
        body = []
        for index, line in enumerate(text.splitlines(), 1):
            if line.startswith('```'):
                if fence is None:
                    fence = line[3:].strip()
                    body = []
                else:
                    if line != '```':
                        errors.append(f'{relative}:{index}: malformed fence')
                    if fence == 'json':
                        blocks += 1
                        try:
                            json.loads('\n'.join(body))
                        except ValueError as exc:
                            errors.append(f'{relative}:{index}: {exc}')
                    fence = None
                continue
            if fence is not None:
                body.append(line)
                continue
            for target in re.findall(r'!?\[[^\]\n]+\]\(([^)\n]+)\)', line):
                target = target.strip('<>')
                if re.match(r'^[a-z]+://', target):
                    continue
                links += 1
                target_path = target.split('#', 1)[0]
                if target_path and not (path.parent / target_path).is_file():
                    errors.append(f'{relative}:{index}: missing link {target}')
        if fence is not None:
            errors.append(f'{relative}: unclosed code fence')
    return errors, links, blocks


def cochange_errors(changed, mapping):
    errors = []
    changes_to_code = sorted(p for p in changed if not p.endswith('.md'))
    if changes_to_code:
        for required in mapping['always']:
            if required not in changed:
                errors.append(f'Implementation changed; update {required}')
    for path in changes_to_code:
        owners = [entry for entry in mapping['components']
                  if any(fnmatch.fnmatchcase(path, pattern) for pattern in entry['patterns'])]
        if not owners:
            errors.append(f'Unmapped implementation path: {path}')
        for owner in owners:
            for document in owner['documents']:
                if document not in changed:
                    errors.append(f'{path}: update owning document {document}')
    return sorted(set(errors))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base', help='Git revision; compare committed and working changes to this revision')
    parser.add_argument('--record', action='store_true', help='Record local hash baseline only after successful build/tests')
    options = parser.parse_args()
    current = inventory(ROOT)
    baseline_file = ROOT / 'out/verification/docs-baseline.json'
    if options.base:
        # Ref is a distinct argument, never interpreted as a shell command or Git option.
        if options.base.startswith('-'):
            parser.error('invalid base revision')
        result = subprocess.run(['git', 'diff', '--name-only', '-z', options.base, '--'], cwd=ROOT, capture_output=True, check=True)
        changed = set(result.stdout.decode('utf-8').strip('\0').split('\0')) - {''}
        untracked = subprocess.run(['git', 'ls-files', '--others', '--exclude-standard', '-z'], cwd=ROOT, capture_output=True, check=True)
        changed.update(set(untracked.stdout.decode('utf-8').strip('\0').split('\0')) - {''})
        baseline_kind = 'git:' + options.base
    elif baseline_file.is_file():
        previous = json.loads(baseline_file.read_text(encoding='utf-8'))
        changed = {path for path in current.keys() | previous.keys() if current.get(path) != previous.get(path)}
        baseline_kind = 'local verified hashes'
    else:
        changed = set(current)
        baseline_kind = 'initial working tree'
    mapping = json.loads((ROOT / 'docs/development/component-map.json').read_text(encoding='utf-8'))
    errors, links, blocks = validate_docs(ROOT, current)
    errors += cochange_errors(changed, mapping)
    for entry in mapping['components']:
        for document in entry['documents']:
            if not (ROOT / document).is_file():
                errors.append('Missing owning document: ' + document)
    report = {'markdownFiles': sum(p.endswith('.md') for p in current), 'localLinks': links,
              'jsonExamples': blocks, 'baseline': baseline_kind, 'changedPaths': len(changed), 'errors': errors}
    output = ROOT / 'out/verification'
    output.mkdir(parents=True, exist_ok=True)
    (output / 'docs-check.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    if options.record:
        baseline_file.write_text(json.dumps(current, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
