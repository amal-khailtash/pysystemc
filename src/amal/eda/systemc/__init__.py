"""Initialize native SystemC and expose Python modeling namespaces.

The kernel has one simulation context per interpreter. Retain modules, channels
and callback dependencies until simulation ends; use subprocesses for unrelated
simulations. Native templates are instantiated by cppyy on demand.
"""

import os
import builtins as _builtins
import sys
from importlib.metadata import version as _package_version
from ._logging import log, show_banner

__version__ = _package_version("pysystemc")
show_banner(__version__)

cppv = 17
# cppv = 14
# os.environ["CLING_STANDARD_PCH"] = os.path.expanduser(os.path.join("~", f"cling_standard_native_{cppv}.pch"))
os.environ.setdefault("EXTRA_CLING_ARGS", f"-std=c++{cppv}")
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -D_LIBCPP_LFTS_STRING_VIEW"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DRWrap_libcpp_string_view_h"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DR__HAS_STD_EXPERIMENTAL_STRING_VIEW"

# os.environ["CLING_REBUILD_PCH"] = "1"

# Native discovery and loading have one owner; importing again is idempotent.
from ._runtime import cppyy, SYSTEMC_HOME as SYSTEMC_HOME, SYSTEMC_LIBRARY_DIR as SYSTEMC_LIBRARY_DIR

cppyy.cppdef("""
// #define SC_CPLUSPLUS 201402L /* std=c++14 */
   #define SC_CPLUSPLUS 201703L /* std=c++17 */
""")

# cppyy.include("tlm.h")
# cppyy.include("systemc.h")
cppyy.cppdef("#define SC_INCLUDE_DYNAMIC_PROCESSES")
cppyy.cppdef("#define SC_INCLUDE_FX")
cppyy.include("systemc")
# Public tracing report identifiers are not included by the umbrella header.
cppyy.include("sysc/tracing/sc_tracing_ids.h")
# The umbrella declares the name generator but omits its public definition.
cppyy.include("sysc/kernel/sc_name_gen.h")
cppyy.include("tlm")
# cppyy renders enum template arguments without their namespace qualifier.
# Make fixed-point policies visible to Cling's generated specializations.
cppyy.cppdef("""
using sc_dt::SC_RND; using sc_dt::SC_RND_ZERO; using sc_dt::SC_RND_MIN_INF;
using sc_dt::SC_RND_INF; using sc_dt::SC_RND_CONV; using sc_dt::SC_TRN;
using sc_dt::SC_TRN_ZERO; using sc_dt::SC_SAT; using sc_dt::SC_SAT_ZERO;
using sc_dt::SC_SAT_SYM; using sc_dt::SC_WRAP; using sc_dt::SC_WRAP_SM;
""")
# cppyy-generated port specializations use unqualified policy enum names.
cppyy.cppdef("using sc_core::SC_ONE_OR_MORE_BOUND; using sc_core::SC_ZERO_OR_MORE_BOUND; using sc_core::SC_ALL_BOUND;")
cppyy.cppdef("using sc_core::SC_ONE_WRITER; using sc_core::SC_MANY_WRITERS; using sc_core::SC_UNCHECKED_WRITERS;")

from .sc_core import *
from .sc_dt import *
from . import tlm as tlm


def print_module_hierarchy(obj: sc_core.sc_object, level: int = 0, all_modules: bool = False) -> None:
    """Print a module and recursively visit its child modules.

    Args:
        obj: Native object whose module hierarchy is inspected.
        level: Initial indentation depth.
        all_modules: Include modules with the internal cynw prefix when true.
    """

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

    children = obj.get_child_objects()
    for index in _builtins.range(children.size()):
        print_module_hierarchy(children[index], level + 1, all_modules)


def print_hierarchy(all_modules: bool = False) -> None:
    """Print top-level modules and descendants in the current kernel context.

    Args:
        all_modules: Include internal cynw modules when true.
    """
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
    # Indexing avoids cppyy iterator conversion on an empty native vector.
    for index in _builtins.range(top_level_objects.size()):
        print_module_hierarchy(top_level_objects[index], 1, all_modules)

    print("=" * 80)


def main() -> None:
    """Invoke __main__.sc_main(argv) through the native SystemC entry point.

    The callable must return an integer status. A nonzero status or exception
    exits with SystemExit. Call once per interpreter; use a fresh subprocess
    for independent simulations. The bridge acquires the GIL for Python entry.
    """
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
