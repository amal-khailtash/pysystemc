"""Compare all public sc_dt left shifts and ostream insertions with native C++."""

from pathlib import Path
import gc

from amal.eda.systemc import sc_dt as dt
import cppyy

NATIVE = Path(__file__).resolve().parents[1] / "native"
for _header in (
    "sc_dt_left_shift_probe.h",
    "sc_dt_integer_bitrefs_probe.h",
    "sc_dt_integer_subrefs_probe.h",
):
    cppyy.include(str(NATIVE / _header))
probe = cppyy.gbl.sc_dt_left_shift_probe
int_bits = cppyy.gbl.sc_dt_integer_bitrefs_probe
int_ranges = cppyy.gbl.sc_dt_integer_subrefs_probe


def decimal(value):
    """Copy a full-width native decimal representation into Python text."""
    text = value.to_string(dt.SC_DEC)
    return text.decode() if isinstance(text, bytes) else str(text)


def emit_shift(row, value):
    """Emit a shifted result's exact value and native width.

    Args:
        row: Inventory row number represented by the expression.
        value: Independently owned native shift result.
    """
    assert value.__python_owns__, f"shift result for row {row} must own independent storage"
    width = int(value.length()) if hasattr(value, "length") else -1
    print("LSM", row, "S", width, decimal(value), sep="\t")


def emit_stream(row, value):
    """Insert one live value and report text plus borrowed stream identity.

    Args:
        row: Inventory row number represented by the insertion overload.
        value: Native datatype value or read-only proxy with a live parent.
    """
    output = cppyy.gbl.std.ostringstream()
    returned = output << value
    assert cppyy.addressof(returned) == cppyy.addressof(output)
    text = output.str()
    if isinstance(text, bytes):
        text = text.decode()
    print("LSM", row, "O", text, 1, sep="\t")


def assert_type(value, expected):
    """Check that a runtime source has its inventoried native C++ type.

    Args:
        value: Python proxy or scalar passed to one public overload.
        expected: Native type name, including alias names for primitive counts.
    """
    type_map = {
        "sc_unsigned": dt.sc_unsigned,
        "sc_signed": dt.sc_signed,
        "sc_uint_base": dt.sc_uint_base,
        "sc_int_base": dt.sc_int_base,
        "sc_concatref": dt.sc_concatref,
        "sc_fxval": dt.sc_fxval,
        "sc_fxval_fast": dt.sc_fxval_fast,
        "sc_fxnum": cppyy.gbl.sc_dt.sc_fxnum,
        "sc_fxnum_fast": cppyy.gbl.sc_dt.sc_fxnum_fast,
        "sc_int_subref_r": dt.sc_int_subref_r,
        "sc_uint_subref_r": dt.sc_uint_subref_r,
        "sc_signed_subref_r": dt.sc_signed_subref_r,
        "sc_unsigned_subref_r": dt.sc_unsigned_subref_r,
        "sc_int_bitref_r": dt.sc_int_bitref_r,
        "sc_uint_bitref_r": dt.sc_uint_bitref_r,
        "sc_signed_bitref_r": dt.sc_signed_bitref_r,
        "sc_unsigned_bitref_r": dt.sc_unsigned_bitref_r,
        "sc_fxnum_bitref_r": dt.sc_fxnum_bitref_r,
        "sc_fxnum_fast_bitref_r": dt.sc_fxnum_fast_bitref_r,
        "sc_fxnum_subref": dt.sc_fxnum_subref,
        "sc_fxnum_fast_subref_r": dt.sc_fxnum_fast_subref_r,
        "sc_length_param": dt.sc_length_param,
        "sc_fxtype_params": dt.sc_fxtype_params,
        "sc_fxcast_switch": dt.sc_fxcast_switch,
        "sc_bit": dt.sc_bit,
        "sc_logic": dt.sc_logic,
        "sc_string_old": dt.sc_string_old,
        "sc_lv_base": dt.sc_lv_base,
        "sc_bv_base": dt.sc_bv_base,
    }
    if expected == "int":
        assert type(value) is int
    elif expected == "sc_dt::int64":
        assert type(value).__cpp_name__ == expected
    elif expected == "sc_dt::uint64":
        assert type(value).__cpp_name__ == expected
    elif expected == "long":
        assert type(value).__cpp_name__ == expected
    elif expected == "unsigned long":
        assert type(value).__cpp_name__ == "sc_dt_left_shift_probe::ulong"
    elif expected == "unsigned int":
        assert type(value).__cpp_name__ == "sc_dt_left_shift_probe::uint"
    elif expected in {"sc_o_mode", "sc_numrep", "sc_q_mode", "sc_fmt", "sc_enc", "sc_switch"}:
        assert type(value).__cpp_name__ == f"sc_dt::{expected}"
    elif expected == "sc_proxy":
        assert type(value).__cpp_name__.startswith("sc_dt::sc_subref<")
    else:
        assert isinstance(value, type_map[expected]), (
            f"expected {expected}, got {getattr(type(value), '__cpp_name__', type(value))}"
        )


unsigned_value = dt.sc_unsigned(257)
unsigned_value.__assign__("0x123456789abcdef001122334455667788")
signed_value = dt.sc_signed(191)
signed_value.__assign__("0x123456789abcdef123456789abcdef")
signed_value.__assign__(-signed_value)
unsigned_count = dt.sc_unsigned(137)
unsigned_count.__assign__(3)
signed_count = dt.sc_signed(137)
signed_count.__assign__(2)
signed_base_count = dt.sc_int[64](5)
unsigned_base_count = dt.sc_uint[64](6)
int64_count = dt.int64(4)
uint64_count = dt.uint64(5)
long_count = cppyy.gbl.long(6)
ulong_count = probe.ulong(7)
int_count = 2
uint_count = probe.uint(3)
concatenated = probe.concat(unsigned_value, signed_value)

# Keep this table in inventory order by row id. Repeated overload declarations
# from sc_signed.h/sc_unsigned.h intentionally have separate observations.
for _row, _left, _right in (
    (1, unsigned_value, signed_base_count),
    (2, unsigned_value, unsigned_count),
    (5, unsigned_value, int64_count),
    (9, unsigned_value, ulong_count),
    (11, signed_value, ulong_count),
    (17, concatenated, ulong_count),
    (20, signed_value, uint_count),
    (26, signed_value, int64_count),
    (29, signed_value, signed_count),
    (30, signed_value, unsigned_count),
    (31, unsigned_value, signed_count),
    (35, signed_value, long_count),
    (36, concatenated, long_count),
    (37, concatenated, int_count),
    (38, concatenated, int64_count),
    (39, concatenated, uint64_count),
    (41, signed_value, uint64_count),
    (43, unsigned_value, signed_count),
    (45, signed_value, unsigned_count),
    (46, unsigned_value, unsigned_base_count),
    (50, signed_value, signed_base_count),
    (52, unsigned_value, long_count),
    (55, signed_value, int_count),
    (6, dt.sc_fxval(3.25), 3),
    (56, probe.fixed_value[dt.sc_fix](-3.25), 2),
    (58, dt.sc_fxval_fast(2.5), 4),
    (59, signed_value, unsigned_base_count),
    (61, unsigned_value, uint64_count),
    (63, unsigned_value, uint_count),
    (64, probe.fixed_value[dt.sc_fix_fast](1.75), 3),
    (66, concatenated, uint_count),
    (67, unsigned_value, int_count),
):
    _left_type, _right_type = {
        1: ("sc_unsigned", "sc_int_base"),
        2: ("sc_unsigned", "sc_unsigned"),
        5: ("sc_unsigned", "sc_dt::int64"),
        9: ("sc_unsigned", "unsigned long"),
        11: ("sc_signed", "unsigned long"),
        17: ("sc_concatref", "unsigned long"),
        20: ("sc_signed", "unsigned int"),
        26: ("sc_signed", "sc_dt::int64"),
        29: ("sc_signed", "sc_signed"),
        30: ("sc_signed", "sc_unsigned"),
        31: ("sc_unsigned", "sc_signed"),
        35: ("sc_signed", "long"),
        36: ("sc_concatref", "long"),
        37: ("sc_concatref", "int"),
        38: ("sc_concatref", "sc_dt::int64"),
        39: ("sc_concatref", "sc_dt::uint64"),
        41: ("sc_signed", "sc_dt::uint64"),
        43: ("sc_unsigned", "sc_signed"),
        45: ("sc_signed", "sc_unsigned"),
        46: ("sc_unsigned", "sc_uint_base"),
        50: ("sc_signed", "sc_int_base"),
        52: ("sc_unsigned", "long"),
        55: ("sc_signed", "int"),
        6: ("sc_fxval", "int"),
        56: ("sc_fxnum", "int"),
        58: ("sc_fxval_fast", "int"),
        59: ("sc_signed", "sc_uint_base"),
        61: ("sc_unsigned", "sc_dt::uint64"),
        63: ("sc_unsigned", "unsigned int"),
        64: ("sc_fxnum_fast", "int"),
        66: ("sc_concatref", "unsigned int"),
        67: ("sc_unsigned", "int"),
    }[_row]
    assert_type(_left, _left_type)
    assert_type(_right, _right_type)
    emit_shift(_row, _left << _right)

int_value = dt.sc_int[32](-29)
uint_value = dt.sc_uint[32](0xABCDEF)
fx_owner = probe.fixed_value[dt.sc_fix](-7.5)
fx_fast_owner = probe.fixed_value[dt.sc_fix_fast](3.75)
const_signed_bit = int_bits.const_signed_view(signed_value, 3)
const_unsigned_bit = int_bits.const_unsigned_view(unsigned_value, 4)
const_fast_fixed_bit = probe.fixed_fast_bit(fx_fast_owner, 3)
const_fixed_bit = probe.fixed_bit(fx_owner, 3)
const_fixed_range = probe.fixed_range(fx_owner, 8, 1)
const_fast_fixed_range = probe.fixed_fast_range(fx_fast_owner, 8, 1)
const_int_range = int_ranges.const_int_range(int_value, 7, 2)
const_uint_range = int_ranges.const_uint_range(uint_value, 7, 2)
const_signed_range = int_ranges.const_signed_range(signed_value, 8, 2)
const_unsigned_range = int_ranges.const_unsigned_range(unsigned_value, 8, 2)
length = dt.sc_length_param(9)
fx_params = dt.sc_fxtype_params(19, 8, dt.SC_RND, dt.SC_SAT, 0)
cast_switch = dt.sc_fxcast_switch(dt.SC_ON)
legacy_string = dt.sc_string_old("old-string")
bit = dt.sc_bit(True)
logic = dt.sc_logic(dt.SC_LOGIC_Z)
proxy_bits = dt.sc_bv[12]("101010110011")

for _row, _value in (
    (3, dt.SC_WRAP),
    (4, int_bits.const_uint_view(uint_value, 4)),
    (7, uint_value),
    (10, fx_fast_owner),
    (12, const_int_range),
    (13, const_signed_bit),
    (14, const_fast_fixed_bit),
    (15, int_bits.const_int_view(int_value, 2)),
    (16, const_uint_range),
    (18, const_fixed_bit),
    (19, length),
    (21, const_unsigned_bit),
    (22, dt.sc_fxval(3.25)),
    (23, dt.SC_DEC),
    (24, dt.SC_RND),
    (25, bit),
    (27, const_fast_fixed_range),
    (28, dt.SC_F),
    (32, fx_owner),
    (33, dt.SC_TC_),
    (34, dt.sc_fxval_fast(6.5)),
    (40, cast_switch),
    (42, const_signed_range),
    (44, int_value),
    # Row 8 is the structural template declaration also instantiated for the
    # callable public row 47 below; retain its separate inventory observation.
    (8, proxy_bits),
    (47, proxy_bits),
    (48, unsigned_value),
    (49, concatenated),
    (51, logic),
    (53, const_unsigned_range),
    (54, fx_params),
    (57, legacy_string),
    (60, dt.SC_OFF),
    (62, signed_value),
    (65, const_fixed_range),
):
    stream_types = {
        3: "sc_o_mode",
        4: "sc_uint_bitref_r",
        7: "sc_uint_base",
        10: "sc_fxnum_fast",
        12: "sc_int_subref_r",
        13: "sc_signed_bitref_r",
        14: "sc_fxnum_fast_bitref_r",
        15: "sc_int_bitref_r",
        16: "sc_uint_subref_r",
        18: "sc_fxnum_bitref_r",
        19: "sc_length_param",
        21: "sc_unsigned_bitref_r",
        22: "sc_fxval",
        23: "sc_numrep",
        24: "sc_q_mode",
        25: "sc_bit",
        27: "sc_fxnum_fast_subref_r",
        28: "sc_fmt",
        32: "sc_fxnum",
        33: "sc_enc",
        34: "sc_fxval_fast",
        40: "sc_fxcast_switch",
        42: "sc_signed_subref_r",
        44: "sc_int_base",
        8: "sc_bv_base",
        47: "sc_bv_base",
        48: "sc_unsigned",
        49: "sc_concatref",
        51: "sc_logic",
        53: "sc_unsigned_subref_r",
        54: "sc_fxtype_params",
        57: "sc_string_old",
        60: "sc_switch",
        62: "sc_signed",
        65: "sc_fxnum_subref",
    }[_row]
    assert_type(_value, stream_types)
    emit_stream(_row, _value)

lv137_text = ("10XZ" * 35)[:137]
lv191_text = ("ZX01" * 48)[:191]
lv137 = dt.sc_lv[137](lv137_text)
lv191 = dt.sc_lv[191](lv191_text)
emit_stream(910, lv137)
emit_stream(911, lv191)
nested_parent = lv137.range(120, 80)
nested_proxy = nested_parent.range(30, 5)
vector_range = lv191.range(150, 100)
assert_type(nested_proxy, "sc_proxy")
assert_type(vector_range, "sc_proxy")
emit_stream(912, nested_proxy)
emit_stream(913, vector_range)

# A negative count preserves integer values; a positive wide count grows the
# arbitrary-width result according to native SystemC rules.
emit_shift(901, unsigned_value << -3)
emit_shift(902, unsigned_value << 257)
emit_shift(903, signed_value << -4)
emit_shift(904, signed_value << 257)
width137_value = dt.sc_unsigned(137)
width137_value.__assign__("0x123456789abcdef00123456789abcdef")
emit_shift(905, width137_value << 3)

life_source = dt.sc_unsigned(137)
life_source.__assign__(13)
life_result = life_source << 4
assert cppyy.addressof(life_result) != cppyy.addressof(life_source)
life_source.__assign__(0)
del life_source
gc.collect()
print("LSM", 900, "L", int(life_result.length()), decimal(life_result), sep="\t")

flags_value = dt.sc_signed(32)
flags_value.__assign__(255)
flags_output = cppyy.gbl.std.ostringstream()
flags_output.setf(cppyy.gbl.std.ios_base.hex, cppyy.gbl.std.ios_base.basefield)
flags_return = flags_output << flags_value
assert cppyy.addressof(flags_return) == cppyy.addressof(flags_output)
flags_text = flags_output.str()
if isinstance(flags_text, bytes):
    flags_text = flags_text.decode()
print("LSM", 906, "F", flags_text, 1, sep="\t")

chained_output = cppyy.gbl.std.ostringstream()
chained_bit = dt.sc_bit(True)
chained_string = dt.sc_string_old("tail")
first_return = chained_output << chained_bit
second_return = first_return << chained_string
assert cppyy.addressof(first_return) == cppyy.addressof(chained_output)
assert cppyy.addressof(second_return) == cppyy.addressof(chained_output)
chained_text = chained_output.str()
if isinstance(chained_text, bytes):
    chained_text = chained_text.decode()
print("LSM", 907, "C", chained_text, 1, sep="\t")

masked_output = cppyy.gbl.std.ostringstream()
badbit = cppyy.gbl.std.ios_base.badbit
masked_output.setstate(badbit)
mask_setup_threw = False
try:
    masked_output.exceptions(badbit)
except cppyy.gbl.std.exception as error:
    assert "exception" in str(error)
    mask_setup_threw = True
insertion_threw = False
try:
    masked_output << chained_bit
except cppyy.gbl.std.exception as error:
    assert "exception" in str(error)
    insertion_threw = True
assert mask_setup_threw and insertion_threw
print("LSM", 908, "E", int(mask_setup_threw), int(insertion_threw), sep="\t")

zero_count_result = unsigned_value << 0
print("LSM", 909, "Z", int(zero_count_result.length()), decimal(zero_count_result), sep="\t")

print("SC_DT_LEFT_SHIFT_PUBLIC_OK")
