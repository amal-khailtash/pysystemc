"""Exercise the public arbitrary-width four-state logic resolver."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


def resolve(width, driver_strings):
    """Resolve borrowed ``sc_lv`` driver pointers in a caller-owned result.

    Args:
        width: Positive vector width used to instantiate the native template.
        driver_strings: Nonempty sequence of equally sized logic strings.

    Returns:
        A tuple containing the resolved value, result-address check, and input
        preservation check.

    The native API asserts on an empty vector. Every pointer below remains live
    for the full call, and each driver has the result's width.
    """
    value_type = dt.sc_lv[width]
    drivers = [value_type(value) for value in driver_strings]
    pointer_vector = cppyy.gbl.std.vector[f"sc_dt::sc_lv<{width}>*"]()
    for value in drivers:
        pointer_vector.push_back(value)

    result = value_type("X" * width)
    original_result_address = cppyy.addressof(result)
    original_driver_strings = [value.to_string() for value in drivers]
    expected_addresses = [cppyy.addressof(value) for value in drivers]
    resolver = sc.sc_lv_resolve[width]
    resolver.resolve(result, pointer_vector)

    addresses_preserved = cppyy.addressof(result) == original_result_address
    pointers_preserved = [cppyy.addressof(pointer_vector[index]) for index in range(pointer_vector.size())]
    inputs_unchanged = [value.to_string() for value in drivers] == original_driver_strings
    assert pointers_preserved == expected_addresses
    assert addresses_preserved and inputs_unchanged
    return result.to_string(), addresses_preserved, inputs_unchanged


symbols = "01XZ"
for left in symbols:
    for right in symbols:
        resolved, _, _ = resolve(1, (left, right))
        print("RESOLVE_PAIR", left, right, resolved)

singleton = "01XZ" * 34 + "0"
single_value, single_alias, single_unchanged = resolve(137, (singleton,))
assert single_value == singleton and single_value != "X" * 137
print("RESOLVE_SINGLETON", single_value, int(single_alias), int(single_unchanged))

multi_values = ("01XZ", "10Z1", "ZZ0Z")
multi_value, multi_alias, multi_unchanged = resolve(4, multi_values)
assert multi_value == "XXX1"
print("RESOLVE_MULTI", multi_value, int(multi_alias), int(multi_unchanged))

wide_values = tuple(
    (pattern * ((137 + len(pattern) - 1) // len(pattern)))[:137] for pattern in ("01XZ", "X0Z1", "ZZ101")
)
wide_value, wide_alias, wide_unchanged = resolve(137, wide_values)
assert len(wide_value) == 137 and wide_value != "X" * 137
print("RESOLVE_WIDE", wide_value, int(wide_alias), int(wide_unchanged))
