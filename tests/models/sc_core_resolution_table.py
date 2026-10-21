"""Verify all native logic-resolution table cells and copied global reads."""

from amal.eda.systemc import sc_core as sc
import cppyy

# Native table storage is read-only and library-owned; never mutate or delete it.
for left in range(4):
    for right in range(4):
        value = sc.sc_logic_resolution_tbl[left][right]
        assert int(value) in range(4)
        assert value == sc.sc_logic_resolution_tbl[right][left]
        print("RESOLUTION_CELL", left, right, int(value))
assert sc.sc_allow_process_control_corners == cppyy.gbl.sc_core.sc_allow_process_control_corners
assert sc.sc_enable_name_checking == cppyy.gbl.sc_core.sc_enable_name_checking
print("CORE_GLOBAL_READS", int(sc.sc_allow_process_control_corners), int(sc.sc_enable_name_checking))
