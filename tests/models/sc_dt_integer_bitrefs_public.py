"""Compare public integer bit-reference behavior with native SystemC."""

from amal.eda.systemc import sc_dt as dt
import cppyy
import gc
from pathlib import Path

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_integer_bitrefs_probe.h"))
probe = cppyy.gbl.sc_dt_integer_bitrefs_probe
native_dt = cppyy.gbl.sc_dt


def emit(family, width, operation, *fields):
    """Print one observation consumed by the independent native comparator.

    Args:
        family: Fully qualified native mutable bit-reference family name.
        width: Width of the live parent integer.
        operation: Operation or signature family exercised by the record.
        fields: Scalar outcomes in native output order.
    """
    print("IBR\t" + "\t".join(map(str, (family, width, operation, *fields))))


def owner_value(owner):
    """Return the complete parent value in decimal without narrowing it.

    Args:
        owner: Live bounded or arbitrary-width SystemC integer.

    Returns:
        Exact decimal value maintained by the native owner.
    """
    return str(owner.to_string(dt.SC_DEC))


def exercise_owner(family, owner, width, index, read_view):
    """Exercise a borrowed mutable and const view while retaining its owner.

    Args:
        family: Fully qualified native mutable bit-reference family name.
        owner: Strong Python reference to the parent integer.
        width: Parent width.
        index: Valid selected bit index.
        read_view: Const proxy borrowed from ``owner`` by the native probe.
    """
    view = owner[index]
    mutable_type = getattr(dt, family.split("::")[-1])
    read_type = getattr(dt, family.split("::")[-1] + "_r")
    const_view = {
        "sc_dt::sc_int_bitref": probe.const_int_view,
        "sc_dt::sc_uint_bitref": probe.const_uint_view,
        "sc_dt::sc_signed_bitref": probe.const_signed_view,
        "sc_dt::sc_unsigned_bitref": probe.const_unsigned_view,
    }[family]
    mutable_copy = mutable_type(view)
    read_copy = read_type(read_view)

    assert type(view).__cpp_name__ == family
    # cppyy exposes the actual pooled derived shell and loses the C++ const base
    # reference in the Python proxy type; the native probe statically checks it.
    assert type(read_view).__cpp_name__ == family, type(read_view).__cpp_name__
    assert not bool(view.__python_owns__)
    assert not bool(read_view.__python_owns__)
    assert bool(mutable_copy.__python_owns__)
    assert not bool(read_view.__python_owns__)
    assert bool(read_copy.__python_owns__)
    emit(family, width, "access", view.length(), read_view.length(), int(view.value()), int(read_view.value()))
    emit(family, width, "copy_ctor", int(mutable_copy.value()), int(read_copy.value()))
    mutable_copy.__destruct__()
    # Integer string parsing may reuse pooled shells while assigning the owner.
    # Keep an owning rvalue proxy copy across those operations.
    assert owner_value(owner) == "0"

    digits = cppyy.gbl.std.vector[native_dt.sc_digit](2, 0)
    controls = cppyy.gbl.std.vector[native_dt.sc_digit](2, 0)
    data_present = read_view.concat_get_data(digits.data(), 1)
    control_present = read_view.concat_get_ctrl(controls.data(), 1)
    printed = cppyy.gbl.std.ostringstream()
    read_view.print(printed)
    emit(
        family,
        width,
        "read",
        int(read_view.value()),
        int(read_view.to_bool()),
        int(read_view),
        int(not read_view),
        int(~read_view),
        int(read_view.concat_length(cppyy.nullptr)),
        int(read_view.concat_get_uint64()),
        int(data_present),
        int(digits[0]),
        int(digits[1]),
        int(control_present),
        int(controls[0]),
        int(controls[1]),
        str(printed.str()),
    )
    print(
        "IBR_GAP\t"
        + "\t".join(
            map(
                str,
                (
                    family + "_r",
                    width,
                    "const_proxy_type",
                    f"cppyy_type={type(read_view).__cpp_name__}",
                    "native_static_type=" + family + "_r",
                ),
            )
        )
    )

    for bit_value in (False, True):
        owner.__assign__(int(bit_value) << index)
        mutable_truth_view = owner[index]
        const_truth_view = read_type(const_view(owner, index))
        emit(
            family,
            width,
            "truth",
            "mutable",
            int(bit_value),
            int(bool(mutable_truth_view)),
            int(not mutable_truth_view),
        )
        emit(
            family,
            width,
            "truth",
            "const",
            int(bit_value),
            int(bool(const_truth_view)),
            int(not const_truth_view),
        )
        const_truth_view.__destruct__()
    owner.__assign__(0)
    emit(
        family,
        width,
        "bitwidth",
        int(read_view.bitwidth()) if hasattr(read_view, "bitwidth") else "feature-disabled",
    )

    owner.__assign__(0)
    owner[index].__assign__(True)
    emit(family, width, "assign_bool", owner_value(owner))
    owner.__assign__(0)
    owner[index - 1].__assign__(True)
    owner[index].__assign__(owner[index - 1])
    emit(family, width, "assign_lvalue", owner_value(owner))
    owner.__assign__(0)
    owner[index].__assign__(True)
    owner[index - 1].__assign__(read_copy)
    emit(family, width, "assign_rvalue", owner_value(owner))

    for operand in (False, True):
        owner.__assign__(0)
        owner[index].__assign__(True)
        owner[index].__iand__(operand)
        emit(family, width, "and", int(operand), owner_value(owner))
        owner.__assign__(0)
        owner[index].__ior__(operand)
        emit(family, width, "or", int(operand), owner_value(owner))
        owner.__assign__(0)
        owner[index].__assign__(True)
        owner[index].__ixor__(operand)
        emit(family, width, "xor", int(operand), owner_value(owner))

    for text in ("0", "1"):
        owner.__assign__(0)
        stream = cppyy.gbl.std.istringstream(text)
        owner[index].scan(stream)
        emit(family, width, "scan", text, owner_value(owner))

    owner.__assign__(0)
    owner[index].concat_set(probe.Int64(8), 3)
    emit(family, width, "concat_int64", owner_value(owner))
    owner.__assign__(0)
    owner[index].concat_set(probe.UInt64(8), 3)
    emit(family, width, "concat_uint64", owner_value(owner))
    owner.__assign__(0)
    signed_source = dt.sc_signed(16)
    signed_source.__assign__(32)
    owner[index].concat_set(signed_source, 5)
    emit(family, width, "concat_signed", owner_value(owner))
    owner.__assign__(0)
    unsigned_source = dt.sc_unsigned(16)
    unsigned_source.__assign__(32)
    owner[index].concat_set(unsigned_source, 5)
    emit(family, width, "concat_unsigned", owner_value(owner))
    read_copy.__destruct__()


def exercise_pool(family, owner, width, pool_size):
    """Check pool wraparound, borrowed-shell ownership, and copied-view lifetime.

    Args:
        family: Fully qualified native mutable bit-reference family name.
        owner: Strong Python reference to the parent integer.
        width: Parent width.
        pool_size: Capacity exposed by the native diagnostic helper.
    """
    owner.__assign__(1)
    first = owner[0]
    view_type = getattr(dt, family.split("::")[-1])
    read_type = getattr(dt, family.split("::")[-1] + "_r")
    const_view = {
        "sc_dt::sc_int_bitref": probe.const_int_view,
        "sc_dt::sc_uint_bitref": probe.const_uint_view,
        "sc_dt::sc_signed_bitref": probe.const_signed_view,
        "sc_dt::sc_unsigned_bitref": probe.const_unsigned_view,
    }[family]
    copied = view_type(first)
    first_read = const_view(owner, 0)
    copied_read = read_type(first_read)
    assert not bool(first.__python_owns__)
    assert not bool(first_read.__python_owns__)
    assert bool(copied.__python_owns__)
    assert bool(copied_read.__python_owns__)
    first_address = cppyy.addressof(first)
    first_read_address = cppyy.addressof(first_read)
    for _ in range(pool_size - 2):
        owner[1]
    wrapped = owner[1]
    wrapped_read = const_view(owner, 1)
    wrapped_address = cppyy.addressof(wrapped)
    wrapped_read_address = cppyy.addressof(wrapped_read)
    emit(
        family,
        width,
        "pool",
        pool_size,
        int(first_address == wrapped_address),
        int(first_read_address == wrapped_read_address),
        int(first.value()),
        int(copied.value()),
        int(first_read.value()),
        int(copied_read.value()),
    )
    copied.__assign__(False)
    emit(family, width, "copied_view_write", owner_value(owner), int(copied_read.value()))
    copied.__destruct__()
    copied_read.__destruct__()
    del copied, copied_read, first, first_read, wrapped, wrapped_read
    gc.collect()
    emit(family, width, "copy_lifetime", owner_value(owner))


def exercise_errors(family, owner, width):
    """Verify invalid owner indexes throw native reports without changing value.

    Args:
        family: Fully qualified native mutable bit-reference family name.
        owner: Strong Python reference to the parent integer.
        width: Parent width.
    """
    old_actions = probe.throw_errors()
    try:
        for index in (-1, width):
            owner.__assign__(0)
            try:
                owner[index]
            except Exception:
                emit(family, width, "invalid_index", index, 1, owner_value(owner))
            else:
                raise AssertionError(f"{family} accepted invalid bit index {index}")
    finally:
        probe.restore_errors(old_actions)


def exercise_fixed(width):
    """Exercise bounded signed and unsigned views at a selected width.

    Args:
        width: Width of the bounded integer template instance.
    """
    signed_owner = dt.sc_int[width](0)
    signed_read = probe.const_int_view(signed_owner, 3)
    exercise_owner("sc_dt::sc_int_bitref", signed_owner, width, 3, signed_read)
    exercise_pool("sc_dt::sc_int_bitref", signed_owner, width, int(probe.int_pool_size()))
    exercise_errors("sc_dt::sc_int_bitref", signed_owner, width)

    unsigned_owner = dt.sc_uint[width](0)
    unsigned_read = probe.const_uint_view(unsigned_owner, 3)
    exercise_owner("sc_dt::sc_uint_bitref", unsigned_owner, width, 3, unsigned_read)
    exercise_pool("sc_dt::sc_uint_bitref", unsigned_owner, width, int(probe.uint_pool_size()))
    exercise_errors("sc_dt::sc_uint_bitref", unsigned_owner, width)


def exercise_arbitrary(width):
    """Exercise arbitrary-width signed and unsigned owner views.

    Args:
        width: Digit width of the arbitrary integer instance.
    """
    signed_owner = dt.sc_signed(width)
    signed_owner.__assign__(0)
    signed_read = probe.const_signed_view(signed_owner, 3)
    exercise_owner("sc_dt::sc_signed_bitref", signed_owner, width, 3, signed_read)
    exercise_pool("sc_dt::sc_signed_bitref", signed_owner, width, int(probe.signed_pool_size()))
    exercise_errors("sc_dt::sc_signed_bitref", signed_owner, width)

    unsigned_owner = dt.sc_unsigned(width)
    unsigned_owner.__assign__(0)
    unsigned_read = probe.const_unsigned_view(unsigned_owner, 3)
    exercise_owner("sc_dt::sc_unsigned_bitref", unsigned_owner, width, 3, unsigned_read)
    exercise_pool("sc_dt::sc_unsigned_bitref", unsigned_owner, width, int(probe.unsigned_pool_size()))
    exercise_errors("sc_dt::sc_unsigned_bitref", unsigned_owner, width)


for bounded_width in (19, 63):
    exercise_fixed(bounded_width)
for arbitrary_width in (137, 191):
    exercise_arbitrary(arbitrary_width)
