"""Verify typed dynamic-process owners across completion, error and unwind."""

import ctypes
import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

cppyy.cppdef("""
/** @brief Ordinary independent value storage for typed process results. */
struct SpawnOwnerValue { int value = -7; };
""")


def make_result(kind):
    """Create one result-storage category without callback ownership edges."""
    if kind == "scalar":
        return ctypes.c_int(-7)
    if kind == "wide":
        return dt.sc_biguint[137](7)
    if kind == "custom":
        return cppyy.gbl.SpawnOwnerValue()
    return sc.sc_time(7, sc.SC_NS)


def read_result(kind, result):
    """Read an independently owned result using its exact native category."""
    if kind in ("scalar", "custom"):
        return result.value
    if kind == "wide":
        return result.to_uint64(), bool(result[136])
    return str(result)


def returned_value(kind):
    """Create a fresh native return value for the completion path."""
    if kind == "scalar":
        return 41
    if kind == "wide":
        value = dt.sc_biguint[137](41)
        value[136] = True
        return value
    if kind == "custom":
        value = cppyy.gbl.SpawnOwnerValue()
        value.value = 41
        return value
    return sc.sc_time(41, sc.SC_NS)


def check_owners(mode):
    """Drop caller references and check the documented kernel owner boundary."""
    kinds = ("scalar",) if mode == "error" else ("scalar", "wide", "custom", "time")
    starts, cleanup = [], []
    handles, owners, sentinels = {}, {}, {}

    class Callback:
        """Own callback state independently of caller-owned result storage."""

        def __init__(self, kind):
            """Store only a native result-category name and no result proxy."""
            self.kind = kind

        def __call__(self):
            """Return a value, fail, or unwind while retaining native storage."""
            starts.append(self.kind)
            if mode == "error":
                raise ValueError("typed owner callback failure")
            if mode == "unwind":
                try:
                    sc.wait(100, sc.SC_NS)
                finally:
                    cleanup.append((self.kind, str(sc.sc_time_stamp())))
            return returned_value(self.kind)

    for kind in kinds:
        result = make_result(kind)
        callback = Callback(kind)
        sentinels[kind] = read_result(kind, result)
        owners[kind] = weakref.ref(result), weakref.ref(callback)
        handles[kind] = sc.sc_spawn(result, callback, f"owner_{kind}")
        del result, callback
    gc.collect()
    assert all(result() is not None and callback() is not None for result, callback in owners.values())

    if mode == "unwind":

        def controller():
            """Reset then kill each result process while its call frame exists."""
            sc.wait(2, sc.SC_NS)
            for handle in handles.values():
                handle.reset()
            sc.wait(1, sc.SC_NS)
            for handle in handles.values():
                handle.kill()

        control = sc.sc_spawn(controller, "owner_control")
        sc.sc_start(4, sc.SC_NS)
        assert control.terminated()
        assert sorted(cleanup) == sorted((kind, time) for kind in kinds for time in ("2 ns", "3 ns"))
        assert sorted(starts) == sorted((*kinds, *kinds))
    elif mode == "error":
        try:
            sc.sc_start(1, sc.SC_NS)
        except sc.sc_report as error:
            assert error.get_id() == 549 and "typed owner callback failure" in error.get_msg()
        else:
            raise AssertionError("typed process error did not propagate")
    else:
        sc.sc_start(1, sc.SC_NS)
        assert starts == list(kinds)

    for kind, (result, callback) in owners.items():
        assert result() is not None and callback() is not None
        expected = (
            {
                "scalar": 41,
                "wide": (41, True),
                "custom": 41,
                "time": "41 ns",
            }[kind]
            if mode == "normal"
            else sentinels[kind]
        )
        assert read_result(kind, result()) == expected
        if mode != "error":
            assert handles[kind].terminated()
    sc.sc_stop()
    handles.clear()
    gc.collect()
    assert all(result() is not None and callback() is not None for result, callback in owners.values())
    print(f"SPAWN_OWNER_LIFETIME_OK mode={mode} categories={len(kinds)} retained_after_stop=1")


if __name__ == "__main__":
    check_owners(sys.argv[1])
