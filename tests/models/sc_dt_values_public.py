"""Compare small public sc_dt enums, constants, stream helpers and aliases."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_values_probe.h"))

PROBE = cppyy.gbl.sc_dt_values_probe
ENUMS = {
    "sc_enc": ("SC_TC_", "SC_US_"),
    "sc_fmt": ("SC_F", "SC_E"),
    "sc_logic_value_t": ("Log_0", "Log_1", "Log_Z", "Log_X"),
    "sc_numrep": (
        "SC_NOBASE",
        "SC_BIN",
        "SC_OCT",
        "SC_DEC",
        "SC_HEX",
        "SC_BIN_US",
        "SC_OCT_US",
        "SC_HEX_US",
        "SC_BIN_SM",
        "SC_OCT_SM",
        "SC_HEX_SM",
        "SC_CSD",
    ),
    "sc_o_mode": ("SC_SAT", "SC_SAT_ZERO", "SC_SAT_SYM", "SC_WRAP", "SC_WRAP_SM"),
    "sc_q_mode": (
        "SC_RND",
        "SC_RND_ZERO",
        "SC_RND_MIN_INF",
        "SC_RND_INF",
        "SC_RND_CONV",
        "SC_TRN",
        "SC_TRN_ZERO",
    ),
    "sc_switch": ("SC_OFF", "SC_ON"),
}
FORMATTERS = {
    "sc_enc": PROBE.format_enc,
    "sc_fmt": PROBE.format_fmt,
    "sc_numrep": PROBE.format_numrep,
    "sc_o_mode": PROBE.format_o_mode,
    "sc_q_mode": PROBE.format_q_mode,
    "sc_switch": PROBE.format_switch,
}
LOGIC_CONSTANTS = (
    ("SC_LOGIC_0", 0, 0),
    ("SC_LOGIC_1", 0, 1),
    ("SC_LOGIC_X", 0, 2),
    ("SC_LOGIC_Z", 0, 3),
    ("sc_logic_0", 1, 0),
    ("sc_logic_1", 1, 1),
    ("sc_logic_X", 1, 2),
    ("sc_logic_Z", 1, 3),
)


def test_enum_values_and_formats():
    """Exercise every public scalar enum member through its exact native type."""
    for type_name, member_names in ENUMS.items():
        enum_type = getattr(dt, type_name)
        assert enum_type.__cpp_name__ == f"sc_dt::{type_name}"
        for member_name in member_names:
            value = getattr(dt, member_name)
            native_value = int(PROBE.enum_value[enum_type](value))
            assert native_value == int(value)
            print(f"SC_DT_ENUM {type_name} {member_name} {native_value}")
        if type_name == "sc_logic_value_t":
            assert int(dt.SC_NOW) == 0 and int(dt.SC_LATER) == 1
            print(f"SC_DT_CONTEXT_ENUM SC_NOW {int(dt.SC_NOW)} SC_LATER {int(dt.SC_LATER)}")

    for type_name in ("sc_enc", "sc_fmt", "sc_numrep", "sc_o_mode", "sc_q_mode", "sc_switch"):
        for member_name in ENUMS[type_name]:
            value = getattr(dt, member_name)
            expected_format = str(FORMATTERS[type_name](value))
            public_format = str(dt.to_string(value))
            assert public_format == expected_format, (
                type_name,
                member_name,
                public_format,
                expected_format,
            )
            print(f"SC_DT_FORMAT {type_name} {member_name} {expected_format}")
            print(f"SC_DT_PUBLIC_FORMAT {type_name} {member_name} {public_format}")


def test_logic_constant_values_and_identity():
    """Check all named logic globals, their enum values and their exact symbols."""
    addresses = []
    values = []
    for name, group, index in LOGIC_CONSTANTS:
        value = getattr(dt, name)
        assert type(value).__cpp_name__ == "sc_dt::sc_logic"
        native_code = int(PROBE.logic_value(value))
        expected_char = str(PROBE.format_logic(value.value()))
        assert str(value) == expected_char
        expected_address = int(PROBE.logic_address(group, index))
        assert cppyy.addressof(value) == expected_address
        addresses.append(expected_address)
        values.append(native_code)
        print(f"SC_DT_LOGIC {name} {native_code} {expected_char}")

    assert len(set(addresses)) == 8
    assert values[:4] == values[4:]
    print("SC_DT_LOGIC_IDENTITIES distinct=1 aliases_equal=1")


def test_stream_manipulators():
    """Exercise sc_io_base and sc_io_show_base on configured native streams."""
    for base, label in ((10, "dec"), (8, "oct"), (16, "hex")):
        for show_base in (False, True):
            stream = cppyy.gbl.std.ostringstream()
            PROBE.configure_stream(stream, base, show_base)
            selected_base = int(dt.sc_io_base(stream, dt.SC_BIN))
            selected_show_base = bool(dt.sc_io_show_base(stream))
            assert selected_base == base
            assert selected_show_base is show_base
            print(f"SC_DT_IO {label} show={int(show_base)} base={selected_base} show_result={int(selected_show_base)}")


def test_numeric_alias_limits_and_conversion():
    """Check alias widths, signed and unsigned endpoints, and native narrowing."""
    int64_min = -(2**63)
    int64_max = 2**63 - 1
    uint64_max = 2**64 - 1
    digit_max = 2**32 - 1
    wide = (1 << 40) + 0x12345

    int64_type = dt.int64
    uint64_type = dt.uint64
    digit_type = dt.sc_digit
    assert int64_type.__cpp_name__ == "sc_dt::int64"
    assert uint64_type.__cpp_name__ == "sc_dt::uint64"
    assert digit_type.__cpp_name__ == "sc_dt::sc_digit"
    assert int(PROBE.echo_int64(int64_type(int64_min))) == int64_min
    assert int(PROBE.echo_int64(int64_type(int64_max))) == int64_max
    assert int(PROBE.echo_uint64(uint64_type(uint64_max))) == uint64_max
    assert int(PROBE.echo_digit(digit_type(digit_max))) == digit_max
    assert int(PROBE.narrow_digit(uint64_type(wide))) == wide % (1 << 32)
    print(
        f"SC_DT_NUMERIC int64=8:{int64_min}:{int64_max} uint64=8:{uint64_max} "
        f"sc_digit=4:0:{digit_max} roundtrip={int64_min}:{int64_max}:{uint64_max}:"
        f"{digit_max} narrow={wide % (1 << 32)}"
    )


def test_math_helpers():
    """Check two scalar abs, three min and three max calls against native results."""
    results = (
        ("sc_abs_int", dt.sc_abs(-19), 19.0),
        ("sc_abs_double", dt.sc_abs(-2.5), 2.5),
        ("sc_min_low_int", dt.sc_min(3, 9), 3.0),
        ("sc_min_negative_int", dt.sc_min(-4, 2), -4.0),
        ("sc_min_double", dt.sc_min(1.25, 0.75), 0.75),
        ("sc_max_high_int", dt.sc_max(3, 9), 9.0),
        ("sc_max_negative_int", dt.sc_max(-4, 2), 2.0),
        ("sc_max_double", dt.sc_max(1.25, 0.75), 1.25),
    )
    for name, value, expected in results:
        assert float(value) == expected
        print(f"SC_DT_MATH {name} {float(value):.1f}")

    magnitude = (1 << 180) + 12345
    positive = dt.sc_bigint[191](1)
    positive <<= 180
    positive += 12345
    big = -positive
    midpoint = positive / 2
    wide_results = (
        ("sc_abs_big191", dt.sc_abs(big), magnitude),
        ("sc_min_big191", dt.sc_min(big, midpoint), -magnitude),
        ("sc_max_big191", dt.sc_max(big, midpoint), magnitude // 2),
    )
    for name, value, expected in wide_results:
        decimal = str(value.to_string(dt.SC_DEC))
        assert decimal == str(expected)
        print(f"SC_DT_MATH {name} {decimal}")


if __name__ == "__main__":
    test_enum_values_and_formats()
    test_logic_constant_values_and_identity()
    test_stream_manipulators()
    test_numeric_alias_limits_and_conversion()
    test_math_helpers()
