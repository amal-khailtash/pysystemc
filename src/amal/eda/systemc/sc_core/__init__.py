import atexit
from enum import Enum
import pathlib
import os
import sys

import cppyy

# from .. import SYSTEMC_HOME
SYSTEMC_HOME = os.environ["SYSTEMC_HOME"]


SCRIPT_PATH = pathlib.Path(__file__).parent.absolute()


with open(SCRIPT_PATH / "sc_main.cpp", "r") as fd:
    cppyy.cppdef(fd.read())

# Reread the sc_main_main.cpp file to link out new sc_main to sc_elan_and_sim function
with open(f"{SYSTEMC_HOME}/src/sysc/kernel/sc_main_main.cpp", "r") as fd:
    cppyy.cppdef(fd.read())

# Store the sc_core.sc_module and its metaclass
# sc_module           = cppyy.gbl.sc_core.sc_module
# sc_module_metaclass = type(cppyy.gbl.sc_core.sc_module)

with open(SCRIPT_PATH / "sc_module_new.cpp", "r") as fd:
    cppyy.cppdef(fd.read())




# print(f"type(sc_core.sc_module): {type(cppyy.gbl.sc_core.sc_module)}")

# # ScModule           = cppyy.gbl.sc_core.ScModule
# # ScModule.__class__ = sc_module_metaclass

# # Replace sc_core.sc_module with the newly defined ScModule
# # Replace the C++ sc_module with the C++ wrapper ScModule

# # cppyy.gbl.sc_core.sc_module = ScModule

# cppyy.gbl.sc_core.ScModule.__release_gil__ = True
# cppyy.gbl.sc_core.ScModule.call_process.__release_gil__ = True
cppyy.gbl.sc_core.sc_start.__release_gil__ = True
cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
cppyy.gbl.sc_main.__release_gil__ = True

# Replace with custom sc_module
cppyy.gbl.sc_core.sc_module = cppyy.gbl.sc_core.ScModule
# # cppyy.gbl.sc_core.sc_module.__class__ = sc_module_metaclass
# # globals()["sc_module"] = sc_core.sc_module

# print(f"type(sc_core.sc_module): {type(cppyy.gbl.sc_core.sc_module)}")




# Access the C++ sc_core namespace
sc_core_namespace = cppyy.gbl.sc_core

def add_sc_core_members():
    module_globals = globals()
    for name in dir(sc_core_namespace):
        if name == "print":
            continue
        try:
            member = getattr(sc_core_namespace, name)
        except AttributeError:
            continue
        module_globals[name] = member

# Add all sc_core members to this module
add_sc_core_members()


# Manually add each public member of the `sc_core` namespace to the module"s globals
for name, member in sc_core_namespace.__dict__.items():
    # Filter out internal or irrelevant attributes
    if not name.startswith("__"):
        globals()[name] = member


# List of classes, enums, and functions to expose from `sc_core`
sc_core_classes = ["sc_module", "sc_module_name", "sc_clock", "sc_event", "sc_signal", "sc_in", "sc_in", "sc_out", "sc_in_clk", "sc_inout_clk", "sc_out_clk"]
sc_core_functions = ["sc_elab_and_sim"]  # Add other functions if needed


# Dynamically add classes and functions from the C++ `sc_core` namespace to this module
for name in sc_core_classes + sc_core_functions:
    try:
        globals()[name] = getattr(sc_core_namespace, name)
    except AttributeError:
        print(f"Warning: {name} not found in sc_core namespace.")





def __getattr__(name):
    mod = getattr(cppyy.gbl.sc_core, name)
    return mod


# Define a custom __dir__ for listing available attributes
def __dir__():
    return list(globals().keys())


def sc_elab_and_sim():
    print(f"[Python ] - sc_elab_and_sim({sys.argv})")

    argc = len(sys.argv)
    argv = sys.argv

    print(f"[Python ] - sc_core.sc_elab_and_sim({argc}, {argv}) ...")

    # restart
    cppyy.gbl.sc_core.sc_curr_simcontext = 0
    cppyy.gbl.sc_core.sc_get_curr_simcontext().reset()

    return cppyy.gbl.sc_core.sc_elab_and_sim(argc, argv)


def sc_stop():
    if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_STOPPED:
        print("[Python ] - sc_core.sc_stop()")
        cppyy.gbl.sc_core.sc_stop()


# Register the cleanup function to be called at exit
atexit.register(sc_stop)


class ScStatus(Enum):
    SC_UNITIALIZED               = cppyy.gbl.sc_core.SC_UNITIALIZED
    SC_ELABORATION               = cppyy.gbl.sc_core.SC_ELABORATION
    SC_BEFORE_END_OF_ELABORATION = cppyy.gbl.sc_core.SC_BEFORE_END_OF_ELABORATION
    SC_END_OF_ELABORATION        = cppyy.gbl.sc_core.SC_END_OF_ELABORATION
    SC_START_OF_SIMULATION       = cppyy.gbl.sc_core.SC_START_OF_SIMULATION
    SC_RUNNING                   = cppyy.gbl.sc_core.SC_RUNNING
    SC_PAUSED                    = cppyy.gbl.sc_core.SC_PAUSED
    SC_STOPPED                   = cppyy.gbl.sc_core.SC_STOPPED
    SC_END_OF_SIMULATION         = cppyy.gbl.sc_core.SC_END_OF_SIMULATION
    SC_END_OF_INITIALIZATION     = cppyy.gbl.sc_core.SC_END_OF_INITIALIZATION
  # SC_END_OF_EVALUATION         = cppyy.gbl.sc_core.SC_END_OF_EVALUATION
    SC_END_OF_UPDATE             = cppyy.gbl.sc_core.SC_END_OF_UPDATE
    SC_BEFORE_TIMESTEP           = cppyy.gbl.sc_core.SC_BEFORE_TIMESTEP
    SC_STATUS_LAST               = cppyy.gbl.sc_core.SC_STATUS_LAST
    SC_STATUS_ANY                = cppyy.gbl.sc_core.SC_STATUS_ANY

    @classmethod
    def from_cpp_enum(cls, cpp_enum_value):
        """Convert a SystemC sc_status int value to the corresponding ScStatus enum."""
        return ScStatus(cpp_enum_value)
        # try:
        #     return ScStatus(cpp_enum_value)
        # except ValueError:
        #     return ScStatus.SC_INVALID_STATUS  # Fallback if the value is invalid

    def __str__(self):
        """Return the string representation of the ScStatus enum."""
        return self.name


def sc_status():
    status = cppyy.gbl.sc_core.sc_status()
    return ScStatus(status)  # Convert to Python enum

