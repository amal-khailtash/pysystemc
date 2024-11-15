import cppyy

# Access the C++ sc_dt namespace
sc_dt_namespace = cppyy.gbl.sc_dt

# Helper function to add all items from `sc_dt` to this module
def add_sc_dt_members():
    module_globals = globals()
    for name in dir(sc_dt_namespace):
        try:
            member = getattr(sc_dt_namespace, name)
        except AttributeError:
            continue
        module_globals[name] = member

# Add all sc_dt members to this module
add_sc_dt_members()


# Manually add each public member of the `sc_core` namespace to the module"s globals
for name, member in sc_dt_namespace.__dict__.items():
    # Filter out internal or irrelevant attributes
    if not name.startswith("__"):
        globals()[name] = member


# List of classes, enums, and functions to expose from `sc_dt`
sc_dt_classes = ["sc_uint", "sc_int", "sc_bigint", "sc_biguint"]
sc_dt_functions = []  # Add functions here if needed

# Dynamically add classes and functions from the C++ `sc_dt` namespace to this module
for name in sc_dt_classes + sc_dt_functions:
    try:
        globals()[name] = getattr(sc_dt_namespace, name)
    except AttributeError:
        print(f"Warning: {name} not found in sc_dt namespace.")


def __getattr__(name):
    mod = getattr(cppyy.gbl.sc_dt, name)
    return mod

# Define a custom __dir__ for listing available attributes
def __dir__():
    return list(globals().keys())


# __all__ = dir(sc_dt_namespace)



# import sys

# import cppyy


# entity = getattr(cppyy.gbl, "sc_dt")
# if getattr(entity, "__module__", None) == "cppyy.gbl":
#     setattr(entity, "__module__", "sc_dt")
# setattr(sys.modules["SystemC"], "sc_dt", entity)


# sc_uint = cppyy.gbl.sc_dt.sc_uint
# sc_int  = cppyy.gbl.sc_dt.sc_int


# sc_dt_namespace = cppyy.gbl.sc_dt


# for name in dir(sc_dt_namespace):
#     if hasattr(sc_dt_namespace, name):  # Check if it"s actually accessible
#         # setattr(cppyy.gbl.sc_core, name, getattr(sc_dt_namespace, name))
#         try:
#             globals().update({name: getattr(sc_dt_namespace, name)})
#         except TypeError:
#             pass
#             # print(f"Attribute {name} could not be accessed and will be skipped.")
#     else:
#         pass
#         # print(f"Attribute {name} could not be accessed and will be skipped.")
