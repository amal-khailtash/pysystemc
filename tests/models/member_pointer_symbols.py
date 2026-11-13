"""Verify native pointer-variable capture, values, errors and borrowed lifetimes."""

from pathlib import Path
import gc
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/member_pointer_symbols_probe.h"))
probe = cppyy.gbl.member_pointer_symbols_probe


def expect(error_type, action, cause=False):
    """Require the documented exception and compiler cause for invalid symbols."""
    try:
        action()
    except error_type as error:
        if cause:
            assert error.__cause__ is not None
        return
    raise AssertionError(f"expected {error_type.__name__}")


def main():
    """Check fresh native values, both assembly routes, type safety and retention."""
    element = probe.Element
    kind = sc.sc_member_access[element, probe.Wide].member_type
    values = sc.sc_vector[element]("python_symbols", 2)
    first = kind.from_pointer("member_pointer_symbols_probe::selection")
    copied = kind(first)
    assert first == copied == kind.from_member("first")
    first_assembly = values.assemble(first)
    probe.select_second(True)
    second = kind.from_pointer("::member_pointer_symbols_probe::selection")
    assert first != second and second == kind.from_member("second")
    second_assembly = sc.sc_assemble_vector(values, second)
    native = probe.native_current(values)
    assert cppyy.addressof(first_assembly[0]) == cppyy.addressof(values[0].first)
    assert cppyy.addressof(second_assembly[1]) == cppyy.addressof(native[1])
    fixed = kind.from_pointer("member_pointer_symbols_probe::fixed")
    assert fixed == second
    null = kind.from_pointer("member_pointer_symbols_probe::null_selection")
    assert not null.valid()
    expect(ValueError, lambda values=values: values.assemble(null))
    expect(ValueError, lambda values=values: sc.sc_assemble_vector(values, null))
    constants = sc.sc_member_access[element, "const int"].member_type
    constant = constants.from_pointer("member_pointer_symbols_probe::const_selection")
    assert int(sc.sc_assemble_vector(values, constant)[0]) == 41
    derived = sc.sc_vector[probe.Derived]("python_inherited", 1)
    inherited_kind = sc.sc_member_access[probe.Derived, int].member_type
    inherited = inherited_kind.from_pointer("member_pointer_symbols_probe::inherited_selection")
    assert int(derived.assemble(inherited)[0]) == 73

    for symbol in ("", "member_pointer_symbols_probe::selection()", "member_pointer_symbols_probe::selection;1"):
        expect(ValueError, lambda symbol=symbol: kind.from_pointer(symbol))
    expect(TypeError, lambda: kind.from_pointer(object()))
    for symbol in (
        "member_pointer_symbols_probe::missing",
        "member_pointer_symbols_probe::wrong_scalar",
        "member_pointer_symbols_probe::wrong_owner",
    ):
        expect(TypeError, lambda symbol=symbol: kind.from_pointer(symbol), cause=True)
    mutable = sc.sc_member_access[element, int].member_type
    expect(TypeError, lambda: mutable.from_pointer("member_pointer_symbols_probe::const_selection"), cause=True)
    # An untyped reflected view must be rejected before cppyy overload marshalling.
    raw = probe.selection
    expect(TypeError, lambda values=values: values.assemble(raw))
    expect(TypeError, lambda values=values: sc.sc_assemble_vector(values, raw))
    probe.select_second(False)
    recovered = kind.from_pointer("member_pointer_symbols_probe::selection")
    assert recovered == first and kind.from_pointer("member_pointer_symbols_probe::fixed") == second
    assert sc.sc_assemble_vector(values, recovered).size() == 2
    assert sc.sc_member_access[element, probe.Wide](first).get(values[0]) == values[0].first

    owner_ref = weakref.ref(values)
    assert first_assembly._vector_owner is values and second_assembly._vector_owner is values
    del native, values
    gc.collect()
    assert owner_ref() is first_assembly._vector_owner
    print(
        "MEMBER_SYMBOL_NATIVE",
        f"first={first_assembly[0].to_string(cppyy.gbl.sc_dt.SC_DEC)}",
        f"second={second_assembly[1].to_string(cppyy.gbl.sc_dt.SC_DEC)}",
        "const=41 inherited=73",
    )
    print("MEMBER_SYMBOL_PYTHON routes=2 dynamic=1 copy=1 null=1 const=1 inheritance=1 errors=1 recovery=1 lifetime=1")


main()
