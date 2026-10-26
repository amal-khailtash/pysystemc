"""Compare legacy sc_string_old behavior with an independent C++ reference."""

import gc
from pathlib import Path
import sys
import weakref

from amal.eda.systemc import sc_core as c
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_dt_string_old_probe.h"
cppyy.include(str(PROBE_HEADER))
PROBE = cppyy.gbl.sc_dt_string_old_probe
STRING = cppyy.gbl.sc_dt.sc_string_old


def text(value):
    """Copy a native string's bytes to Python text without replacing its type.

    Args:
        value: Live ``sc_string_old`` object.

    Returns:
        Independent Python string copied from the native ``c_str()`` result.
    """
    return str(value.c_str())


def emit(*fields):
    """Emit one deterministic tab-separated legacy-string observation."""
    print("SO\t" + "\t".join(str(field) for field in fields))


def error_mode(mode):
    """Capture one native error report in an isolated Python process.

    Args:
        mode: One of the report cases also implemented by the native program.
    """
    previous = c.sc_report_handler.set_actions(c.SC_ERROR, c.SC_THROW)
    try:
        value = STRING("abc")
        if mode == "insert-upper":
            value.insert(STRING("x"), 4)
        elif mode == "remove-negative":
            value.remove(0xFFFFFFFF, 1)
        elif mode == "delimiter-upper":
            value.is_delimiter(STRING("abc"), 3)
        else:
            raise ValueError(f"unknown sc_string_old report mode: {mode}")
    except c.sc_report as report:
        emit("report", mode, report.get_msg_type(), report.get_msg())
    finally:
        c.sc_report_handler.set_actions(c.SC_ERROR, previous)


def main():
    """Exercise all public constructors, operators, methods and lifetimes."""
    if len(sys.argv) > 1:
        error_mode(sys.argv[1])
        return

    emit("type", STRING.__cpp_name__, int(STRING.__cpp_name__ == "std::string"))
    empty = STRING()
    reserved = STRING(33)
    source = STRING("alpha")
    prefix = STRING("alphabet", 4)
    copied = STRING(source)
    emit("construct", "default", empty.length(), text(empty))
    emit("construct", "size", reserved.length(), text(reserved))
    emit("construct", "char_pointer", source.length(), text(source))
    emit("construct", "prefix", prefix.length(), text(prefix))
    emit("construct", "copy", copied.length(), text(copied))
    internal_names = ("rep", "test", "fmt_length")
    internal_mask = sum(int(hasattr(STRING, name)) << index for index, name in enumerate(internal_names))
    emit("visibility", internal_mask)

    copy_source = STRING("shared")
    copy_alias = STRING(copy_source)
    copy_alias.set(0, "S")
    emit("cow", "copy_ctor", text(copy_source), text(copy_alias))
    assignment_source = STRING("assigned")
    assignment_alias = STRING("old")
    assignment_alias.__assign__(assignment_source)
    assignment_alias.set(0, "A")
    emit("cow", "copy_assign", text(assignment_source), text(assignment_alias))

    assigned_pointer = STRING("old")
    pointer_result = assigned_pointer.__assign__("from-char-pointer")
    emit(
        "assign",
        "char_pointer",
        int(cppyy.addressof(pointer_result) == cppyy.addressof(assigned_pointer)),
        text(assigned_pointer),
    )
    assigned_object = STRING("old")
    object_source = STRING("from-object")
    object_result = assigned_object.__assign__(object_source)
    emit(
        "assign",
        "object",
        int(cppyy.addressof(object_result) == cppyy.addressof(assigned_object)),
        text(assigned_object),
    )

    appended_pointer = STRING("a")
    append_pointer_result = appended_pointer.__iadd__("bc")
    emit(
        "append",
        "char_pointer",
        int(cppyy.addressof(append_pointer_result) == cppyy.addressof(appended_pointer)),
        text(appended_pointer),
    )
    appended_char = STRING("a")
    append_char_result = appended_char.__iadd__("b")
    emit(
        "append",
        "char",
        int(cppyy.addressof(append_char_result) == cppyy.addressof(appended_char)),
        text(appended_char),
    )
    appended_object = STRING("a")
    object_suffix = STRING("bc")
    append_object_result = appended_object.__iadd__(object_suffix)
    emit(
        "append",
        "object",
        int(cppyy.addressof(append_object_result) == cppyy.addressof(appended_object)),
        text(appended_object),
    )

    base = STRING("alpha")
    suffix = STRING("bet")
    emit("concat", "char_pointer", text(base.__add__("bet")))
    emit("concat", "char", text(base.__add__("!")))
    emit("concat", "object", text(base.__add__(suffix)))
    emit("concat", "prefix", text("pre-" + suffix))

    lhs = STRING("beta")
    rhs = STRING("bet")
    emit("compare", "char_equal", "eq", int(lhs == "beta"))
    emit("compare", "char_unequal", "ne", int(lhs != "bet"))
    emit("compare", "char_lt", "lt", int(lhs < "zeta"))
    emit("compare", "char_le", "le", int(lhs <= "beta"))
    emit("compare", "char_gt", "gt", int(lhs > "alpha"))
    emit("compare", "char_ge", "ge", int(lhs >= "beta"))
    emit("compare", "object_equal", "eq", int(lhs == STRING("beta")))
    emit("compare", "object_unequal", "ne", int(lhs != rhs))
    emit("compare", "object_lt", "lt", int(lhs < STRING("zeta")))
    emit("compare", "object_le", "le", int(lhs <= STRING("beta")))
    emit("compare", "object_gt", "gt", int(lhs > STRING("alpha")))
    emit("compare", "object_ge", "ge", int(lhs >= STRING("beta")))
    emit(
        "cmp",
        "char_pointer",
        (lhs.cmp("beta") > 0) - (lhs.cmp("beta") < 0),
        (lhs.cmp("zeta") > 0) - (lhs.cmp("zeta") < 0),
        (lhs.cmp("alpha") > 0) - (lhs.cmp("alpha") < 0),
    )
    emit(
        "cmp",
        "object",
        (lhs.cmp(STRING("beta")) > 0) - (lhs.cmp(STRING("beta")) < 0),
        (lhs.cmp(STRING("zeta")) > 0) - (lhs.cmp(STRING("zeta")) < 0),
        (lhs.cmp(STRING("alpha")) > 0) - (lhs.cmp(STRING("alpha")) < 0),
    )

    indexed = STRING("index")
    emit("index", "const", PROBE.read_character_const(indexed, 2))
    emit("index", "mutable_read", indexed[3])
    indexed[1] = "X"
    emit("index", "mutable_reference", "n", "X", text(indexed))
    indexed.set(4, "!")
    emit("index", "set", text(indexed))

    substr_source = STRING("012345")
    for label, first, last in (
        ("middle", 2, 4),
        ("left_invalid", -1, 2),
        ("reversed", 4, 2),
        ("right_invalid", 3, 9),
    ):
        emit("substr", label, text(substr_source.substr(first, last)))
    edited = STRING("abcdef")
    inserted = edited.insert(STRING("XY"), 3)
    emit("insert", "middle", int(cppyy.addressof(inserted) == cppyy.addressof(edited)), text(edited))
    removed = edited.remove(2, 3)
    emit("remove", "middle", int(cppyy.addressof(removed) == cppyy.addressof(edited)), text(edited))
    append_end = STRING("abc")
    append_end.insert(STRING("!"), 3)
    emit("insert", "end", text(append_end))

    delimiter = STRING("-:/")
    scan_text = STRING("one-two")
    emit(
        "search",
        int(scan_text.contains("-")),
        int(scan_text.contains("z")),
        int(scan_text.is_delimiter(delimiter, 3)),
        int(scan_text.is_delimiter(delimiter, 1)),
        scan_text.pos(STRING("two")),
        scan_text.pos(STRING("missing")),
        scan_text.pos(STRING("")),
    )
    emit("length", scan_text.length(), text(scan_text), text(STRING(scan_text.c_str())))
    emit("conversion", str(scan_text))
    mixed_upper = STRING("MiXeD")
    upper_result = mixed_upper.uppercase()
    mixed_lower = STRING("MiXeD")
    lower_result = mixed_lower.lowercase()
    emit("case", text(upper_result), text(lower_result))
    emit("make_str", text(STRING.make_str(-123456789)))

    formatted = STRING("%s")
    formatted.fmt(STRING("object"))
    emit("fmt", "object", text(formatted))
    templated_int = STRING("value=%04d")
    templated_int.fmt(7)
    emit("fmt", "int", text(templated_int))
    templated_string = STRING("name=%s")
    templated_string.fmt("item")
    emit("fmt", "char_pointer", text(templated_string))
    chained_format = STRING("n=%04d s=%s")
    chained_format.fmt(7).fmt("ok")
    emit("fmt", "chain", text(chained_format))
    formatted_varargs = STRING.to_string("%s:%d:%.2f", "item", 17, 1.25)
    emit("format_varargs", text(formatted_varargs))
    dynamic = STRING.to_string("D" * 1300)
    emit("format_dynamic", 1300, dynamic.length(), dynamic[0], dynamic[dynamic.length() - 1])
    truncated = PROBE.formatted_dynamic(66000)
    emit("format_truncated", 66000, truncated[0], truncated[1], truncated[2])

    printed = cppyy.gbl.std.ostringstream()
    scan_text.print(printed)
    emit("print", printed.str())
    emit("ostream", PROBE.streamed_text(scan_text))
    extracted = PROBE.extracted_text("  first second")
    emit("istream", extracted.first, extracted.second)

    lifetime_source = STRING("borrowed representation")
    lifetime_ref = weakref.ref(lifetime_source)
    lifetime_copy = STRING(lifetime_source)
    del lifetime_source
    gc.collect()
    assert lifetime_ref() is None
    assert text(lifetime_copy) == "borrowed representation"
    emit("lifetime", PROBE.lifetime_roundtrip(64))


if __name__ == "__main__":
    main()
