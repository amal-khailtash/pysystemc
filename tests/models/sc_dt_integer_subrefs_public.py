"""Compare public integer range-reference behavior with native SystemC."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_integer_subrefs_probe.h"))
probe = cppyy.gbl.sc_dt_integer_subrefs_probe
native_dt = cppyy.gbl.sc_dt


def emit(family, width, operation, *fields):
    """Print one scalar observation consumed by the native comparator.

    Args:
        family: Fully qualified mutable native range-reference type.
        width: Width of the live parent integer.
        operation: API family exercised by this record.
        fields: Scalar outcomes in native output order.
    """
    print("ISR\t" + "\t".join(map(str, (family, width, operation, *fields))))


def native_string(value):
    """Convert a cppyy string result to its native text spelling.

    Args:
        value: Native ``std::string`` result represented by cppyy.
    Returns:
        UTF-8 decoded string suitable for parity output.
    """
    return value.decode() if isinstance(value, bytes) else str(value)


def owner_value(owner):
    """Return the complete parent value in decimal without narrowing it.

    Args:
        owner: Live bounded or arbitrary-width SystemC integer.
    Returns:
        Exact decimal parent value.
    """
    return native_string(owner.to_string(dt.SC_DEC))


def const_selector(family):
    """Select the native helper for a family-specific const range view.

    Args:
        family: Fully qualified mutable range-reference type.
    Returns:
        Probe function returning that owner's borrowed const range view.
    """
    return {
        "sc_dt::sc_int_subref": probe.const_int_range,
        "sc_dt::sc_uint_subref": probe.const_uint_range,
        "sc_dt::sc_signed_subref": probe.const_signed_range,
        "sc_dt::sc_unsigned_subref": probe.const_unsigned_range,
    }[family]


def read_signature(family, width, label, read_view):
    """Emit reductions, conversions, concatenation data and formatted output.

    Args:
        family: Fully qualified mutable range-reference type.
        width: Width of the live parent integer.
        label: Read-view category used in output.
        read_view: Owned const range proxy copied from its borrowed shell.
    """
    digits = cppyy.gbl.std.vector[native_dt.sc_digit](2, 0)
    controls = cppyy.gbl.std.vector[native_dt.sc_digit](2, 0)
    data_present = read_view.concat_get_data(digits.data(), 0)
    control_present = read_view.concat_get_ctrl(controls.data(), 0)
    printed = cppyy.gbl.std.ostringstream()
    read_view.print(printed)
    numeric = (
        int(read_view.to_int()),
        int(read_view.to_uint()),
        int(read_view.to_long()),
        int(read_view.to_ulong()),
        int(read_view.to_int64()),
        int(read_view.to_uint64()),
        int(round(read_view.to_double())),
    )
    if family in ("sc_dt::sc_int_subref", "sc_dt::sc_uint_subref"):
        conversion = int(read_view.value()), int(read_view)
    else:
        converted = dt.sc_unsigned(read_view)
        conversion = native_string(converted.to_string(dt.SC_DEC)), int(converted.length())
    emit(
        family,
        width,
        "read",
        label,
        read_view.length(),
        int(read_view.and_reduce()),
        int(read_view.nand_reduce()),
        int(read_view.or_reduce()),
        int(read_view.nor_reduce()),
        int(read_view.xor_reduce()),
        int(read_view.xnor_reduce()),
        *numeric,
        *conversion,
        int(read_view.concat_length(cppyy.nullptr)),
        int(read_view.concat_get_uint64()),
        int(data_present),
        int(digits[0]),
        int(digits[1]),
        int(control_present),
        int(controls[0]),
        int(controls[1]),
        native_string(read_view.to_string(dt.SC_DEC)),
        native_string(read_view.to_string(dt.SC_HEX)),
        native_string(read_view.to_string(dt.SC_HEX, True)),
        native_string(printed.str()),
    )
    if family in ("sc_dt::sc_int_subref", "sc_dt::sc_uint_subref"):
        emit(family, width, "bitwidth", label, read_view.bitwidth())


def exercise_pool(family, owner, width, left, right, pool_size):
    """Check pool-shell reuse and copy durability while retaining the owner.

    Args:
        family: Fully qualified mutable range-reference type.
        owner: Strong reference to the live parent integer.
        width: Width of the parent integer.
        left: Original range's high bit.
        right: Original range's low bit.
        pool_size: Capacity exposed by the native diagnostics helper.
    """
    select_const = const_selector(family)
    owner.__assign__(1 << right)
    first = owner(left, right)
    const_first = select_const(owner, left, right)
    view_type = getattr(dt, family.split("::")[-1])
    read_type = getattr(dt, family.split("::")[-1] + "_r")
    copied = view_type(first)
    copied_read = read_type(const_first)
    assert not bool(first.__python_owns__)
    assert not bool(const_first.__python_owns__)
    assert bool(copied.__python_owns__)
    assert bool(copied_read.__python_owns__)
    first_address = cppyy.addressof(first)
    const_first_address = cppyy.addressof(const_first)
    for _ in range(pool_size - 2):
        owner(3, 0)
    wrapped = owner(3, 0)
    wrapped_read = select_const(owner, 3, 0)
    wrapped_address = cppyy.addressof(wrapped)
    wrapped_read_address = cppyy.addressof(wrapped_read)
    emit(
        family,
        width,
        "pool",
        pool_size,
        int(first_address == wrapped_address),
        int(const_first_address == wrapped_read_address),
        first.length(),
        copied.length(),
        int(first.to_uint64()),
        int(copied.to_uint64()),
        const_first.length(),
        copied_read.length(),
        int(copied_read.to_uint64()),
    )
    copied.__assign__(0)
    emit(family, width, "copy_write", owner_value(owner), int(copied_read.to_uint64()))
    copied.__destruct__()
    copied_read.__destruct__()
    del copied, copied_read, first, const_first, wrapped, wrapped_read
    emit(family, width, "copy_lifetime", owner_value(owner))


def exercise_errors(family, owner, width, left, right):
    """Check invalid bounded ranges and out-of-bounds accesses under SC_THROW.

    Args:
        family: Fully qualified mutable range-reference type.
        owner: Strong reference to the parent integer.
        width: Width of the parent integer.
        left: A known-valid range's high bit.
        right: A known-valid range's low bit.
    """
    previous = probe.throw_errors()
    try:
        invalid_ranges = [(width, 0), (1, -1)]
        if family in ("sc_dt::sc_int_subref", "sc_dt::sc_uint_subref"):
            invalid_ranges.append((right, left))
        for bad_left, bad_right in invalid_ranges:
            owner.__assign__(0)
            try:
                owner(bad_left, bad_right)
            except Exception:
                emit(family, width, "invalid_range", bad_left, bad_right, 1, owner_value(owner))
            else:
                raise AssertionError(f"{family} accepted invalid range ({bad_left}, {bad_right})")
    finally:
        probe.restore_errors(previous)


def exercise_owner(family, owner, width, left, right, pool_size):
    """Exercise one public mutable/const range pair and its parent operations.

    Args:
        family: Fully qualified mutable range-reference type.
        owner: Live bounded or arbitrary-width SystemC integer.
        width: Width of the parent integer.
        left: Selected range's high bit.
        right: Selected range's low bit.
        pool_size: Capacity of the native family's reusable proxy pool.
    """
    select_const = const_selector(family)
    read_type = getattr(dt, family.split("::")[-1] + "_r")
    owner.__assign__(0)
    high_bit = width - 2 if family in ("sc_dt::sc_int_subref", "sc_dt::sc_uint_subref") else width - 1
    owner[high_bit].__assign__(True)
    owner(left, right).__assign__(0xA6)
    initial_value = owner_value(owner)
    mutable_view = owner(left, right)
    const_shell = select_const(owner, left, right)
    copied_mutable = getattr(dt, family.split("::")[-1])(mutable_view)
    copied_read = read_type(const_shell)
    assert not bool(mutable_view.__python_owns__)
    assert not bool(const_shell.__python_owns__)
    assert bool(copied_mutable.__python_owns__)
    assert bool(copied_read.__python_owns__)
    emit(
        family,
        width,
        "access",
        left,
        right,
        initial_value,
        mutable_view.length(),
        native_string(mutable_view.to_string(dt.SC_DEC)),
        copied_read.length(),
        native_string(copied_read.to_string(dt.SC_DEC)),
    )
    emit(
        family,
        width,
        "copy_ctor",
        native_string(copied_mutable.to_string(dt.SC_DEC)),
        native_string(copied_read.to_string(dt.SC_DEC)),
    )
    read_signature(family, width, "forward", copied_read)
    if family == "sc_dt::sc_signed_subref":
        initialized_read = probe.initialized_signed_range(owner, left, right)
    elif family == "sc_dt::sc_unsigned_subref":
        initialized_read = probe.initialized_unsigned_range(owner, left, right)
    else:
        initialized_read = None
    if initialized_read is not None:
        assert bool(initialized_read.__python_owns__)
        read_signature(family, width, "protected_initialize", initialized_read)
        initialized_read.__destruct__()
    copied_mutable.__destruct__()
    copied_read.__destruct__()

    if family in ("sc_dt::sc_signed_subref", "sc_dt::sc_unsigned_subref"):
        reverse_mutable = owner(right, left)
        reverse_read = read_type(select_const(owner, right, left))
        emit(
            family,
            width,
            "reverse",
            reverse_mutable.length(),
            int(reverse_mutable.to_uint64()),
            reverse_read.length(),
            native_string(reverse_read.to_string(dt.SC_DEC)),
            int(reverse_read.to_uint64()),
        )
        reverse_read.__destruct__()

    source_type = type(owner)
    source_owner = (
        source_type(width) if family in ("sc_dt::sc_signed_subref", "sc_dt::sc_unsigned_subref") else source_type(0)
    )
    if family in ("sc_dt::sc_signed_subref", "sc_dt::sc_unsigned_subref"):
        source_owner.__assign__(0x5A << right)
    else:
        source_owner.__assign__(0x5A << right)
    source_mutable = source_owner(left, right)
    source_read = read_type(select_const(source_owner, left, right))
    source_mutable_copy = getattr(dt, family.split("::")[-1])(source_mutable)
    source_read_copy = read_type(source_read)

    def assign_value(operation, value):
        """Assign one value to the destination view and emit the parent.

        Args:
            operation: Assignment overload category.
            value: Value passed to the Python proxy's assignment method.
        """
        owner.__assign__(0)
        owner(left, right).__assign__(value)
        emit(family, width, operation, owner_value(owner))

    def assign_overload(operation, signature, value):
        """Dispatch one scalar assignment through its exact C++ signature.

        Args:
            operation: Output label for this native overload.
            signature: Exact C++ parameter signature selected by cppyy.
            value: Scalar value supplied to that overload.
        """
        owner.__assign__(0)
        target = owner(left, right)
        target.__assign__.__overload__(signature)(target, value)
        emit(family, width, operation, owner_value(owner))

    assign_value("assign_mutable", source_mutable_copy)
    assign_value("assign_const", source_read_copy)
    assign_value("assign_cstr", "173")
    int64_signature = "sc_dt::int_type" if family == "sc_dt::sc_int_subref" else "sc_dt::int64"
    uint64_signature = "sc_dt::uint_type" if family == "sc_dt::sc_uint_subref" else "sc_dt::uint64"
    for operation, signature, value in (
        ("assign_int", "int", 37),
        ("assign_uint", "unsigned int", 37),
        ("assign_long", "long", -13),
        ("assign_ulong", "unsigned long", 221),
        ("assign_int64", int64_signature, probe.Int64(-13)),
        ("assign_uint64", uint64_signature, probe.UInt64(221)),
        ("assign_double", "double", 37.75),
    ):
        assign_overload(operation, signature, value)
    signed_source = dt.sc_signed(16)
    signed_source.__assign__(0x35)
    unsigned_source = dt.sc_unsigned(16)
    unsigned_source.__assign__(0x69)
    assign_value("assign_signed", signed_source)
    assign_value("assign_unsigned", unsigned_source)
    fixed_signed_source = dt.sc_int[16](0x47)
    fixed_unsigned_source = dt.sc_uint[16](0x83)
    if family == "sc_dt::sc_int_subref":
        assign_overload("assign_int_base", "const sc_dt::sc_int_base&", fixed_signed_source)
        assign_value("assign_sc_uint_generic", fixed_unsigned_source)
    elif family == "sc_dt::sc_uint_subref":
        assign_value("assign_sc_int_generic", fixed_signed_source)
        assign_overload("assign_uint_base", "const sc_dt::sc_uint_base&", fixed_unsigned_source)
    else:
        assign_overload("assign_int_base", "const sc_dt::sc_int_base&", fixed_signed_source)
        assign_overload("assign_uint_base", "const sc_dt::sc_uint_base&", fixed_unsigned_source)
    generic_source = probe.generic_concat_source(signed_source, unsigned_source)
    assign_value("assign_generic_concat", generic_source)
    if family in ("sc_dt::sc_int_subref", "sc_dt::sc_uint_subref"):
        assign_value("assign_bv", dt.sc_bv_base("0000000000101010"))
        assign_value("assign_lv", dt.sc_lv_base("0000000000011011"))
    if family in ("sc_dt::sc_signed_subref", "sc_dt::sc_unsigned_subref"):
        if family == "sc_dt::sc_signed_subref":
            other_owner = dt.sc_unsigned(width)
            other_selector = probe.const_unsigned_range
            other_read_type = dt.sc_unsigned_subref_r
        else:
            other_owner = dt.sc_signed(width)
            other_selector = probe.const_signed_range
            other_read_type = dt.sc_signed_subref_r
        other_owner.__assign__(0)
        other_owner(left, right).__assign__(0x37)
        other_const = other_read_type(other_selector(other_owner, left, right))
        assign_value("assign_cross_const", other_const)
        other_const.__destruct__()

    for text in ("0", "1", "173"):
        owner.__assign__(0)
        owner(left, right).scan(cppyy.gbl.std.istringstream(text))
        emit(family, width, "scan", text, owner_value(owner))

    for operation, source in (
        ("concat_int64", probe.Int64(0x35)),
        ("concat_uint64", probe.UInt64(0x69)),
        ("concat_signed", signed_source),
        ("concat_unsigned", unsigned_source),
    ):
        owner.__assign__(0)
        owner(left, right).concat_set(source, 0)
        emit(family, width, operation, owner_value(owner))

    source_mutable_copy.__destruct__()
    source_read_copy.__destruct__()
    exercise_pool(family, owner, width, left, right, pool_size)
    exercise_errors(family, owner, width, left, right)


def exercise_fixed(family, width):
    """Run a bounded integer range matrix at one width.

    Args:
        family: Bounded signed or unsigned mutable range family.
        width: Template width for the bounded owner.
    """
    owner = (dt.sc_int if family == "sc_dt::sc_int_subref" else dt.sc_uint)[width](0)
    pool_size = probe.int_pool_size() if family == "sc_dt::sc_int_subref" else probe.uint_pool_size()
    exercise_owner(family, owner, width, 14, 5, int(pool_size))


def exercise_arbitrary(family, width):
    """Run an arbitrary-width integer range matrix at one width.

    Args:
        family: Arbitrary signed or unsigned mutable range family.
        width: Runtime owner width.
    """
    owner = dt.sc_signed(width) if family == "sc_dt::sc_signed_subref" else dt.sc_unsigned(width)
    pool_size = probe.signed_pool_size() if family == "sc_dt::sc_signed_subref" else probe.unsigned_pool_size()
    exercise_owner(family, owner, width, 14, 5, int(pool_size))


for _width in (19, 63):
    exercise_fixed("sc_dt::sc_int_subref", _width)
    exercise_fixed("sc_dt::sc_uint_subref", _width)
for _width in (137, 191):
    exercise_arbitrary("sc_dt::sc_signed_subref", _width)
    exercise_arbitrary("sc_dt::sc_unsigned_subref", _width)
