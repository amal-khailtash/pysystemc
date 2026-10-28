"""Exercise public right shifts and native stream extraction overloads."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_right_shift_probe.h"))
probe = cppyy.gbl.sc_dt_right_shift_probe
COUNT_CPP_TYPES = {
    "int": "int",
    "unsigned_int": "sc_dt_right_shift_probe::UnsignedIntCount",
    "long": "sc_dt_right_shift_probe::LongCount",
    "ulong": "sc_dt_right_shift_probe::UnsignedLongCount",
    "int64": "sc_dt_right_shift_probe::Int64Count",
    "uint64": "sc_dt_right_shift_probe::UInt64Count",
    "unsigned_long": "sc_dt_right_shift_probe::UnsignedLongCount",
    "signed": "sc_dt::sc_signed",
    "unsigned": "sc_dt::sc_unsigned",
    "sc_int": "sc_dt::sc_int<16>",
    "sc_uint": "sc_dt::sc_uint<16>",
}
STREAM_CPP_PREFIXES = {
    "signed": "sc_dt::sc_signed",
    "unsigned": "sc_dt::sc_unsigned",
    "sc_int_base": "sc_dt::sc_int<16>",
    "sc_uint_base": "sc_dt::sc_uint<16>",
    "sc_bv_proxy": "sc_dt::sc_bv<8>",
    "sc_lv_proxy": "sc_dt::sc_lv<8>",
    "sc_bit": "sc_dt::sc_bit",
    "sc_logic": "sc_dt::sc_logic",
    "sc_fxval": "sc_dt::sc_fxval",
    "sc_fxval_fast": "sc_dt::sc_fxval_fast",
    "sc_fxnum": "sc_dt::sc_fixed<16,8,",
    "sc_fxnum_fast": "sc_dt::sc_fixed_fast<16,8,",
    "sc_string_old": "sc_dt::sc_string_old",
    "sc_signed_bitref": "sc_dt::sc_signed_bitref",
    "sc_unsigned_bitref": "sc_dt::sc_unsigned_bitref",
    "sc_int_bitref": "sc_dt::sc_int_bitref",
    "sc_uint_bitref": "sc_dt::sc_uint_bitref",
    "sc_bitref_template": "sc_dt::sc_bitref<",
    "sc_logic_bitref": "sc_dt::sc_bitref<",
    "sc_signed_subref": "sc_dt::sc_signed_subref",
    "sc_unsigned_subref": "sc_dt::sc_unsigned_subref",
    "sc_int_subref": "sc_dt::sc_int_subref",
    "sc_uint_subref": "sc_dt::sc_uint_subref",
    "sc_subref_template": "sc_dt::sc_subref<",
    "sc_logic_subref": "sc_dt::sc_subref<",
    "sc_fxnum_bitref": "sc_dt::sc_fxnum_bitref",
    "sc_fxnum_fast_bitref": "sc_dt::sc_fxnum_fast_bitref",
    "sc_fxnum_subref": "sc_dt::sc_fxnum_subref",
    "sc_fxnum_fast_subref": "sc_dt::sc_fxnum_fast_subref",
    "sc_concref": "sc_dt::sc_concref<sc_dt::sc_bv_base,sc_dt::sc_bv_base>",
    "sc_concatref": "sc_dt::sc_concatref",
}


def render(value):
    """Copy text through an exact class-static native insertion specialization.

    Args:
        value: Borrowed native datatype or proxy with a live owner.
    Returns:
        Independent Python text copied before the native stream is destroyed.
    """
    return str(probe.Text[type(value)].get(value))


def cpp_name(value):
    """Return cppyy's exact native type spelling for a Python value."""
    return getattr(type(value), "__cpp_name__", "int" if type(value) is int else "")


def assert_shift_types(label, value, count):
    """Assert that a public shift label uses its declared source and count types."""
    if label.startswith("signed_"):
        expected_source = "sc_dt::sc_signed"
        count_label = label.split("_", 1)[1]
    elif label.startswith("unsigned_"):
        expected_source = "sc_dt::sc_unsigned"
        count_label = label.split("_", 1)[1]
    elif label.startswith("concat_"):
        expected_source = "sc_dt::sc_concatref"
        count_label = label.split("_", 1)[1]
    elif label.startswith("fxval_fast_"):
        expected_source, count_label = "sc_dt::sc_fxval_fast", "int"
    elif label.startswith("fxval_"):
        expected_source, count_label = "sc_dt::sc_fxval", "int"
    elif label.startswith("fxnum_fast_"):
        expected_source, count_label = "sc_dt::sc_fixed_fast<32,16,", "int"
    elif label.startswith("fxnum_"):
        expected_source, count_label = "sc_dt::sc_fixed<32,16,", "int"
    elif label.startswith("bigint_"):
        width = label.rsplit("_", 1)[1]
        expected_source, count_label = f"sc_dt::sc_bigint<{width}>", "int"
    else:
        width = label.rsplit("_", 1)[1]
        expected_source, count_label = f"sc_dt::sc_biguint<{width}>", "int"
    actual_source = cpp_name(value)
    if expected_source.endswith(","):
        assert actual_source.startswith(expected_source), (label, actual_source, expected_source)
    else:
        assert actual_source == expected_source, (label, actual_source, expected_source)
    expected_count = COUNT_CPP_TYPES.get(count_label, "int")
    assert cpp_name(count) == expected_count, (label, cpp_name(count), expected_count)
    if count_label in COUNT_CPP_TYPES and count_label not in {"int", "zero", "negative", "large"}:
        assert count == 2, (label, count)


def assert_stream_type(label, value):
    """Assert a concrete Python target maps to the intended public native type."""
    actual = cpp_name(value)
    expected = STREAM_CPP_PREFIXES[label]
    if expected.endswith(("<", ",")):
        assert actual.startswith(expected), (label, actual, expected)
    else:
        assert actual == expected, (label, actual, expected)


def emit_shift(label, value, count):
    """Emit and validate one owning native right-shift result.

    Args:
        label: Exact native source/count family under test.
        value: Borrowed live native source.
        count: Python integer or live native SystemC count object.
    """
    assert_shift_types(label, value, count)
    source_before = render(value)
    result = value >> count
    assert render(value) == source_before, "a value shift changed its source"
    assert bool(result.__python_owns__), "a value shift must return independent storage"
    length = result.length() if hasattr(result, "length") else -1
    print("RS\t" + "\t".join(map(str, (label, render(result), length))))


def emit_parent(label, parent, before):
    """Emit a live-parent mutation observed through a previously made proxy.

    Args:
        label: Proxy family whose extraction changed the parent.
        parent: Native owner retained by the caller.
        before: Owner text captured before extraction.
    """
    after = render(parent)
    assert before != after, "proxy extraction must mutate its live parent"
    print("PAR\t" + "\t".join((label, before, after)))


def emit_extraction(label, value, token, parents=()):
    """Extract one token and check the live stream reference and flags.

    Args:
        label: Native extraction target family.
        value: Mutable native value or view with its parent kept alive.
        token: Valid native input spelling for the target.
        parents: Optional ``(label, owner)`` entries whose mutation is checked.
    """
    assert_stream_type(label, value)
    before = render(value)
    parent_before = [(parent_label, owner, render(owner)) for parent_label, owner in parents]
    stream = cppyy.gbl.std.istringstream(f"{token} 99")
    returned = stream >> value
    same_stream = returned is stream
    good_state = int(stream.rdstate())
    good = bool(stream)
    after = render(value)
    assert before != after, "valid stream extraction must change its target"
    stream.setstate(cppyy.gbl.std.ios.failbit)
    failed = bool(stream)
    failed_state = int(stream.rdstate())
    assert before != after, "valid stream extraction must change its target"
    assert same_stream, "native extraction must return the original borrowed stream"
    assert good and good_state == 0, "valid extraction must preserve a good stream state"
    assert not failed and failed_state == int(cppyy.gbl.std.ios.failbit)
    print(
        "SE\t"
        + "\t".join(
            map(str, (label, before, after, int(same_stream), int(good), good_state, int(failed), failed_state))
        )
    )
    for parent_label, owner, old_text in parent_before:
        emit_parent(parent_label, owner, old_text)


signed = dt.sc_signed(64)
signed.__assign__("-1234567890123")
unsigned = dt.sc_unsigned(64)
unsigned.__assign__("1234567890123")
signed_count = dt.sc_signed(16)
signed_count.__assign__(2)
unsigned_count = dt.sc_unsigned(16)
unsigned_count.__assign__(2)
int_count = dt.sc_int[16](2)
uint_count = dt.sc_uint[16](2)

for name, count in (
    ("signed", signed_count),
    ("unsigned", unsigned_count),
    ("int64", probe.Int64Count(2)),
    ("uint64", probe.UInt64Count(2)),
    ("long", probe.LongCount(2)),
    ("unsigned_long", probe.UnsignedLongCount(2)),
    ("int", 2),
    ("unsigned_int", probe.UnsignedIntCount(2)),
    ("sc_int", int_count),
    ("sc_uint", uint_count),
):
    native_name = "ulong" if name == "unsigned_long" else name
    emit_shift("signed_" + native_name, signed, count)
for label, count in (("zero", 0), ("negative", -1), ("large", 257)):
    emit_shift("signed_" + label, signed, count)
for name, count in (
    ("signed", signed_count),
    ("unsigned", unsigned_count),
    ("int64", probe.Int64Count(2)),
    ("uint64", probe.UInt64Count(2)),
    ("long", probe.LongCount(2)),
    ("unsigned_long", probe.UnsignedLongCount(2)),
    ("int", 2),
    ("unsigned_int", probe.UnsignedIntCount(2)),
    ("sc_int", int_count),
    ("sc_uint", uint_count),
):
    native_name = "ulong" if name == "unsigned_long" else name
    emit_shift("unsigned_" + native_name, unsigned, count)
for label, count in (("zero", 0), ("negative", -1), ("large", 257)):
    emit_shift("unsigned_" + label, unsigned, count)

concat_left, concat_right = dt.sc_uint[16](0x1234), dt.sc_uint[16](0x5678)
concat_source = dt.concat(concat_left, concat_right)
for name, count in (
    ("int", 2),
    ("unsigned_int", probe.UnsignedIntCount(2)),
    ("long", probe.LongCount(2)),
    ("unsigned_long", probe.UnsignedLongCount(2)),
    ("int64", probe.Int64Count(2)),
    ("uint64", probe.UInt64Count(2)),
):
    emit_shift("concat_" + name, concat_source, count)
for label, count in (("zero", 0), ("large", 257)):
    emit_shift("concat_" + label, concat_source, count)

fx_value = dt.sc_fxval(123.75)
fx_fast_value = dt.sc_fxval_fast(123.75)
fixed_value = dt.sc_fixed[32, 16](123.75)
fixed_fast_value = dt.sc_fixed_fast[32, 16](123.75)
for label, value in (
    ("fxval_int", fx_value),
    ("fxval_fast_int", fx_fast_value),
    ("fxnum_int", fixed_value),
    ("fxnum_fast_int", fixed_fast_value),
):
    emit_shift(label, value, 2)
for suffix, count in (("zero", 0), ("negative", -1), ("large", 16)):
    for prefix, value in (
        ("fxval", fx_value),
        ("fxval_fast", fx_fast_value),
        ("fxnum", fixed_value),
        ("fxnum_fast", fixed_fast_value),
    ):
        emit_shift(f"{prefix}_{suffix}", value, count)

for width in (137, 191, 257):
    signed_number = -(1 << (width - 2)) + 12345
    unsigned_number = (1 << (width - 2)) + 23456
    signed_template = dt.sc_bigint[width](str(signed_number))
    unsigned_template = dt.sc_biguint[width](str(unsigned_number))
    emit_shift(f"bigint_{width}", signed_template, 3)
    emit_shift(f"biguint_{width}", unsigned_template, 3)

signed_target = dt.sc_signed(16)
unsigned_target = dt.sc_unsigned(16)
int_target = dt.sc_int[16](0)
uint_target = dt.sc_uint[16](0)
bit_vector = dt.sc_bv[8]("00000000")
bit_vector_right = dt.sc_bv[8]("00000000")
logic_vector = dt.sc_lv[8]("00000000")
bit_target = dt.sc_bit("0")
logic_target = dt.sc_logic("0")
fx_target = dt.sc_fxval(0)
fx_fast_target = dt.sc_fxval_fast(0)
fixed_target = dt.sc_fixed[16, 8](0)
fixed_fast_target = dt.sc_fixed_fast[16, 8](0)
string_target = dt.sc_string_old("old")

for label, target, token in (
    ("signed", signed_target, "1"),
    ("unsigned", unsigned_target, "1"),
    ("sc_int_base", int_target, "1"),
    ("sc_uint_base", uint_target, "1"),
    ("sc_bv_proxy", bit_vector, "1"),
    ("sc_lv_proxy", logic_vector, "1"),
    ("sc_bit", bit_target, "1"),
    ("sc_logic", logic_target, "1"),
    ("sc_fxval", fx_target, "1"),
    ("sc_fxval_fast", fx_fast_target, "1"),
    ("sc_fxnum", fixed_target, "1"),
    ("sc_fxnum_fast", fixed_fast_target, "1"),
    ("sc_string_old", string_target, "one"),
):
    emit_extraction(label, target, token)

signed_bit = signed_target[0]
unsigned_bit = unsigned_target[0]
int_bit = int_target[0]
uint_bit = uint_target[0]
vector_bit = bit_vector[0]
logic_vector_bit = logic_vector[0]
signed_range = signed_target(3, 0)
unsigned_range = unsigned_target(3, 0)
int_range = int_target(3, 0)
uint_range = uint_target(3, 0)
vector_range = bit_vector(3, 0)
logic_vector_range = logic_vector(3, 0)
fixed_bit = fixed_target.bit(0)
fixed_fast_bit = fixed_fast_target.bit(0)
fixed_range = fixed_target(3, 0)
fixed_fast_range = fixed_fast_target(3, 0)
for item in (
    ("sc_signed_bitref", signed_bit, "0", (("sc_signed_bitref", signed_target),)),
    ("sc_unsigned_bitref", unsigned_bit, "0", (("sc_unsigned_bitref", unsigned_target),)),
    ("sc_int_bitref", int_bit, "0", (("sc_int_bitref", int_target),)),
    ("sc_uint_bitref", uint_bit, "0", (("sc_uint_bitref", uint_target),)),
    ("sc_bitref_template", vector_bit, "0", (("sc_bitref_template", bit_vector),)),
    ("sc_logic_bitref", logic_vector_bit, "0", (("sc_logic_bitref", logic_vector),)),
    ("sc_signed_subref", signed_range, "3", (("sc_signed_subref", signed_target),)),
    ("sc_unsigned_subref", unsigned_range, "3", (("sc_unsigned_subref", unsigned_target),)),
    ("sc_int_subref", int_range, "3", (("sc_int_subref", int_target),)),
    ("sc_uint_subref", uint_range, "3", (("sc_uint_subref", uint_target),)),
    ("sc_subref_template", vector_range, "0011", (("sc_subref_template", bit_vector),)),
    ("sc_logic_subref", logic_vector_range, "0011", (("sc_logic_subref", logic_vector),)),
    ("sc_fxnum_bitref", fixed_bit, "1", (("sc_fxnum_bitref", fixed_target),)),
    ("sc_fxnum_fast_bitref", fixed_fast_bit, "1", (("sc_fxnum_fast_bitref", fixed_fast_target),)),
    ("sc_fxnum_subref", fixed_range, "0010", (("sc_fxnum_subref", fixed_target),)),
    ("sc_fxnum_fast_subref", fixed_fast_range, "0010", (("sc_fxnum_fast_subref", fixed_fast_target),)),
):
    label, target, token = item[:3]
    parents = item[3] if len(item) == 4 else ()
    emit_extraction(label, target, token, parents)

concrete_type = dt.sc_concref[dt.sc_bv_base, dt.sc_bv_base]
concrete = concrete_type(bit_vector, bit_vector_right)
emit_extraction(
    "sc_concref",
    concrete,
    "10101010",
    (("sc_concref_left", bit_vector), ("sc_concref_right", bit_vector_right)),
)
concat_target = dt.concat(unsigned_target, uint_target)
emit_extraction(
    "sc_concatref",
    concat_target,
    "123",
    (("sc_concatref_left", unsigned_target), ("sc_concatref_right", uint_target)),
)

handler = cppyy.gbl.sc_core.sc_report_handler
old_actions = handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, cppyy.gbl.sc_core.SC_THROW)
signed_failure = dt.sc_signed(16)
signed_failure.__assign__(37)
signed_failure_before = render(signed_failure)
conversion_input = cppyy.gbl.std.istringstream("")
conversion_report = ""
try:
    conversion_input >> signed_failure
except cppyy.gbl.sc_core.sc_report as report:
    conversion_report = str(report.get_msg_type())
finally:
    handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, old_actions)
assert conversion_report == str(cppyy.gbl.sc_core.SC_ID_CONVERSION_FAILED_)
assert render(signed_failure) == signed_failure_before
assert int(conversion_input.rdstate()) == 6
print("FAIL\tsigned_empty_conversion\t1\t" + "\t".join((render(signed_failure), "6", conversion_report)))

unsigned_failure = dt.sc_unsigned(16)
unsigned_failure.__assign__(37)
unsigned_failure_before = render(unsigned_failure)
masked_input = cppyy.gbl.std.istringstream("")
masked_input.exceptions(cppyy.gbl.std.ios.failbit)
masked_diagnostic = ""
try:
    masked_input >> unsigned_failure
except cppyy.gbl.std.exception as error:
    masked_diagnostic = str(error.what())
masked_state = int(masked_input.rdstate())
assert masked_diagnostic == "std::exception"
assert render(unsigned_failure) == unsigned_failure_before
assert masked_state == 6
print("FAIL\tunsigned_masked_empty\t1\t" + "\t".join((render(unsigned_failure), str(masked_state), masked_diagnostic)))

old_string_failure = dt.sc_string_old("old")
string_input = cppyy.gbl.std.istringstream("")
string_return = string_input >> old_string_failure
assert string_return is string_input
assert render(old_string_failure) == ""
assert int(string_input.rdstate()) == 6 and not bool(string_input)
print("FAIL\tstring_empty\t1\t\t6\t0")

lifetime_value = dt.sc_unsigned(137)
lifetime_value.__assign__("123456789012345678901234567890123456789")
lifetime_result = lifetime_value >> 17
lifetime_text = render(lifetime_result)
del lifetime_value
gc.collect()
assert render(lifetime_result) == lifetime_text and lifetime_result.__python_owns__
print("LIFE\t" + lifetime_text + "\t" + str(lifetime_result.length()))

print("SC_DT_RIGHT_SHIFT_PUBLIC_OK")
