from pathlib import Path
import importlib.util
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('check_docs', ROOT / 'tools/check_docs.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class DocumentationGateTests(unittest.TestCase):
    mapping = {'always': ['docs/status.md', 'docs/changes.md'], 'components': [
        {'patterns': ['src/*'], 'documents': ['docs/owner.md']} ]}

    def test_code_change_requires_status_and_owner(self):
        errors = gate.cochange_errors({'src/changed.cpp'}, self.mapping)
        self.assertEqual(len(errors), 3)

    def test_addition_or_deletion_uses_path_even_without_file(self):
        changed = {'src/removed.cpp', 'docs/status.md', 'docs/changes.md', 'docs/owner.md'}
        self.assertFalse(gate.cochange_errors(changed, self.mapping))

    def test_unknown_component_fails(self):
        self.assertTrue(any('Unmapped' in x for x in gate.cochange_errors({'new/component.cpp'}, self.mapping)))

    def test_docs_only_does_not_require_code(self):
        self.assertFalse(gate.cochange_errors({'docs/owner.md'}, self.mapping))

    def test_link_and_json_errors_detected(self):
        with tempfile.TemporaryDirectory(prefix='saex-doc-gate-') as tmp:
            root = Path(tmp)
            (root / 'README.md').write_text('# Test\n\n[missing](absent.md)\n```json\n{bad}\n```\n', encoding='utf-8')
            errors, links, examples = gate.validate_docs(root, {'README.md'})
            self.assertEqual((len(errors), links, examples), (2, 1, 1))


if __name__ == '__main__':
    unittest.main()
