"""Verify public integer and vector constructors copy temporary sources."""

import gc
import weakref

from amal.eda.systemc import sc_dt as dt


INTEGER_VALUE = (1 << 136) | 0x123456789ABCDEF
INTEGER_TEXT = str(INTEGER_VALUE)


def decimal(value):
    """Render a native integer owner without narrowing its 137-bit value."""
    return str(value.to_string(dt.SC_DEC))


def integer_source(target_type, label, source_kind):
    """Copy a range or concatenation into an owning integer destination.

    Args:
        target_type: Public ``sc_biguint[137]`` or ``sc_unsigned`` class.
        label: Stable test label for assertion context.
        source_kind: Either ``range`` or ``concat``.
    Returns:
        The converted owner after every source proxy and its parent is released.

    The 137-bit source is borrowed only while the public constructor reads it.
    The returned target must own independent numeric storage.
    """
    parent = dt.sc_unsigned(137)
    parent.__assign__(INTEGER_VALUE)
    if source_kind == "range":
        source = parent(136, 0)
        source_views = [source]
    else:
        high = parent(136, 68)
        low = parent(67, 0)
        source_views = [high, low]
        source = dt.concat(high, low)
        source_views.append(source)

    assert source.length() == 137, label
    assert decimal(source) == INTEGER_TEXT, label
    target = target_type(source)
    assert target.__python_owns__, label
    assert target.length() == 137, label
    assert decimal(target) == INTEGER_TEXT, label

    # Changing the new owner cannot change the borrowed numeric source.
    target.__assign__(INTEGER_VALUE ^ 1)
    assert decimal(parent) == INTEGER_TEXT, label
    expected = str(INTEGER_VALUE ^ 1)

    parent_ref = weakref.ref(parent)
    source_refs = [weakref.ref(view) for view in source_views]
    if source_kind == "range":
        del source
    else:
        del high, low, source
    del source_views, parent
    gc.collect()

    assert parent_ref() is None, label
    assert all(reference() is None for reference in source_refs), label
    assert target.length() == 137, label
    assert decimal(target) == expected, label
    target.__assign__(INTEGER_VALUE ^ 3)
    assert decimal(target) == str(INTEGER_VALUE ^ 3), label
    return target


def vector_source(target_type, label, source_kind, pattern):
    """Copy a range or concatenation into an owning vector destination.

    Args:
        target_type: Public ``sc_bv_base`` or ``sc_lv_base`` class.
        label: Stable test label for assertion context.
        source_kind: Either ``range`` or ``concat``.
        pattern: Expected 137-character binary or four-state value.
    Returns:
        The converted owner after every source proxy and its parent is released.

    The parent remains live while the native proxy is read and copied. The
    destination must retain its own vector allocation after collection.
    """
    parent_type = dt.sc_bv[137] if target_type is dt.sc_bv_base else dt.sc_lv[137]
    parent = parent_type(pattern)
    if source_kind == "range":
        source = parent.range(136, 0)
        source_views = [source]
    else:
        high = parent.range(136, 68)
        low = parent.range(67, 0)
        source_views = [high, low]
        source = dt.concat(high, low)
        source_views.append(source)

    assert source.length() == 137, label
    assert str(source) == pattern, label
    target = target_type(source)
    assert target.__python_owns__, label
    assert target.length() == 137, label
    assert str(target) == pattern, label

    # A destination write leaves the still-live parent unchanged.
    target.set_bit(0, 0)
    mutated = pattern[:-1] + "0"
    assert str(parent) == pattern, label
    assert str(target) == mutated, label

    parent_ref = weakref.ref(parent)
    source_refs = [weakref.ref(view) for view in source_views]
    if source_kind == "range":
        del source
    else:
        del high, low, source
    del source_views, parent
    gc.collect()

    assert parent_ref() is None, label
    assert all(reference() is None for reference in source_refs), label
    assert target.length() == 137, label
    assert str(target) == mutated, label
    target.set_bit(1, 1)
    assert str(target) == pattern[:-2] + "10", label
    return target


integer_targets = []
for target_type, name in ((dt.sc_biguint[137], "sc_biguint"), (dt.sc_unsigned, "sc_unsigned")):
    for source_kind in ("range", "concat"):
        integer_targets.append(integer_source(target_type, f"{name}_{source_kind}", source_kind))

binary_pattern = "1" + "0" * 135 + "1"
logic_chars = list(binary_pattern)
logic_chars[40] = "X"
logic_chars[90] = "Z"
logic_pattern = "".join(logic_chars)
vector_targets = []
for target_type, name, pattern in (
    (dt.sc_bv_base, "sc_bv_base", binary_pattern),
    (dt.sc_lv_base, "sc_lv_base", logic_pattern),
):
    for source_kind in ("range", "concat"):
        vector_targets.append(vector_source(target_type, f"{name}_{source_kind}", source_kind, pattern))

assert len(integer_targets) == len(vector_targets) == 4
print("INTEGER_TEMPORARY_SOURCES_OK integer=4 vector=4 width=137")
