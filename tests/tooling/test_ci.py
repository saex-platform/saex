from pathlib import Path
import importlib.util
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('saex_ci', ROOT / 'tools/ci.py')
ci = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ci)


class GitHubContextTests(unittest.TestCase):
    def test_first_push_has_no_base_commit(self):
        self.assertIsNone(ci.documentation_base('push', {'before': '0' * 40}))

    def test_push_compares_previous_commit(self):
        sha = 'a1' * 20
        self.assertEqual(ci.documentation_base('push', {'before': sha}), sha)

    def test_pr_uses_base_not_head_or_title(self):
        sha = '12' * 20
        event = {'pull_request': {'base': {'sha': sha}, 'head': {'sha': '34' * 20},
                                  'title': 'untrusted text'}}
        self.assertEqual(ci.documentation_base('pull_request', event), sha)

    def test_zero_pr_base_is_rejected(self):
        with self.assertRaises(ValueError):
            ci.documentation_base('pull_request', {'pull_request': {'base': {'sha': '0' * 40}}})

    def test_invalid_sha_is_rejected(self):
        for value in ('--output=bad', 'main', 'a' * 39, 'a' * 41, None, ['a' * 40], 'a' * 40 + '\n'):
            with self.subTest(value=value), self.assertRaises(ValueError):
                ci.documentation_base('push', {'before': value})

    def test_missing_payload_is_rejected(self):
        with self.assertRaises(KeyError):
            ci.documentation_base('pull_request', {})

    def test_privileged_event_is_not_accepted(self):
        with self.assertRaises(ValueError):
            ci.documentation_base('pull_request_target', {})

    def test_manual_run_checks_full_checkout(self):
        self.assertIsNone(ci.documentation_base('workflow_dispatch', {}))


if __name__ == '__main__':
    unittest.main()
