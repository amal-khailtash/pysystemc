"""Validate active-environment header repair without touching shared caches."""

import importlib.util
from importlib.metadata import PackagePath
import os
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch

root = Path(sys.argv[1])
scenario = sys.argv[2]
source = root / "src/amal/eda/systemc/_runtime.py"
for parent_name in ("amal", "amal.eda"):
    parent = types.ModuleType(parent_name)
    parent.__path__ = []
    sys.modules[parent_name] = parent
package = types.ModuleType("amal.eda.systemc")
package.__path__ = []
sys.modules[package.__name__] = package
logging = types.ModuleType("amal.eda.systemc._logging")
logging.log = lambda message: None
sys.modules[logging.__name__] = logging
cppyy = types.ModuleType("cppyy")
paths = []
cppyy.add_include_path = paths.append
cppyy.add_library_path = lambda path: None
cppyy.cppdef = lambda code: True
cppyy.include = lambda header: None
cppyy.load_library = lambda path: None
sys.modules["cppyy"] = cppyy
os.environ["CPPYY_API_PATH"] = "preserved-user-setting"

with tempfile.TemporaryDirectory() as directory:
    prefix = Path(directory) / "active"
    parent = prefix / "include/site" / f"python{sys.version_info.major}.{sys.version_info.minor}"
    cache = Path(directory) / "cache"
    cache.mkdir()
    (cache / "API.h").write_text("header-marker")
    if scenario in ("alias", "failure"):
        lower = parent / "cpycppyy"
        lower.mkdir(parents=True)
        (lower / "API.h").write_text("header-marker")
    header = PackagePath("API.h")
    header.dist = types.SimpleNamespace(locate_file=lambda value: cache / str(value))
    spec = importlib.util.spec_from_file_location("amal.eda.systemc._runtime", source)
    module = importlib.util.module_from_spec(spec)
    with patch.object(sys, "prefix", str(prefix)), patch("importlib.metadata.files", return_value=[header]):
        if scenario == "failure":
            with patch.object(Path, "symlink_to", side_effect=OSError("denied")):
                try:
                    spec.loader.exec_module(module)
                except RuntimeError as error:
                    assert "make fix-cppyy" in str(error)
                else:
                    raise AssertionError("repair failure accepted")
        else:
            spec.loader.exec_module(module)
            alias = parent / "CPyCppyy"
            assert (alias / "API.h").read_text() == "header-marker"
            assert alias.is_symlink() == (scenario == "alias")
            assert str(parent) in paths
    assert (cache / "API.h").read_text() == "header-marker"
    assert os.environ["CPPYY_API_PATH"] == "preserved-user-setting"
print("RUNTIME_REPAIR_OK")
