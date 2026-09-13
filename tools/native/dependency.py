"""D1-N1 pinned source acquisition/verification. No upstream scripts are executed."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import sys
import tempfile
import time
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LOCK = Path("contracts/engine/plugin-sdk.lock.json")
MAX_ARCHIVE = 32 * 1024 * 1024
MAX_FILE = 4 * 1024 * 1024


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def relative(name: str) -> Path:
    # Reject Windows drive/ADS/device paths even when validation runs on Linux.
    if not isinstance(name, str) or not name or "\\" in name:
        raise ValueError("unsafe_path")
    p = PurePosixPath(name)
    if p.is_absolute() or str(p) != name:
        raise ValueError("unsafe_path")
    for part in p.parts:
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", part) or part in (".", "..") or part.endswith((".", " ")):
            raise ValueError("unsafe_path")
        if part.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *(f"COM{i}" for i in range(10)), *(f"LPT{i}" for i in range(10))}:
            raise ValueError("unsafe_path")
    return Path(*p.parts)


def safe_path(root: Path, name: str) -> Path:
    target = root / relative(name)
    for part in (target, *target.parents):
        if part.is_symlink() or (hasattr(part, "is_junction") and part.is_junction()):
            raise ValueError("linked_path")
        if part == root:
            break
    if not target.resolve().is_relative_to(root.resolve()):
        raise ValueError("outside_root")
    return target


def read(path: Path, limit: int = MAX_FILE) -> bytes:
    with path.open("rb") as stream:
        result = stream.read(limit + 1)
    if len(result) > limit:
        raise ValueError("file_size_limit")
    return result


def unique_object(items):
    result = {}
    for key, value in items:
        if key in result:
            raise ValueError("duplicate_json_key")
        result[key] = value
    return result


def parse(data: bytes):
    return json.loads(data, object_pairs_hook=unique_object)


def inventory(files: dict[str, bytes]) -> list[dict]:
    return [{"path": p, "bytes": len(data), "sha256": digest(data)} for p, data in sorted(files.items())]


def inventory_digest(entries: list[dict]) -> str:
    return digest("".join(f'{e["sha256"]} {e["bytes"]} {e["path"]}\n' for e in sorted(entries, key=lambda e: e["path"])).encode())


def validate_entries(entries: list[dict]) -> None:
    if not isinstance(entries, list) or not 1 <= len(entries) <= 512:
        raise ValueError("inventory_limit")
    names = set()
    for entry in entries:
        name = entry["path"]
        relative(name)
        if name.casefold() in names:
            raise ValueError("duplicate_inventory_path")
        names.add(name.casefold())
        if type(entry["bytes"]) is not int or not 0 <= entry["bytes"] <= MAX_FILE:
            raise ValueError("inventory_size")
        if not re.fullmatch(r"[0-9a-f]{64}", entry["sha256"]):
            raise ValueError("invalid_digest")


def load_lock(root: Path = ROOT) -> dict:
    lock = parse(read(safe_path(root, LOCK.as_posix())))
    if lock["schemaVersion"] != 1:
        raise ValueError("unsupported_lock_schema")
    for key in ("files", "recipeFiles"):
        validate_entries(lock[key])
    if inventory_digest(lock["files"]) != lock["sourceInventoryDigest"]:
        raise ValueError("source_inventory_digest")
    if inventory_digest(lock["recipeFiles"]) != lock["recipeDigest"]:
        raise ValueError("recipe_inventory_digest")
    upstream = lock["upstream"]
    if not re.fullmatch(r"[0-9a-f]{40}", upstream["commit"]):
        raise ValueError("invalid_commit")
    expected_url = "https://codeload.github.com/Dryxio/plugin-sdk-sa/zip/" + upstream["commit"]
    if upstream["archiveUrl"] != expected_url or not re.fullmatch(r"[0-9a-f]{64}", upstream["archiveSha256"]):
        raise ValueError("invalid_archive_identity")
    if not 0 < upstream["archiveBytes"] <= MAX_ARCHIVE or not 0 <= len(lock["patches"]) <= 16:
        raise ValueError("dependency_limit")
    for key in ("sourceDirectory", "buildSourceDirectory"):
        path = relative(lock[key])
        if path.parts[:2] != ("out", "dependencies"):
            raise ValueError("cache_outside_dependencies")
    if lock["sourceDirectory"] == lock["buildSourceDirectory"]:
        raise ValueError("source_must_remain_unmodified")
    return lock


def verify_file(data: bytes, entry: dict) -> None:
    if len(data) != entry["bytes"] or digest(data) != entry["sha256"]:
        raise ValueError("digest_mismatch: " + entry["path"])


def verify_tree(root: Path, entries: list[dict]) -> dict[str, bytes]:
    validate_entries(entries)
    if not root.is_dir() or root.is_symlink() or (hasattr(root, "is_junction") and root.is_junction()):
        raise ValueError("missing_or_linked_source")
    expected = {e["path"] for e in entries}
    observed = set()
    # Do not recurse into junctions/reparse directories or accept extra headers.
    for current, dirs, names in os.walk(root, followlinks=False):
        for name in (*dirs, *names):
            safe_path(root, (Path(current) / name).relative_to(root).as_posix())
        observed.update((Path(current) / name).relative_to(root).as_posix() for name in names)
        if not observed.issubset(expected):
            raise ValueError("unexpected_source_file")
    if observed != expected:
        raise ValueError("missing_source_file")
    result = {}
    for entry in entries:
        data = read(safe_path(root, entry["path"]))
        verify_file(data, entry)
        result[entry["path"]] = data
    return result


def apply_patch(data: bytes, patch: dict) -> bytes:
    if patch["schemaVersion"] != 1 or digest(data) != patch["beforeSha256"]:
        raise ValueError("patch_preimage_mismatch")
    if not 1 <= len(patch["replacements"]) <= 32:
        raise ValueError("patch_limit")
    for change in patch["replacements"]:
        old, new = change["old"].encode(), change["new"].encode()
        if not old or data.count(old) != 1:
            raise ValueError("patch_context_not_unique")
        data = data.replace(old, new, 1)
        if len(data) > MAX_FILE:
            raise ValueError("patched_size_limit")
    if digest(data) != patch["afterSha256"]:
        raise ValueError("patch_postimage_mismatch")
    return data


def patched_files(root: Path, files: dict[str, bytes], patches: list[dict]) -> dict[str, bytes]:
    result = dict(files)
    for entry in patches:
        raw = read(safe_path(root, entry["path"]))
        verify_file(raw, entry)
        patch = parse(raw)
        if patch["target"] not in result:
            raise ValueError("patch_target_not_in_inventory")
        result[patch["target"]] = apply_patch(result[patch["target"]], patch)
    return result


def verify_recipe(root: Path, lock: dict) -> None:
    for entry in lock["recipeFiles"]:
        verify_file(read(safe_path(root, entry["path"])), entry)


def verify(root: Path = ROOT) -> dict:
    lock = load_lock(root)
    verify_recipe(root, lock)
    original = verify_tree(safe_path(root, lock["sourceDirectory"]), lock["files"])
    patched = patched_files(root, original, lock["patches"])
    entries = inventory(patched)
    if inventory_digest(entries) != lock["buildSourceInventoryDigest"]:
        raise ValueError("patched_inventory_mismatch")
    verify_tree(safe_path(root, lock["buildSourceDirectory"]), entries)
    return lock


def archive_files(path: Path, lock: dict) -> dict[str, bytes]:
    verify_file(read(path, MAX_ARCHIVE), {"path": path.name, "bytes": lock["upstream"]["archiveBytes"], "sha256": lock["upstream"]["archiveSha256"]})
    prefix = "plugin-sdk-sa-" + lock["upstream"]["commit"] + "/"
    with zipfile.ZipFile(path) as archive:
        infos = archive.infolist()
        if len(infos) > 16384 or len({x.filename.casefold() for x in infos}) != len(infos):
            raise ValueError("archive_entry_limit_or_duplicate")
        result = {}
        for entry in lock["files"]:
            relative(entry["path"])
            info = archive.getinfo(prefix + entry["path"])
            if info.file_size != entry["bytes"] or stat.S_ISLNK(info.external_attr >> 16) or info.flag_bits & 1:
                raise ValueError("invalid_archive_entry")
            data = archive.read(info)
            verify_file(data, entry)
            result[entry["path"]] = data
        return result


def publish_tree(target: Path, files: dict[str, bytes]) -> None:
    if target.exists():
        verify_tree(target, inventory(files))
        return
    target.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=".saex-sdk-stage-", dir=target.parent))
    try:
        for name, data in files.items():
            path = safe_path(stage, name)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        verify_tree(stage, inventory(files))
        stage.rename(target)
    finally:
        # Only this invocation's newly allocated staging directory is disposable.
        if stage.exists() and stage.resolve().parent == target.parent.resolve() and not stage.is_symlink():
            shutil.rmtree(stage)


def acquire(root: Path = ROOT) -> dict:
    lock = load_lock(root)
    verify_recipe(root, lock)
    upstream = lock["upstream"]
    archive = safe_path(root, "out/dependencies/plugin-sdk-sa-" + upstream["commit"] + ".zip")
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        fd, temp = tempfile.mkstemp(prefix=".saex-sdk-download-", dir=archive.parent)
        try:
            with os.fdopen(fd, "wb") as output, urllib.request.urlopen(upstream["archiveUrl"], timeout=30) as response:
                total, started = 0, time.monotonic()
                while chunk := response.read(65536):
                    total += len(chunk)
                    if total > MAX_ARCHIVE or time.monotonic() - started > 90:
                        raise ValueError("download_budget")
                    output.write(chunk)
            archive_files(Path(temp), lock)
            Path(temp).rename(archive)
        finally:
            Path(temp).unlink(missing_ok=True)
    original = archive_files(archive, lock)
    patched = patched_files(root, original, lock["patches"])
    if inventory_digest(inventory(patched)) != lock["buildSourceInventoryDigest"]:
        raise ValueError("patched_inventory_mismatch")
    publish_tree(safe_path(root, lock["sourceDirectory"]), original)
    publish_tree(safe_path(root, lock["buildSourceDirectory"]), patched)
    return verify(root)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=("acquire", "verify"))
    args = parser.parse_args()
    try:
        lock = acquire() if args.action == "acquire" else verify()
        print(json.dumps({"verified": True, "scope": "D1-N1-build-only", "dependencyLockDigest": digest(read(ROOT / LOCK)), "sourceFiles": len(lock["files"]), "gtaEligible": False}))
        return 0
    except (OSError, ValueError, KeyError, TypeError, zipfile.BadZipFile) as error:
        print("SAEX_SDK_REJECTED: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
