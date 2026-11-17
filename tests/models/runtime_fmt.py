"""Exercise actual fmt discovery with isolated bundle, override and checkout trees."""

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch

root = Path(sys.argv[1])
scenario = sys.argv[2]
source = root / "src/amal/eda/systemc/_runtime.py"
for name in ("amal", "amal.eda", "amal.eda.systemc"):
    package = types.ModuleType(name)
    package.__path__ = []
    sys.modules[name] = package
logging = types.ModuleType("amal.eda.systemc._logging")
logging.log = lambda message: None
sys.modules[logging.__name__] = logging
cppyy = types.ModuleType("cppyy")
include_paths = []
libraries = []
cppyy.add_include_path = include_paths.append
cppyy.add_library_path = lambda path: None
cppyy.load_library = libraries.append
cppyy.cppdef = lambda code: True


def include(header):
    """Reject system fallback: these scenarios must select a staged fmt tree."""
    raise AssertionError(f"Unexpected system header fallback: {header}")


cppyy.include = include
bootstrap = types.ModuleType("amal.eda.systemc._bootstrap")
bootstrap.import_cppyy = lambda: cppyy
sys.modules[bootstrap.__name__] = bootstrap

with tempfile.TemporaryDirectory() as directory:
    repository = Path(directory)
    staged_source = repository / "src/amal/eda/systemc/_runtime.py"
    bundled = staged_source.parent / "_native/include"
    checkout = repository / "build/deps/fmt-11.2.0/include"
    override = repository / "user-fmt/include"
    # All candidates exist in the precedence cases. The fallback case has
    # no bundled fmt header, matching the ordinary source checkout.
    candidates = (checkout, override) if scenario == "checkout" else (bundled, checkout, override)
    for candidate in candidates:
        (candidate / "fmt").mkdir(parents=True)
        (candidate / "fmt/format.h").write_text("// fmt discovery fixture\n")
    native = repository / "systemc-install"
    (native / "include").mkdir(parents=True, exist_ok=True)
    (native / "include/systemc").write_text("// SystemC discovery fixture\n")
    (native / "lib").mkdir()
    (native / "lib/libsystemc.so").write_bytes(b"discovery-only library")
    prefix = repository / "active"
    api = prefix / "include/site" / f"python{sys.version_info.major}.{sys.version_info.minor}" / "CPyCppyy"
    api.mkdir(parents=True)
    (api / "API.h").write_text("// active environment header\n")
    env = os.environ.copy()
    for name in ("FMT_HOME", "SYSTEMC_HOME", "SYSTEMC_SOURCE_DIR", "SYSTEMC_INSTALL_DIR", "SYSTEMC_LIBRARY_DIR"):
        env.pop(name, None)
    env["SYSTEMC_INSTALL_DIR"] = str(native)
    if scenario == "override":
        env["FMT_HOME"] = str(override.parent)
    spec = importlib.util.spec_from_file_location("amal.eda.systemc._runtime", source)
    module = importlib.util.module_from_spec(spec)
    # Execute the tracked source for coverage while giving discovery a real,
    # isolated package layout; the native bridge itself remains stubbed.
    module.__file__ = str(staged_source)
    with patch.dict(os.environ, env, clear=True), patch.object(sys, "prefix", str(prefix)):
        spec.loader.exec_module(module)
        expected = {"bundled": bundled, "override": override, "checkout": checkout}[scenario]
        assert module._fmt_include == expected, module._fmt_include
        assert include_paths.count(str(expected)) == 1, include_paths
        assert all(str(path) not in include_paths for path in (bundled, checkout, override) if path != expected)
        assert libraries == [str(native / "lib/libsystemc.so")], libraries
        assert os.environ.get("FMT_HOME") == env.get("FMT_HOME")
print("RUNTIME_FMT_OK")
