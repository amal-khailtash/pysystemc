"""Configure and load the pinned native runtime once per interpreter.

Discovery preserves user overrides and prefers bundled wheel resources. This
module owns CPyCppyy header repair and library loading; modeling submodules use
its initialized cppyy namespace instead of loading libraries independently.
"""

import os
import sys
import pathlib
import sysconfig
import shutil
from importlib.metadata import files as _distribution_files
from ._logging import log

# Some CPyCppyy distributions install lowercase headers although generated
# bindings include CPyCppyy/API.h. Repair only the active environment.
_api_parent = pathlib.Path(sys.prefix) / "include" / "site" / f"python{sys.version_info.major}.{sys.version_info.minor}"
_api_alias = _api_parent / "CPyCppyy"
if not (_api_alias / "API.h").is_file() and not (_api_parent / "cpycppyy/API.h").is_file():
    # uv can retain wheel data headers in its cache rather than sys.prefix.
    # Copy into this environment so generated uppercase includes work without
    # modifying shared cache contents.
    for _header in _distribution_files("CPyCppyy") or ():
        if _header.name == "API.h":
            _header_dir = pathlib.Path(_header.locate()).parent
            if (_header_dir / "API.h").is_file():
                shutil.copytree(_header_dir, _api_alias, dirs_exist_ok=True)
                break
if (_api_parent / "cpycppyy" / "API.h").is_file() and not _api_alias.exists():
    try:
        _api_alias.symlink_to("cpycppyy", target_is_directory=True)
    except OSError as error:
        raise RuntimeError("CPyCppyy headers need repair; run make fix-cppyy in your environment") from error

# uv may load cppyy from a cache outside sys.prefix. Select the active
# environment's header parent before cppyy performs its import-time lookup.
# cppyy expects this parent directory, despite its diagnostic wording.
if (_api_alias / "API.h").is_file():
    os.environ.setdefault("CPPYY_API_PATH", str(_api_parent))

import cppyy

cppyy.add_include_path(sysconfig.get_path("include"))
if (_api_alias / "API.h").is_file():
    cppyy.add_include_path(str(_api_parent))
    # cppyy generates dispatcher includes in later interpreter transactions.
    # Suppress only the case-path diagnostic caused by this compatibility
    # alias; other compiler warnings and errors remain enabled.
    cppyy.cppdef("""
    #pragma clang diagnostic ignored "-Wnonportable-include-path"
    """)

# fmt is header-only in the JIT bridge; no extra shared-library ABI is needed.
_bundled = pathlib.Path(__file__).parent / "_native"
_fmt_home = os.environ.get("FMT_HOME")
_fmt_include = (
    pathlib.Path(_fmt_home) / "include"
    if _fmt_home
    else pathlib.Path(__file__).resolve().parents[4] / "build/deps/fmt-11.2.0/include"
)
if not _fmt_home and (_bundled / "include/fmt/format.h").is_file():
    _fmt_include = _bundled / "include"
if (_fmt_include / "fmt/format.h").is_file():
    cppyy.add_include_path(str(_fmt_include))
else:
    # A system installation may already be visible to Cling.
    try:
        cppyy.include("fmt/format.h")
    except Exception as error:
        raise RuntimeError(
            "fmt headers not found: run make setup-fmt or set FMT_HOME to an installed fmt prefix"
        ) from error
cppyy.add_include_path(str(pathlib.Path(__file__).parent / "sc_core"))

# Both native builders install to the same prefix. Explicit overrides remain
# available, but ordinary checkout use requires no library-location settings.
_repository = pathlib.Path(__file__).resolve().parents[4]
SYSTEMC_HOME = str(
    pathlib.Path(
        os.environ.get(
            "SYSTEMC_SOURCE_DIR",
            os.environ.get(
                "SYSTEMC_HOME", str(_bundled if (_bundled / "src").exists() else _repository / "submodules/systemc")
            ),
        )
    )
)
_install = pathlib.Path(os.environ.get("SYSTEMC_INSTALL_DIR", str(_repository / "build/deps/systemc-install")))
_prefixes = ([_install] if "SYSTEMC_INSTALL_DIR" in os.environ else []) + [
    _bundled,
    _install,
    pathlib.Path(SYSTEMC_HOME),
]
_headers = next((prefix / "include" for prefix in _prefixes if (prefix / "include/systemc").is_file()), None)
if _headers is None:
    raise RuntimeError("SystemC headers not found; run make setup-systemc or make setup-systemc-cmake")
cppyy.add_include_path(str(_headers))
_library_override = os.environ.get("SYSTEMC_LIBRARY_DIR")
_library_dirs = (
    [pathlib.Path(_library_override)]
    if _library_override
    else [prefix / name for prefix in _prefixes for name in ("lib", "lib64", "lib-linux64")]
)
_library = next(
    (directory / "libsystemc.so" for directory in _library_dirs if (directory / "libsystemc.so").is_file()), None
)
if _library is None:
    raise RuntimeError("SystemC library not found; run make setup-systemc or make setup-systemc-cmake")
SYSTEMC_LIBRARY_DIR = str(_library.parent)
cppyy.add_library_path(SYSTEMC_LIBRARY_DIR)
cppyy.load_library(str(_library))
log(f"Loaded SystemC library {_library}")
