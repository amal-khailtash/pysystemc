"""Exercise public sc_dt vector algorithms and emit native-comparable records."""

from pathlib import Path
import gc

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_vector_algorithms_probe.h"))

PROBE = cppyy.gbl.sc_dt_vector_algorithms_probe


def make_pattern(width, alphabet, offset):
    """Build a fixed-width bit pattern shared with the native reference."""
    return "".join(alphabet[(index * 7 + offset) % len(alphabet)] for index in range(width))


def make_wide_integer(template, bits):
    """Build a wide integer in chunks that fit cppyy's scalar conversion path."""
    value = template(0)
    for start in range(0, len(bits), 32):
        chunk = bits[start : start + 32]
        value <<= len(chunk)
        value |= int(chunk, 2)
    return value


def render_bits(value):
    """Render a bit vector or proxy in native bit-string form."""
    return str(value)


def make_operator_value(kind, alternate):
    """Create an owned BV or LV operand for normal/reflected operator checks."""
    if kind == "bv137":
        return dt.sc_bv[137](make_pattern(137, "01", 4 if alternate else 3))
    if kind == "lv191":
        return dt.sc_lv[191](make_pattern(191, "01ZX", 2 if alternate else 1))
    raise AssertionError(f"unknown vector operand kind: {kind}")


def emit_vector_operators():
    """Compare all normal and reflected binary vector operators to C++ results."""
    pairs = (("bv137", "bv137"), ("lv191", "lv191"), ("bv137", "lv191"), ("lv191", "bv137"))
    for operation in ("and", "or", "xor"):
        for left_kind, right_kind in pairs:
            left = make_operator_value(left_kind, False)
            right = make_operator_value(right_kind, left_kind == right_kind)
            left_before = render_bits(left)
            right_before = render_bits(right)
            normal = getattr(left, f"__{operation}__")(right)
            reflected = getattr(left, f"__r{operation}__")(right)
            normal_text = render_bits(normal)
            reflected_text = render_bits(reflected)
            normal_width = int(left.length())
            reflected_width = int(right.length())
            assert int(normal.length()) == normal_width
            assert int(reflected.length()) == reflected_width
            assert render_bits(left) == left_before
            assert render_bits(right) == right_before
            del left, right
            gc.collect()
            assert render_bits(normal) == normal_text
            assert render_bits(reflected) == reflected_text
            print(f"SCA\toperator\t{left_kind}\t{right_kind}\t{operation}\tnormal\t{normal_width}\t{normal_text}")
            print(
                f"SCA\toperator\t{left_kind}\t{right_kind}\t{operation}\treflected\t{reflected_width}\t{reflected_text}"
            )


def emit_scalar_operators():
    """Cover native bool/int/bit-string operators in both operand orders."""
    for vector_kind in ("bv137", "lv191"):
        string_pattern = make_pattern(137, "01", 4) if vector_kind == "bv137" else make_pattern(191, "01ZX", 3)
        scalars = (("bool", True), ("int", 5), ("string", string_pattern))
        for operation in ("and", "or", "xor"):
            for scalar_kind, scalar in scalars:
                vector = make_operator_value(vector_kind, False)
                vector_before = render_bits(vector)
                normal = getattr(vector, f"__{operation}__")(scalar)
                reflected = getattr(vector, f"__r{operation}__")(scalar)
                normal_text = render_bits(normal)
                reflected_text = render_bits(reflected)
                expected_width = int(vector.length())
                assert int(normal.length()) == expected_width
                assert int(reflected.length()) == expected_width
                assert render_bits(vector) == vector_before
                del vector
                gc.collect()
                assert render_bits(normal) == normal_text
                assert render_bits(reflected) == reflected_text
                print(
                    f"SCA\toperator\t{vector_kind}\t{scalar_kind}\t{operation}\tnormal\t{expected_width}\t{normal_text}"
                )
                print(
                    f"SCA\toperator\t{scalar_kind}\t{vector_kind}\t{operation}\treflected\t"
                    f"{expected_width}\t{reflected_text}"
                )


def emit_vector_algorithms(kind, source):
    """Emit free rotation/reversal results and assert they preserve the input."""
    original = render_bits(source)
    results = (
        ("lrotate", dt.lrotate(source, 23)),
        ("rrotate", dt.rrotate(source, int(source.length()) + 19)),
        ("reverse", dt.reverse(source)),
    )
    assert render_bits(source) == original
    for operation, result in results:
        print(f"SCA\trotate\t{kind}\t{operation}\t{render_bits(result)}")


def render_reduction(value):
    """Render native bool reductions as digits and four-state values as logic."""
    if isinstance(value, bool):
        return str(int(value))
    logic_value = dt.sc_logic_value_t(int(value))
    return str(dt.sc_logic(logic_value).to_char())


def emit_reductions(kind, value):
    """Emit all six functional reductions for one native argument type."""
    for operation in ("and_reduce", "nand_reduce", "nor_reduce", "or_reduce", "xnor_reduce", "xor_reduce"):
        result = getattr(dt, operation)(value)
        print(f"SCA\treduce\t{kind}\t{operation}\t{render_reduction(result)}")


def render_fixed(value):
    """Format a fixed-point value at the shared four-decimal precision."""
    return f"{float(value):.4f}"


def emit_shifts(kind, destination, source):
    """Emit both mutating shift results and verify the source remains stable."""
    original = render_fixed(source)
    dt.lshift(destination, source, 2)
    print(f"SCA\tshift\t{kind}\tlshift\t{render_fixed(destination)}")
    dt.rshift(destination, source, 2)
    print(f"SCA\tshift\t{kind}\trshift\t{render_fixed(destination)}")
    assert render_fixed(source) == original


def emit_comparison(operation, left_name, right_name, result):
    """Print one exact sc_bit comparison overload result."""
    print(f"SCA\tcompare\t{operation}\t{left_name}\t{right_name}\t{int(bool(result))}")


def emit_comparisons():
    """Call each equal/not_equal overload with both possible bit values."""
    for left_value in (0, 1):
        left = dt.sc_bit(bool(left_value))
        for right_value in (0, 1):
            right = dt.sc_bit(bool(right_value))
            boolean = bool(right_value)
            character = "1" if right_value else "0"
            integer = right_value
            for operation in ("equal", "not_equal"):
                function = getattr(dt, operation)
                emit_comparison(operation, "bit", "bit", function(left, right))
                emit_comparison(operation, "bit", "bool", function(left, boolean))
                emit_comparison(operation, "bit", "char", function(left, character))
                emit_comparison(operation, "bit", "int", function(left, integer))
                emit_comparison(operation, "bool", "bit", function(boolean, left))
                emit_comparison(operation, "char", "bit", function(character, left))
                emit_comparison(operation, "int", "bit", function(integer, left))


def emit_b_not():
    """Exercise all public b_not signatures, including output-reference forms."""
    bit_source = dt.sc_bit("0")
    bit_result = dt.b_not(bit_source)
    assert str(bit_source) == "0"
    print(f"SCA\tb_not\tbit_return\t{bit_result.to_char()}")
    bit_output = dt.sc_bit("1")
    dt.b_not(bit_output, bit_source)
    assert str(bit_source) == "0"
    print(f"SCA\tb_not\tbit_output\t{bit_output.to_char()}")

    for kind, fixed_type in (
        ("fix", dt.sc_fix),
        ("ufix", dt.sc_ufix),
        ("fix_fast", dt.sc_fix_fast),
        ("ufix_fast", dt.sc_ufix_fast),
    ):
        source = fixed_type(3.25, 8, 4)
        output = fixed_type(0, 8, 4)
        original = render_fixed(source)
        dt.b_not(output, source)
        assert render_fixed(source) == original
        print(f"SCA\tb_not\t{kind}\t{render_fixed(output)}")


def run():
    """Run all assigned vector algorithm families in one isolated process."""
    bv_pattern = make_pattern(137, "01", 3)
    lv_pattern = make_pattern(191, "01ZX", 1)
    lv_second_pattern = make_pattern(191, "01ZX", 2)
    bv = dt.sc_bv[137](bv_pattern)
    lv = dt.sc_lv[191](lv_pattern)
    lv_second = dt.sc_lv[191](lv_second_pattern)
    emit_vector_algorithms("bv137", bv)
    emit_vector_algorithms("lv191", lv)
    lv_subref = lv.range(155, 31)
    emit_vector_algorithms("lv_subref", lv_subref)
    lv_expression = lv ^ lv_second
    print(f"SCA\texpression\tlv_xor\t{render_bits(lv_expression)}")
    emit_vector_algorithms("lv_expression", lv_expression)

    emit_reductions("bv137", bv)
    emit_reductions("lv191", lv)
    uint_value = dt.sc_uint[37](0x5A35)
    int_value = dt.sc_int[41](-0x13579)
    emit_reductions("uint37", uint_value)
    emit_reductions("int41", int_value)
    uint64 = make_wide_integer(dt.sc_uint[64], make_pattern(64, "01", 2))
    int64 = make_wide_integer(dt.sc_int[64], make_pattern(63, "01", 1))
    uint_read_ref = PROBE.uint_subref(uint64, 61, 17)
    int_read_ref = PROBE.int_subref(int64, 60, 12)
    emit_reductions("uint_subref", uint_read_ref)
    emit_reductions("int_subref", int_read_ref)
    concat = PROBE.concat(uint_value, int_value)
    emit_reductions("concat", concat)

    emit_b_not()

    fx_value = dt.sc_fxval(3.25)
    emit_shifts("fxval", dt.sc_fxval(0), fx_value)
    fx_fast_value = dt.sc_fxval_fast(3.25)
    emit_shifts("fxval_fast", dt.sc_fxval_fast(0), fx_fast_value)
    fix_value = dt.sc_fix(3.25, 8, 4)
    emit_shifts("fxval_from_fxnum", dt.sc_fxval(0), fix_value)
    emit_shifts("fxnum", dt.sc_fix(0, 8, 4), fix_value)
    fix_fast_value = dt.sc_fix_fast(3.25, 8, 4)
    emit_shifts("fxval_fast_from_fxnum_fast", dt.sc_fxval_fast(0), fix_fast_value)
    emit_shifts("fxnum_fast", dt.sc_fix_fast(0, 8, 4), fix_fast_value)

    emit_comparisons()
    emit_vector_operators()
    emit_scalar_operators()


run()
