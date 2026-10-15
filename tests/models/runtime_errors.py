"""Execute actual runtime discovery under controlled dependency failures."""

import importlib.util
import os
from pathlib import Path
import sys
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
sys.modules["amal.eda.systemc"] = package
logging = types.ModuleType("amal.eda.systemc._logging")
logging.log = lambda message: None
sys.modules[logging.__name__] = logging
cppyy = types.ModuleType("cppyy")
cppyy.add_include_path = lambda path: None
cppyy.add_library_path = lambda path: None
cppyy.cppdef = lambda code: True


def load_library(path):
    """Simulate an incompatible library after successful file discovery."""
    if scenario == "abi":
        raise RuntimeError("incompatible ABI probe")


cppyy.load_library = load_library


def include(header):
    """Reject missing fmt only in its controlled discovery scenario."""
    if scenario == "fmt":
        raise RuntimeError("missing fmt")


cppyy.include = include
sys.modules["cppyy"] = cppyy
os.environ["FMT_HOME"] = "/missing-fmt-probe"
if scenario == "library":
    os.environ["SYSTEMC_LIBRARY_DIR"] = "/missing-library-probe"

original = Path.is_file


def is_file(path):
    """Hide selected dependency files while preserving real source and Python headers."""
    value = str(path)
    if scenario == "headers" and value.endswith("include/systemc"):
        return False
    return original(path)


spec = importlib.util.spec_from_file_location("amal.eda.systemc._runtime", source)
module = importlib.util.module_from_spec(spec)
with patch.object(Path, "is_file", is_file):
    try:
        spec.loader.exec_module(module)
    except RuntimeError as error:
        expected = {
            "fmt": "fmt headers not found",
            "headers": "SystemC headers not found",
            "library": "SystemC library not found",
            "abi": "incompatible ABI probe",
        }
        assert expected[scenario] in str(error), str(error)
    else:
        raise AssertionError("controlled missing dependency accepted")
print("RUNTIME_ERRORS_OK")
