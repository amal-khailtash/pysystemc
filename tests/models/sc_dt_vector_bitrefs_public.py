"""Compare native binary/four-state vector bit-reference members and reports."""

from pathlib import Path
from amal.eda.systemc import sc_dt as d, sc_core as c
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_vector_bitrefs_probe.h"))
p = cppyy.gbl.vector_bitref_probe
c.sc_report_handler.set_actions(c.SC_WARNING, c.SC_DO_NOTHING)


def emit(*values):
    """Emit deterministic native value records without storing borrowed views."""
    print("VBR", *(int(value) if type(value) is bool else value for value in values))


def snapshot(view):
    """Copy the binary/four-state character represented by a live proxy."""
    return view.to_char()


def exercise(parent_type, width, binary):
    """Verify public mutable/readonly members and all native scalar overloads."""
    parent = parent_type(width) if parent_type in (d.sc_lv_base, d.sc_bv_base) else parent_type()
    parent.__assign__("0" * width)
    readonly_type = d.sc_bitref_r[parent_type]
    mutable_type = d.sc_bitref[parent_type]
    view = mutable_type(parent, 67)
    read = readonly_type(parent, 67)
    copy = mutable_type(view)
    read_copy = readonly_type(read)
    clone = view.clone()
    clone.__python_owns__ = True
    read_clone = read.clone()
    read_clone.__python_owns__ = True
    states = "01" if binary else "01XZ"
    for state in states:
        view.__assign__(state)
        complement = ~read
        converted = p.converted[parent_type](read)
        complement_char = complement.to_char()
        converted_char = converted.to_char()
        emit(
            width,
            binary,
            "read",
            state,
            read.length(),
            read.size(),
            int(read.value()),
            int(read.is_01()),
            int(read.to_bool()),
            int(bool(read)),
            int(not read),
            read.to_char(),
            int(read.get_bit(0)),
            int(read.get_word(0)),
            int(read.get_cword(0)),
            complement_char,
            converted_char,
            p.printed[parent_type](read),
            snapshot(copy),
            snapshot(read_copy),
            snapshot(clone),
            snapshot(read_clone),
        )
    other = parent_type(width) if parent_type in (d.sc_lv_base, d.sc_bv_base) else parent_type()
    other.__assign__("0" * width)
    other_view = mutable_type(other, 67)
    other_read = readonly_type(other, 67)
    arguments = [
        ("bool", False),
        ("bool", True),
        ("int", 0),
        ("int", 1),
        ("char", "0"),
        ("char", "1"),
        ("enum", d.Log_0),
        ("enum", d.Log_1),
        ("logic", d.sc_logic("0")),
        ("logic", d.sc_logic("1")),
        ("bit", d.sc_bit(False)),
        ("bit", d.sc_bit(True)),
    ]
    if not binary:
        arguments += [
            ("int", 2),
            ("int", 3),
            ("char", "X"),
            ("char", "Z"),
            ("enum", d.Log_X),
            ("enum", d.Log_Z),
            ("logic", d.sc_logic("X")),
            ("logic", d.sc_logic("Z")),
        ]
    for value in states:
        other_view.__assign__(value)
        for kind, argument in arguments + [("read", other_read), ("mutable", other_view)]:
            for operation in ("assign", "iand", "ior", "ixor"):
                if operation != "assign" and kind == "bit":
                    continue
                view.__assign__(value)
                result = getattr(view, "__" + operation + "__")(argument)
                assert cppyy.addressof(result) == cppyy.addressof(view)
                emit(width, binary, operation, value, kind, snapshot(view))
    for state in states:
        view.__assign__(state)
        assert cppyy.addressof(view.b_not()) == cppyy.addressof(view)
        emit(width, binary, "b_not", state, snapshot(view))
    view.set_bit(0, d.Log_1)
    view.set_word(0, 0)
    if not binary:
        view.set_cword(0, 1)
    else:
        view.set_cword(0, 0)
    view.clean_tail()
    emit(width, binary, "words", snapshot(view), view.get_word(0), view.get_cword(0))
    for state in states:
        stream = cppyy.gbl.std.istringstream(state)
        view.scan(stream)
        emit(width, binary, "scan", state, snapshot(view))
    c.sc_report_handler.set_actions(c.SC_ERROR, c.SC_THROW)
    for method in ("get_bit", "get_word", "get_cword", "set_bit", "set_word", "set_cword"):
        before = str(parent.to_string())
        try:
            getattr(view, method)(1, 0) if method.startswith("set") else getattr(read, method)(1)
        except c.sc_report as error:
            emit(width, binary, "bounds", method, error.get_id(), int(str(parent.to_string()) == before))
        else:
            raise AssertionError("missing native bounds report")
    c.sc_report_handler.set_actions(c.SC_ERROR, c.SC_THROW)
    for method in ("__assign__", "__iand__", "__ior__", "__ixor__"):
        for value in (4, chr(128)):
            before = str(parent.to_string())
            try:
                getattr(view, method)(value)
            except c.sc_report as error:
                assert error.get_id() == 204, error.get_id()
                emit(
                    width,
                    binary,
                    "invalid",
                    method,
                    str(ord(value) if isinstance(value, str) else value),
                    int(str(parent.to_string()) == before),
                )
            else:
                raise AssertionError("invalid scalar silently accepted")


assert p.traits()
for parent_type, width, binary in (
    (d.sc_lv_base, 137, False),
    (d.sc_bv_base, 191, True),
    (d.sc_lv[191], 191, False),
    (d.sc_bv[137], 137, True),
):
    exercise(parent_type, width, binary)
for owner_type in (d.sc_int[19], d.sc_uint[19]):
    owner = owner_type(1)
    emit("legacy_bitwidth", type(owner).__cpp_name__, owner[0].bitwidth())
print("VECTOR_BITREFS_PUBLIC_OK")
