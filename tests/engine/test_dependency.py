"""Failure-path tests for pinned acquisition; no network or GTA required."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("saex_dependency", ROOT / "tools/native/dependency.py")
dep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dep)


class DependencyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="saex-dependency-test-")
        self.root = Path(self.temp.name)
        self.files = {"shared/first.h": b"first", "shared/second.cpp": b"second"}
        self.entries = dep.inventory(self.files)
        dep.publish_tree(self.root / "source", self.files)

    def tearDown(self):
        self.temp.cleanup()

    def test_exact_inventory_roundtrip(self):
        self.assertEqual(dep.verify_tree(self.root / "source", self.entries), self.files)

    def test_modified_header_rejected(self):
        (self.root / "source/shared/first.h").write_bytes(b"other")
        with self.assertRaisesRegex(ValueError, "digest_mismatch"):
            dep.verify_tree(self.root / "source", self.entries)

    def test_missing_source_rejected(self):
        (self.root / "source/shared/second.cpp").unlink()
        with self.assertRaisesRegex(ValueError, "missing_source"):
            dep.verify_tree(self.root / "source", self.entries)

    def test_shadow_header_rejected(self):
        (self.root / "source/PluginBase.h").write_bytes(b"shadow")
        with self.assertRaisesRegex(ValueError, "unexpected_source"):
            dep.verify_tree(self.root / "source", self.entries)

    def test_existing_cache_never_overwritten(self):
        changed = {**self.files, "shared/first.h": b"changed"}
        with self.assertRaises(ValueError):
            dep.publish_tree(self.root / "source", changed)
        self.assertEqual((self.root / "source/shared/first.h").read_bytes(), b"first")

    def test_windows_traversal_ads_and_device_paths_rejected(self):
        for name in ("../outside", "/absolute", "C:/outside", "shared\\escape", "a:b", "a/../b", "a//b", "CON.h", "x/NUL", "x/LPT1.cpp", "tail."):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, "unsafe_path"):
                dep.relative(name)

    def test_case_colliding_inventory_rejected(self):
        entries = self.entries + [{**self.entries[0], "path": self.entries[0]["path"].upper()}]
        with self.assertRaisesRegex(ValueError, "duplicate_inventory"):
            dep.validate_entries(entries)

    def test_duplicate_json_keys_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate_json"):
            dep.parse(b'{"sha256":"a","sha256":"b"}')

    def patch(self):
        return {"schemaVersion": 1, "beforeSha256": dep.digest(b"before"), "afterSha256": dep.digest(b"after"), "replacements": [{"old": "before", "new": "after"}]}

    def test_patch_requires_exact_before_and_after(self):
        self.assertEqual(dep.apply_patch(b"before", self.patch()), b"after")
        with self.assertRaisesRegex(ValueError, "preimage"):
            dep.apply_patch(b"other", self.patch())
        patch = self.patch()
        patch["afterSha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "postimage"):
            dep.apply_patch(b"before", patch)

    def test_repeated_patch_context_rejected(self):
        patch = self.patch()
        patch["beforeSha256"] = dep.digest(b"beforebefore")
        with self.assertRaisesRegex(ValueError, "not_unique"):
            dep.apply_patch(b"beforebefore", patch)

    def test_tampered_recipe_rejected(self):
        entry = {"path": "recipe.cmake", "bytes": 4, "sha256": dep.digest(b"good")}
        (self.root / entry["path"]).write_bytes(b"evil")
        with self.assertRaisesRegex(ValueError, "digest_mismatch"):
            dep.verify_recipe(self.root, {"recipeFiles": [entry]})

    def archive(self, names=None):
        path = self.root / "source.zip"
        prefix = "plugin-sdk-sa-" + "a" * 40 + "/"
        with zipfile.ZipFile(path, "w") as archive:
            for name, data in (names or self.files).items():
                archive.writestr(prefix + name, data)
        return path, {"upstream": {"commit": "a" * 40, "archiveBytes": path.stat().st_size, "archiveSha256": dep.digest(path.read_bytes())}, "files": self.entries}

    def test_archive_extracts_only_verified_inventory(self):
        path, lock = self.archive({**self.files, "installer.exe": b"not executed", "../outside": b"not extracted"})
        self.assertEqual(dep.archive_files(path, lock), self.files)
        self.assertFalse((self.root / "outside").exists())

    def test_corrupt_archive_rejected_before_extraction(self):
        path, lock = self.archive()
        path.write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "digest_mismatch"):
            dep.archive_files(path, lock)

    def test_archive_wrong_file_hash_rejected(self):
        path, lock = self.archive({**self.files, "shared/first.h": b"wrong"})
        with self.assertRaisesRegex(ValueError, "digest_mismatch"):
            dep.archive_files(path, lock)

    def test_archive_symlink_rejected(self):
        path, lock = self.archive()
        with zipfile.ZipFile(path, "w") as archive:
            for name, data in self.files.items():
                info = zipfile.ZipInfo("plugin-sdk-sa-" + "a" * 40 + "/" + name)
                info.create_system = 3
                info.external_attr = (0o120777 << 16)
                archive.writestr(info, data)
        lock["upstream"]["archiveBytes"] = path.stat().st_size
        lock["upstream"]["archiveSha256"] = dep.digest(path.read_bytes())
        with self.assertRaisesRegex(ValueError, "invalid_archive_entry"):
            dep.archive_files(path, lock)

    def test_source_digest_change_invalidates_lock(self):
        lock = dep.load_lock(ROOT)
        lock["files"][0]["sha256"] = "0" * 64
        target = self.root / dep.LOCK
        target.parent.mkdir(parents=True)
        target.write_text(json.dumps(lock), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "source_inventory_digest"):
            dep.load_lock(self.root)

    def test_recipe_and_sources_are_verified_live(self):
        lock = dep.verify(ROOT)
        self.assertFalse(lock["runtimeEligible"])
        self.assertEqual(lock["toolchain"]["architecture"], "x86")


if __name__ == "__main__":
    unittest.main()
