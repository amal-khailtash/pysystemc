"""Compare familiar ``sc_time`` operators with native free-function semantics."""

from amal.eda.systemc import sc_core as sc


def emit_case(label, left, right, factor):
    """Exercise all seven free-operator overloads without mutating operands.

    Args:
        label: Case name used to pair Python and native output.
        left: Dividend or first duration, retained unchanged during operations.
        right: Second duration used by binary time operators.
        factor: Floating point scalar used for multiplication and division.

    Returns:
        Output records containing exact result ticks or the time ratio.
    """
    left_ticks, right_ticks = int(left.value()), int(right.value())
    results = (
        ("TIME_ADD", (left + right).value()),
        ("TIME_SUB", (left - right).value()),
        ("TIME_MUL_RIGHT", (left * factor).value()),
        ("TIME_MUL_LEFT", (factor * left).value()),
        ("TIME_DIV_DOUBLE", (left / factor).value()),
        ("TIME_RATIO", left / right),
        ("TIME_MOD", (left % right).value()),
    )
    unchanged = int(left.value() == left_ticks and right.value() == right_ticks)
    records = []
    for operation, result in results:
        if operation == "TIME_RATIO":
            record = f"{operation} {label} {left_ticks} {right_ticks} {float(result):.17g} {unchanged}"
        elif operation == "TIME_MUL_LEFT":
            record = f"{operation} {label} {factor:.17g} {left_ticks} {int(result)} {unchanged}"
        elif operation in ("TIME_MUL_RIGHT", "TIME_DIV_DOUBLE"):
            record = f"{operation} {label} {left_ticks} {factor:.17g} {int(result)} {unchanged}"
        else:
            record = f"{operation} {label} {left_ticks} {right_ticks} {int(result)} {unchanged}"
        records.append(record)
        print(record)
    assert unchanged
    return records


basic_left = sc.sc_time.from_value(1001)
basic_right = sc.sc_time.from_value(250)
emit_case("basic", basic_left, basic_right, 1.5)

fractional_left = sc.sc_time(0.625, sc.SC_NS)
fractional_right = sc.sc_time(0.125, sc.SC_NS)
assert fractional_left.value() == 625 and fractional_right.value() == 125
emit_case("fractional", fractional_left, fractional_right, 1.25)

large_left = sc.sc_time.from_value((1 << 52) + 128)
large_right = sc.sc_time.from_value((1 << 50) + 64)
emit_case("large", large_left, large_right, 1.5)

try:
    basic_left * basic_right
except TypeError, NotImplementedError:
    print("TIME_PRODUCT_INVALID 1")
else:
    raise AssertionError("sc_time * sc_time must not select scalar multiplication")

assert int((basic_left + basic_right).value()) == 1251
assert int((basic_left - basic_right).value()) == 751
assert int((basic_left * 1.5).value()) == 1502
assert int((1.5 * basic_left).value()) == 1502
assert int((basic_left / 1.5).value()) == 667
assert float(basic_left / basic_right) == 4.004
assert int((basic_left % basic_right).value()) == 1
assert int(large_left.value()) == (1 << 52) + 128
assert int(large_right.value()) == (1 << 50) + 64
