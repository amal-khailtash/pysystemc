import os
import sys

cppv = 17
# cppv = 14
# os.environ["CLING_STANDARD_PCH"] = os.path.expanduser(os.path.join("~", f"cling_standard_native_{cppv}.pch"))
os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv}"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -D_LIBCPP_LFTS_STRING_VIEW"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DRWrap_libcpp_string_view_h"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DR__HAS_STD_EXPERIMENTAL_STRING_VIEW"

# os.environ["CLING_REBUILD_PCH"] = "1"

import cppyy

SYSTEMC_HOME = os.environ["SYSTEMC_HOME"]

# os.environ["SYSTEMC_DISABLE_COPYRIGHT_MESSAGE"] = "1"        # Disables SystemC Copyright Message
# os.environ["SC_COPYRIGHT_MESSAGE"]              = "DISABLE"  # Disables SystemC Copyright Message
# os.environ["SC_DEPRECATION_WARNINGS"]           = "DISABLE"  # Do not issue warnings about using deprecated features as of IEEE Std. 1666-2011.
# os.environ["SC_VCD_SCOPES"]                     = "DISABLE"  # Disable grouping of VCD trace variables in hierarchical scopes by default.


cppyy.add_include_path(f"{SYSTEMC_HOME}/include")
cppyy.add_library_path(f"{SYSTEMC_HOME}/lib-linux64")

cppyy.load_library("systemc")
cppyy.load_library(f"{SYSTEMC_HOME}/lib-linux64/libsystemc.so")
# cppyy.load_reflection_info(f"{SYSTEMC_HOME}/lib-linux64/libsystemc.so")

cppyy.cppdef("""
// #define SC_CPLUSPLUS 201402L /* std=c++14 */
   #define SC_CPLUSPLUS 201703L /* std=c++17 */
""")

# cppyy.include("tlm.h")
# cppyy.include("systemc.h")
cppyy.include("systemc")

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
        print_module_hierarchy(child, level + 1, all)



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
    print("-" * 80)
    print(f"[Python ] - SYSTEMC_HOME: {SYSTEMC_HOME}")
    print(f"[Python ] - SC_VERSION  : {sc_core.sc_release()}")
    print(f"[Python ] - main({sys.argv})")
    try:
        result = sc_core.sc_elab_and_sim()

        if result != 0:
            print("Error: sc_main returned an error code:", result)
            sys.exit(result)
    except Exception as e:
        print("Caught exception while executing sc_main:", e)
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