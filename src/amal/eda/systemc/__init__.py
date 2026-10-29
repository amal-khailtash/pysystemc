"""Initialize native SystemC and expose Python modeling namespaces.

The kernel has one simulation context per interpreter. Retain modules, channels
and callback dependencies until simulation ends; use subprocesses for unrelated
simulations. Native templates are instantiated by cppyy on demand.
"""

import os
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
    """Print a native module subtree using the full SystemC object hierarchy.

    Args:
        obj: Borrowed native SystemC object whose module subtree is inspected.
            Non-module objects produce no output.
        level: Nonnegative two-space indentation depth for the root module.
        all_modules: Whether to include ``cynw::`` modules and their subtrees.

    Returns:
        None. The returned native text is written to the current Python stdout.

    Raises:
        TypeError: If ``obj`` is not a native ``sc_object``, ``level`` is not an
            integer, or ``all_modules`` is not a boolean.
        ValueError: If ``level`` is negative.

    The native traversal borrows the object tree for this call and retains no
    modules or callbacks. Call on the simulation thread while the hierarchy is
    stable. The Python print preserves ``redirect_stdout`` and flushes text
    before subsequent native scheduler output.
    """
    if not isinstance(obj, sc_core.sc_object):
        raise TypeError("obj must be a native SystemC sc_object")
    if isinstance(level, bool) or not isinstance(level, int):
        raise TypeError("level must be an integer")
    if level < 0:
        raise ValueError("level must be nonnegative")
    if not isinstance(all_modules, bool):
        raise TypeError("all_modules must be a bool")

    text = _decode_hierarchy_text(cppyy.gbl.pysystemc_public.module_hierarchy_text(obj, level, all_modules))
    print(text, end="", flush=True)


def print_hierarchy(all_modules: bool = False) -> None:
    """Print native module hierarchy text for the current kernel context.

    Args:
        all_modules: Whether to include ``cynw::`` modules and their subtrees.

    Returns:
        None. Native plain text is written to the current Python stdout.

    Raises:
        TypeError: If ``all_modules`` is not a boolean.

    The native traversal borrows the object tree for this call and retains no
    modules or callbacks. Call on the simulation thread while the hierarchy is
    stable. Printing the returned native string through Python preserves
    ``redirect_stdout`` and flushes text before subsequent native scheduler
    output.
    """
    if not isinstance(all_modules, bool):
        raise TypeError("all_modules must be a bool")

    text = _decode_hierarchy_text(cppyy.gbl.pysystemc_public.hierarchy_text(all_modules))
    print(text, end="", flush=True)


def sc_main(argc=None, argv=None) -> int:
    """Invoke the global native sc_main bridge directly.

    Args:
        argc: Native argument count; omit both arguments to use sys.argv.
        argv: List/tuple of strings with at least argc entries; selected entries
            are copied and retained until the native call returns.

    Returns:
        Callback integer status, or -1 after reporting a Python callback error.

    Raises:
        TypeError: Argument count/array types are invalid or incomplete.
        ValueError: Count is outside native bounds or an argument contains NUL.

    Define ``__main__.sc_main(argv)`` before invocation. The bridge releases GIL
    for native execution and acquires it for Python dispatch, releasing temporary
    references on every exit. Direct entry does not establish the sc_argc/sc_argv
    snapshot or catch escaping native model exceptions; use sc_elab_and_sim for
    that wrapper lifecycle. Invoke once per independent simulation process.
    """
    from .sc_core import _prepare_entry_arguments

    argc, argv = _prepare_entry_arguments(argc, argv)
    return cppyy.gbl.sc_main(argc, argv)


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


def _decode_hierarchy_text(text: str | bytes) -> str:
    """Convert cppyy's owned native hierarchy string into Python text."""
    return text.decode("utf-8") if isinstance(text, bytes) else str(text)


sc_elab_and_sim = sc_core.sc_elab_and_sim

all = (
    "SYSTEMC_HOME",
    "main",
    "sc_main",
    "sc_elab_and_sim",
    "sc_start",
    "sc_stop",
    "sc_time_stamp",
    "print_hierarchy",
)
