"""Verify direct public native argument-array access and entry lifetime."""

import sys
from amal.eda.systemc import sc_core as sc

saved = []
snapshots = []


def assert_outside_entry():
    """Reject native argv reads when the borrowed entry storage is unavailable."""
    try:
        sc.sc_argv()
    except RuntimeError as error:
        assert "during sc_main" in str(error)
    else:
        raise AssertionError("argv accepted outside entry")


def sc_main(args):
    """Compare original native argv with the copied Python entry arguments."""
    assert sc.sc_argc() == len(args) == len(sys.argv)
    values = sc.sc_argv()
    assert isinstance(values, tuple)
    snapshots.append(values)
    copied = [values[i] for i in range(sc.sc_argc())]
    assert copied == args == sys.argv, (copied, args, type(values))
    saved.extend(copied)
    args[1] = "modified"
    assert values[1] == "café" and sc.sc_argv()[1] == "café"
    assert sc.python_argv_at(1) == "café"
    print("ENTRY_ARGUMENT_REFERENCE", sc.sc_argc(), *copied[1:])
    return 0


assert_outside_entry()
assert sc.sc_elab_and_sim() == 0
assert_outside_entry()
assert saved[1:] == ["café", "--flag", ""]
assert snapshots[0] == tuple(saved)
