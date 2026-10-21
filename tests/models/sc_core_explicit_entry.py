"""Verify explicit native entry arguments and pre-entry validation recovery."""

import sys
from amal.eda.systemc import sc_core as sc

original = list(sys.argv)
called = []
zero = len(sys.argv) > 1 and sys.argv[1] == "zero"


def sc_main(args):
    """Check selected argument count, copies and original sys.argv preservation."""
    if zero:
        assert args == [] and sc.sc_argc() == 0 and sc.sc_argv() == ()
        called.append(True)
        print("ZERO_ENTRY_REFERENCE 0")
        return 0
    assert args == ["explicit", "café", "--flag", ""]
    assert sc.sc_argc() == 4
    assert sc.sc_argv() == tuple(args)
    args[0] = "changed"
    assert sc.sc_argv()[0] == "explicit"
    assert sys.argv == original
    called.append(True)
    print("EXPLICIT_ENTRY_REFERENCE 4 café --flag ")
    return 7


for arguments, error_type in (
    ((-1, []), ValueError),
    ((2, ["one"]), ValueError),
    ((2**31, []), ValueError),
    ((True, []), TypeError),
    ((1.5, []), TypeError),
    ((1, "word"), TypeError),
    ((1, [None]), TypeError),
    ((1, ["bad\0value"]), ValueError),
    ((1,), TypeError),
    ((None, []), TypeError),
):
    try:
        sc.sc_elab_and_sim(*arguments)
    except error_type:
        pass
    else:
        raise AssertionError(f"invalid entry accepted: {arguments!r}")
    assert called == [] and sc.sc_get_status() == sc.SC_ELABORATION
if zero:
    assert sc.sc_elab_and_sim(0, []) == 0
else:
    assert sc.sc_elab_and_sim(4, ("explicit", "café", "--flag", "", object())) == 7
assert called == [True] and sys.argv == original
try:
    sc.sc_argv()
except RuntimeError:
    pass
else:
    raise AssertionError("argv accepted after entry")
