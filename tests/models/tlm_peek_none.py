"""Check blocking TLM peek None, tag, null, output and error behavior."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt, tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_peek_none_probe.h"))
native = cppyy.gbl.tlm_peek_none


def borrowed_view(owner, value_type):
    """Bind a non-owning Python view to the adjusted combined-interface base.

    Args:
        owner: Live native probe that owns the sample and counters.
        value_type: Native transaction type used by the interface template.

    Returns:
        A borrowed ``tlm_blocking_get_peek_if`` view. Keep ``owner`` alive for
        every use of the returned proxy.
    """
    interface_type = tlm.tlm_blocking_get_peek_if[value_type]
    address = native.combined_interface_address[value_type](owner)
    view = cppyy.bind_object(address, interface_type)
    view.__python_owns__ = False
    return view


def wide_parts(value):
    """Return the low word and bit 256 of a borrowed 257-bit value."""
    return int(value.to_uint64()), int(bool(value[256]))


def check_malformed(view, owner, tag):
    """Require two tag arguments to fail before entering native peek."""
    before = int(owner.peek_calls())
    try:
        view.peek(tag, tag)
    except TypeError:
        pass
    else:
        raise AssertionError("peek accepted two tag arguments")
    after = int(owner.peek_calls())
    assert after == before, (before, after)
    return before == after


int_owner = native.PeekNoneProbe[int](23)
int_view = borrowed_view(int_owner, int)
int_tag = tlm.tlm_tag[int]()
int_expected = (23, 23, 23, 23, 23)
int_results = (
    int(int_view.peek()),
    int(int_view.peek(None)),
    int(int_view.peek(cppyy.nullptr)),
    int(int_view.peek(int_tag)),
    int(native.typed_nullptr_peek[int](int_view)),
)
assert int_results == int_expected, int_results
int_no_call = check_malformed(int_view, int_owner, int_tag)
assert int(int_owner.value()) == 23
assert int(int_owner.peek_calls()) == 5
assert int(int_owner.get_calls()) == 0

wide_type = sc_dt.sc_biguint[257]
wide_expected = wide_type(str((1 << 256) + 7))
wide_owner = native.PeekNoneProbe[wide_type](wide_expected)
wide_view = borrowed_view(wide_owner, wide_type)
wide_tag = tlm.tlm_tag[wide_type]()
wide_results = (
    wide_view.peek(),
    wide_view.peek(None),
    wide_view.peek(cppyy.nullptr),
    wide_view.peek(wide_tag),
    native.typed_nullptr_peek[wide_type](wide_view),
)
assert all(value == wide_expected for value in wide_results)
assert all(wide_parts(value) == (7, 1) for value in wide_results)
wide_output = wide_type(0)
assert wide_view.peek(wide_output) is None
assert wide_output == wide_expected and wide_parts(wide_output) == (7, 1)
wide_no_call = check_malformed(wide_view, wide_owner, wide_tag)
assert wide_owner.value() == wide_expected
assert int(wide_owner.peek_calls()) == 6
assert int(wide_owner.get_calls()) == 0

assert int(sc.sc_time_stamp().value()) == 0
assert int_no_call and wide_no_call
print(
    "TLM_PEEK_NONE_INT",
    *int_results,
    int(int_owner.value()),
    int(int_owner.peek_calls()),
    int(int_owner.get_calls()),
    int(sc.sc_time_stamp().value()),
)
print(
    "TLM_PEEK_NONE_WIDE",
    *[part for value in wide_results for part in wide_parts(value)],
    *wide_parts(wide_output),
    *wide_parts(wide_owner.value()),
    int(wide_owner.peek_calls()),
    int(wide_owner.get_calls()),
    int(sc.sc_time_stamp().value()),
)
print("TLM_PEEK_NONE_ERRORS TypeError TypeError native_calls=0")
