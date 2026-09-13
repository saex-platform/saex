"""Actual CMake configure rejection tests; never mutate the shared source cache."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("saex_dependency", ROOT / "tools/native/dependency.py")
dep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dep)
python = sys.argv[1] if len(sys.argv) == 2 else sys.executable


def rejected(source: Path, build: Path, architecture: str, expected: str):
    result = subprocess.run(["cmake", "-S", str(source), "-B", str(build), "-G", "Visual Studio 17 2022", "-A", architecture, "-DPython3_EXECUTABLE=" + python],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60, encoding="utf-8", errors="replace")
    if result.returncode == 0 or expected not in result.stdout:
        raise RuntimeError("expected " + expected + ": " + result.stdout[-6000:])
    print("PASS configure rejects " + expected)


with tempfile.TemporaryDirectory(prefix="saex-sdk-configure-") as temp:
    root = Path(temp)
    rejected(ROOT / "tools/native", root / "x64-build", "x64", "SAEX_SDK_X86_ONLY")
    lock = dep.load_lock(ROOT)
    replica = root / "missing-source"
    for entry in [*lock["recipeFiles"], *lock["patches"], {"path": dep.LOCK.as_posix()}]:
        path = dep.safe_path(replica, entry["path"])
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / entry["path"], path)
    rejected(replica / "tools/native", root / "missing-build", "Win32", "missing_or_linked_source")
print("2/2 SDK configure rejection tests passed")
