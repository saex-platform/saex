import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("engine_profiles", ROOT / "tools/engine_profiles.py")
profiles = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profiles)


class EngineProfilesTests(unittest.TestCase):
    def setUp(self):
        self.raw = (ROOT / profiles.SOURCE).read_bytes()
        self.profile = json.loads(self.raw)

    def test_generated_matches_shared_source(self):
        self.assertEqual((ROOT / profiles.OUTPUT).read_text(encoding="utf-8"),
                         profiles.generate(self.profile, hashlib.sha256(self.raw).hexdigest()))

    def test_observation_cannot_be_upgraded_by_flag(self):
        self.profile["observationOnly"] = False
        with self.assertRaises(ValueError): profiles.validate(self.profile)

    def test_runtime_verified_anchor_rejected(self):
        self.profile["anchors"][0]["runtimeVerified"] = True
        with self.assertRaises(ValueError): profiles.validate(self.profile)

    def test_anchor_bounds_and_identity(self):
        for key, value in [("rva", 0xffffffff), ("sectionIndex", -1), ("bytes", "00" * 17),
                           ("bytes", "gg"), ("id", '"; injected')]:
            with self.subTest(key=key, value=value):
                p = copy.deepcopy(self.profile); p["anchors"][0][key] = value
                with self.assertRaises(ValueError): profiles.validate(p)

    def test_duplicate_anchor_rejected(self):
        self.profile["anchors"].append(copy.deepcopy(self.profile["anchors"][0]))
        with self.assertRaises(ValueError): profiles.validate(self.profile)

    def test_numeric_bool_not_integer(self):
        for key in ["schemaVersion", "fileBytes"]:
            p = copy.deepcopy(self.profile); p[key] = True
            with self.assertRaises(ValueError): profiles.validate(p)

    def test_duplicate_json_key_rejected(self):
        with self.assertRaises(ValueError):
            json.loads('{"observationOnly":false,"observationOnly":true}', object_pairs_hook=profiles.unique_object)

    def test_section_collection_limit(self):
        self.profile["layout"]["sections"] *= 10
        with self.assertRaises(ValueError): profiles.validate(self.profile)


if __name__ == "__main__": unittest.main()
