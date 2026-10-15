"""Audit entry status conversion, argument ownership and repeated error cleanup."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy


class Entry:
    """Callable whose lifetime exposes leaked native references."""

    def __init__(self, result):
        """Store the return value for native status conversion."""
        self.result = result

    def __call__(self, args):
        """Validate argument decoding and return the configured status."""
        assert args == ["entry", "café"]
        return self.result


for value, expected in [(0, 0), (-7, -7), (2**31 - 1, 2**31 - 1), (2**31, -1), (2**100, -1), (None, -1)]:
    callback = Entry(value)
    reference = weakref.ref(callback)
    sys.modules["__main__"].sc_main = callback
    assert cppyy.gbl.sc_main(2, ["entry", "café"]) == expected
    del sys.modules["__main__"].sc_main
    del callback
    gc.collect()
    assert reference() is None, "entry bridge leaked its callable"

sys.modules["__main__"].sc_main = 42
assert cppyy.gbl.sc_main(0, []) == -1
assert sc.sc_get_status() == sc.SC_ELABORATION
print("ENTRY_ERRORS_OK")
