"""Compare selected datatype adapter edges with native SystemC behavior."""

import ctypes
import gc
from pathlib import Path
import weakref

from amal.eda.systemc import sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_dt_fixed_subrefs_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.sc_dt_fixed_subrefs_probe


def emit(*fields):
    """Print one deterministic datatype edge record."""
    print("DAE\t" + "\t".join(map(str, fields)))


def binary(value):
    """Render a native datatype in binary notation for reference comparison."""
    return str(value.to_string(dt.SC_BIN, True))


def text_concatenation():
    """Check text at both ends of a live native integer concatenation."""
    head_owner = dt.sc_uint[13](22)
    head_view = dt.concat("101", head_owner)
    emit("CONCAT", "head", "initial", binary(head_view))
    head_owner.__assign__(5)
    emit("CONCAT", "head", "updated", binary(head_view), head_owner.to_uint())
    del head_view, head_owner

    tail_owner = dt.sc_uint[13](22)
    tail_view = dt.concat(tail_owner, "011")
    emit("CONCAT", "tail", "initial", binary(tail_view))
    tail_owner.__assign__(5)
    emit("CONCAT", "tail", "updated", binary(tail_view), tail_owner.to_uint())

    vector_head_owner = dt.sc_bv[8]("01011010")
    vector_head_view = dt.concat("10", vector_head_owner)
    emit("CONCAT", "vector_head", "initial", binary(vector_head_view))
    vector_head_owner.__assign__("10101010")
    emit("CONCAT", "vector_head", "updated", binary(vector_head_view), binary(vector_head_owner))
    del vector_head_view, vector_head_owner

    vector_tail_owner = dt.sc_bv[8]("01011010")
    vector_tail_view = dt.concat(vector_tail_owner, "10")
    emit("CONCAT", "vector_tail", "initial", binary(vector_tail_view))
    vector_tail_owner.__assign__("10101010")
    emit("CONCAT", "vector_tail", "updated", binary(vector_tail_view), binary(vector_tail_owner))


def fixed_range_copy(label, number_type, const_range):
    """Copy a slow or fast read-only range, then release its native parent.

    Args:
        label: Stable model name used in parity records.
        number_type: Native fixed-point owner template specialization.
        const_range: Native helper selecting the read-only range overload.
    """
    parent = number_type("5.25")
    parent_ref = weakref.ref(parent)
    view = const_range(parent, 11, 4)
    copied = dt.sc_bv[8](view)
    initial = binary(copied)
    emit("RANGE", label, "copy", initial, copied.length())

    parent.__assign__(0)
    emit("RANGE", label, "parent_updated", parent.to_string(dt.SC_DEC), binary(copied))
    del view, parent
    gc.collect()
    assert parent_ref() is None
    emit("RANGE", label, "parent_deleted", int(parent_ref() is None), binary(copied))


def mutable_fixed_range_copy(label, number_type):
    """Copy a mutable native range, then test parent and proxy lifetime.

    Args:
        label: Stable model name used in parity records.
        number_type: Native fixed-point owner template specialization.
    """
    parent = number_type("5.25")
    parent_ref = weakref.ref(parent)
    view = parent.range(11, 4)
    copied = dt.sc_bv[8](view)
    initial = binary(copied)
    emit("RANGE", label, "copy", initial, copied.length())

    parent.__assign__(0)
    emit("RANGE", label, "parent_updated", binary(view), binary(copied))
    del view, parent
    gc.collect()
    assert parent_ref() is None
    emit("RANGE", label, "parent_deleted", int(parent_ref() is None), binary(copied))


def exact_integer_assignment():
    """Assign an integer wider than native 64-bit scalar overloads exactly."""
    value = 18_446_744_073_709_551_637
    target = dt.sc_fxval("0")
    target.__assign__(value)
    rendered = str(target.to_string(dt.SC_DEC))
    assert rendered == str(value)
    emit("ASSIGN", "wide_integer", rendered)


def unsupported_operator_argument():
    """Record the native-operator adapter's public TypeError contract."""
    rejected = False
    try:
        dt.sc_bigint[13](7) + object()
    except TypeError as error:
        rejected = True
        emit("ERROR", "unsupported_operator", type(error).__name__)
    assert rejected


def ordinary_stream_extraction():
    """Check standard string adaptation, mutable fallback, and scalar rejection."""
    stream = cppyy.gbl.std.istringstream("first second")
    first = cppyy.gbl.std.string()
    second = cppyy.gbl.std.string()
    chain = stream.__rshift__(first)
    chain.__rshift__(second)
    third = cppyy.gbl.std.string()
    stream.__rshift__(third)
    emit("STREAM", "ordinary_string", str(first), str(second), str(third), int(stream.eof()), int(stream.fail()))

    integer_stream = cppyy.gbl.std.istringstream("41")
    integer_target = ctypes.c_int(0)
    integer_stream.__rshift__(integer_target)
    emit(
        "STREAM",
        "mutable_ctypes_int",
        integer_target.value,
        int(integer_stream.eof()),
        int(integer_stream.fail()),
    )

    immutable_targets = (
        ("bool", True),
        ("int", 1),
        ("float", 1.5),
        ("str", "old"),
        ("cppyy_int", cppyy.gbl.int(0)),
    )
    for label, immutable_target in immutable_targets:
        rejected_stream = cppyy.gbl.std.istringstream("41")
        rejected = False
        try:
            rejected_stream.__rshift__(immutable_target)
        except TypeError as error:
            rejected = True
            message = str(error)
        assert rejected
        assert not rejected_stream.eof() and not rejected_stream.fail()

        recovered_target = ctypes.c_int(0)
        rejected_stream.__rshift__(recovered_target)
        assert recovered_target.value == 41
        assert rejected_stream.eof() and not rejected_stream.fail()
        emit("ERROR", "immutable_stream_target", label, "TypeError", message, recovered_target.value)


text_concatenation()
fixed_range_copy("slow", dt.sc_fixed[16, 8], probe.fixed_const_range)
fixed_range_copy("fast", dt.sc_fixed_fast[16, 8], probe.fast_const_range)
mutable_fixed_range_copy("mutable_slow", dt.sc_fixed[16, 8])
mutable_fixed_range_copy("mutable_fast", dt.sc_fixed_fast[16, 8])
exact_integer_assignment()
unsupported_operator_argument()
ordinary_stream_extraction()
