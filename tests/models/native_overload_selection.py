"""Exercise the public datatype formatter's native overload fallback."""

from amal.eda.systemc import sc_dt as dt
import cppyy


numeric_format = int(dt.SC_DEC)
public_result = dt.to_string(numeric_format)
native_result = cppyy.gbl.sc_dt.to_string.__overload__("sc_dt::sc_numrep")(numeric_format)
assert public_result == native_result == b"SC_DEC", (public_result, native_result)
print("NATIVE_OVERLOAD_SELECTION_OK")
