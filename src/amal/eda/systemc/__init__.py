import os
import sys
import pathlib
import sysconfig
from importlib.metadata import version as _package_version
from ._logging import log, show_banner

__version__ = _package_version("systemc")
show_banner(__version__)

cppv = 17
# cppv = 14
# os.environ["CLING_STANDARD_PCH"] = os.path.expanduser(os.path.join("~", f"cling_standard_native_{cppv}.pch"))
os.environ.setdefault("EXTRA_CLING_ARGS", f"-std=c++{cppv}")
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -D_LIBCPP_LFTS_STRING_VIEW"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DRWrap_libcpp_string_view_h"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DR__HAS_STD_EXPERIMENTAL_STRING_VIEW"

# os.environ["CLING_REBUILD_PCH"] = "1"

# Some CPyCppyy distributions install lowercase headers although generated
# bindings include CPyCppyy/API.h. Repair only the active environment.
_api_parent = pathlib.Path(sys.prefix) / "include" / "site" / f"python{sys.version_info.major}.{sys.version_info.minor}"
_api_alias = _api_parent / "CPyCppyy"
if (_api_parent / "cpycppyy" / "API.h").is_file() and not _api_alias.exists():
    try:
        _api_alias.symlink_to("cpycppyy", target_is_directory=True)
    except OSError as error:
        raise RuntimeError("CPyCppyy headers need repair; run make fix-cppyy in your environment") from error

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
_fmt_home = os.environ.get("FMT_HOME")
_fmt_include = (pathlib.Path(_fmt_home) / "include" if _fmt_home else
                pathlib.Path(__file__).resolve().parents[4] / "build/deps/fmt-11.2.0/include")
if (_fmt_include / "fmt/format.h").is_file():
    cppyy.add_include_path(str(_fmt_include))
else:
    # A system installation may already be visible to Cling.
    try:
        cppyy.include("fmt/format.h")
    except Exception as error:
        raise RuntimeError("fmt headers not found: run make setup-fmt or set FMT_HOME to an installed fmt prefix") from error
cppyy.add_include_path(str(pathlib.Path(__file__).parent / "sc_core"))

# Both native builders install to the same prefix. Explicit overrides remain
# available, but ordinary checkout use requires no library-location settings.
_repository = pathlib.Path(__file__).resolve().parents[4]
SYSTEMC_HOME = str(pathlib.Path(os.environ.get("SYSTEMC_SOURCE_DIR",
    os.environ.get("SYSTEMC_HOME", str(_repository / "submodules/systemc")))))
_install = pathlib.Path(os.environ.get("SYSTEMC_INSTALL_DIR",
    str(_repository / "build/deps/systemc-install")))
_prefixes = [_install, pathlib.Path(SYSTEMC_HOME)]
_headers = next((prefix / "include" for prefix in _prefixes
                 if (prefix / "include/systemc").is_file()), None)
if _headers is None:
    raise RuntimeError("SystemC headers not found; run make setup-systemc or make setup-systemc-cmake")
cppyy.add_include_path(str(_headers))
_library_override = os.environ.get("SYSTEMC_LIBRARY_DIR")
_library_dirs = ([pathlib.Path(_library_override)] if _library_override else
                 [prefix / name for prefix in _prefixes
                  for name in ("lib", "lib64", "lib-linux64")])
_library = next((directory / "libsystemc.so" for directory in _library_dirs
                 if (directory / "libsystemc.so").is_file()), None)
if _library is None:
    raise RuntimeError("SystemC library not found; run make setup-systemc or make setup-systemc-cmake")
SYSTEMC_LIBRARY_DIR = str(_library.parent)
cppyy.add_library_path(SYSTEMC_LIBRARY_DIR)
cppyy.load_library(str(_library))
log(f"Loaded SystemC library {_library}")

cppyy.cppdef("""
// #define SC_CPLUSPLUS 201402L /* std=c++14 */
   #define SC_CPLUSPLUS 201703L /* std=c++17 */
""")

# cppyy.include("tlm.h")
# cppyy.include("systemc.h")
cppyy.cppdef("#define SC_INCLUDE_DYNAMIC_PROCESSES")
cppyy.include("systemc")
# cppyy-generated port specializations use unqualified policy enum names.
cppyy.cppdef("using sc_core::SC_ONE_OR_MORE_BOUND; using sc_core::SC_ZERO_OR_MORE_BOUND; using sc_core::SC_ALL_BOUND;")

from .sc_core import *
from .sc_dt import *



def print_module_hierarchy(obj: sc_core.sc_object, level: int = 0, all_modules: bool=False):
    """Function to recursively print the hierarchy"""

    # print(f"kind     : {obj.kind()}")
    # print(f"name     : {obj.name()}")
    # print(f"basename : {obj.basename()}")

    if obj.kind() != "sc_module":
        return

    mod_name = obj.name()  # cppyy.gbl.boost.core.demangle(type(obj).__name__)
    inst_name = obj.basename()
    is_mod_stratus = mod_name.startswith("cynw::")

    # print(f"mod_name: {mod_name}, inst_name: {inst_name}, is_mod_stratus: {is_mod_stratus}")

    if not all_modules and is_mod_stratus:
        return

    print("  " * level + f"{mod_name:<50} ({inst_name})")

    for child in obj.get_child_objects():
        print_module_hierarchy(child, level + 1, all_modules)



def print_hierarchy(all_modules: bool=False):
    print("=" * 80)
    print("Design Hierarchy:")

    simcontext = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    print(cppyy.gbl.sc_core.sc_get_status())
    # print("1")
    top_level_objects = cppyy.gbl.sc_core.sc_get_top_level_objects(simcontext)
    # top_level_objects = cppyy.gbl.sc_core.sc_get_top_level_objects()
    # print(type(top_level_objects))
    print(top_level_objects.size())
    # print(dir(top_level_objects))
    # print(f"Len: {len(top_level_objects[0])}")
    for obj in top_level_objects:
        print_module_hierarchy(obj, 1, all_modules)

    print("=" * 80)


def main():
    log(f"SystemC {sc_core.sc_release()}; main({sys.argv})")
    try:
        result = sc_core.sc_elab_and_sim()

        if result != 0:
            log(f"sc_main returned status {result}", error=True)
            sys.exit(result)
    except Exception as e:
        log(f"Exception while executing sc_main: {e}", error=True)
        sys.exit(1)  # Exit with a general error code


all = (
    "SYSTEMC_HOME",
    "main",
    "sc_main",
    "sc_start",
    "sc_stop",
    "sc_time_stamp",
    "print_hierarchy",
)
