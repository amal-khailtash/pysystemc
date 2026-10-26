"""Exercise public vector concat functions and preserve native result families."""

import csv
import ctypes
from pathlib import Path
import re

from amal.eda.systemc import sc_dt as dt
import cppyy


NATIVE = Path(__file__).resolve().parents[1] / "native"
cppyy.include(str(NATIVE / "sc_dt_concat_functions_probe.h"))
cppyy.include(str(NATIVE / "sc_dt_proxy_probe.h"))
PROBE = cppyy.gbl.sc_dt_concat_functions_probe


def emit(label, *values):
    """Write a stable record for the independent native reference."""
    print("SCF\t" + "\t".join(map(str, (label, *values))))


def render(view):
    """Copy a native value or borrowed proxy spelling while parents are live."""
    return str(view)


def _split_cpp_arguments(arguments):
    """Split C++ parameters without breaking nested template arguments."""
    parts = []
    start = 0
    angle_depth = 0
    paren_depth = 0
    for index, character in enumerate(arguments):
        if character == "<":
            angle_depth += 1
        elif character == ">":
            angle_depth -= 1
        elif character == "(":
            paren_depth += 1
        elif character == ")":
            paren_depth -= 1
        elif character == "," and angle_depth == 0 and paren_depth == 0:
            parts.append(arguments[start:index].strip())
            start = index + 1
    parts.append(arguments[start:].strip())
    return parts


def _split_signature(signature):
    """Separate a normalized C++ result type from its parameters."""
    angle_depth = 0
    for index, character in enumerate(signature):
        if character == "<":
            angle_depth += 1
        elif character == ">":
            angle_depth -= 1
        elif character == "(" and angle_depth == 0:
            return signature[:index].strip(), signature[index + 1 : -1].strip()
    raise AssertionError(f"signature has no parameter list: {signature}")


def _concrete_signature_type(type_name):
    """Substitute one representative BV/LV type into a generic declaration."""
    substitutions = {
        "T": "sc_dt::sc_bv_base",
        "T1": "sc_dt::sc_bv_base",
        "T2": "sc_dt::sc_lv_base",
        "T3": "sc_dt::sc_bv_base",
        "T4": "sc_dt::sc_lv_base",
    }
    for template_name, concrete_name in substitutions.items():
        type_name = re.sub(rf"\b{template_name}\b", concrete_name, type_name)
    return type_name


def _template_type_parts(type_name):
    """Return a native proxy template's name and top-level type arguments."""
    opening = type_name.find("<")
    if opening < 0 or not type_name.endswith(">"):
        return type_name, []
    return type_name[:opening], _split_cpp_arguments(type_name[opening + 1 : -1])


def _matrix_operand(type_name, objects):
    """Choose a live Python operand for one concrete native parameter type."""
    type_name = type_name.strip()
    if type_name == "const char *":
        return ctypes.c_char_p(b"10XZ01")
    type_name = type_name.removeprefix("const ").removesuffix("&").strip()
    if type_name == "bool":
        return True
    if type_name == "sc_dt::sc_logic":
        return objects["logic"]
    if type_name == "sc_dt::sc_value_base":
        return objects["signed_value"]
    template_name, template_args = _template_type_parts(type_name)
    if not template_args:
        raise AssertionError(f"no Python operand for native type {type_name!r}")
    if template_name == "sc_proxy":
        return objects["vectors"][template_args[0]]
    if template_name == "sc_bitref":
        return objects["bits"][template_args[0]]
    if template_name == "sc_bitref_r":
        return objects["readonly_bits"][template_args[0]]
    if template_name == "sc_subref":
        return objects["ranges"][template_args[0]]
    if template_name == "sc_subref_r":
        return objects["readonly_ranges"][template_args[0]]
    pair_key = tuple(template_args)
    if template_name == "sc_concref":
        return objects["pairs"][pair_key]
    if template_name == "sc_concref_r":
        return objects["readonly_pairs"][pair_key]
    raise AssertionError(f"no Python operand for native proxy type {type_name!r}")


def _cppyy_selector_signature(parameters):
    """Format concrete parameters for cppyy's public exact selector."""
    proxy_templates = ("sc_proxy", "sc_bitref", "sc_bitref_r", "sc_subref", "sc_subref_r", "sc_concref", "sc_concref_r")
    formatted = []
    for parameter in parameters:
        for template_name in proxy_templates:
            parameter = parameter.replace(f"{template_name}<", f"sc_dt::{template_name}<")
        parameter = re.sub(r"\s+([&*])", r"\1", parameter)
        formatted.append(parameter)
    return ",".join(formatted)


def _python_cpp_type_name(value):
    """Return cppyy's native result type spelling in inventory form."""
    name = getattr(type(value), "__cpp_name__", type(value).__name__)
    return re.sub(r"\s*([<>,])\s*", r"\1", name)


def run_signature_matrix():
    """Execute each distinct inventory signature with public ``dt.concat``."""
    vector_types = ("sc_dt::sc_bv_base", "sc_dt::sc_lv_base")
    vector_classes = {vector_types[0]: dt.sc_bv_base, vector_types[1]: dt.sc_lv_base}
    owners = {name: vector_class(8) for name, vector_class in vector_classes.items()}
    owners[vector_types[0]].__assign__("10100101")
    owners[vector_types[1]].__assign__("10ZX0101")
    readonly_bits = {
        vector_types[0]: PROBE.readonly_bv_bit(owners[vector_types[0]], 1),
        vector_types[1]: PROBE.readonly_lv_bit(owners[vector_types[1]], 1),
    }
    readonly_ranges = {
        vector_types[0]: PROBE.readonly_bv_range(owners[vector_types[0]], 3, 0),
        vector_types[1]: PROBE.readonly_lv_range(owners[vector_types[1]], 3, 0),
    }
    pairs = {}
    readonly_pairs = {}
    for left_type, left in owners.items():
        for right_type, right in owners.items():
            key = (left_type, right_type)
            pairs[key] = dt.concat(left, right)
            readonly_pairs[key] = PROBE.concat_readonly(left, right)
    objects = {
        "vectors": owners,
        "bits": {name: owner.bit(1) for name, owner in owners.items()},
        "readonly_bits": readonly_bits,
        "ranges": {name: owner.range(3, 0) for name, owner in owners.items()},
        "readonly_ranges": readonly_ranges,
        "pairs": pairs,
        "readonly_pairs": readonly_pairs,
        "logic": dt.sc_logic(dt.SC_LOGIC_Z),
        "signed_value": dt.sc_signed(8),
    }
    objects["signed_value"].__assign__(5)

    inventory_path = Path(__file__).resolve().parents[2] / "docs/_static/public-api-inventory.csv"
    with inventory_path.open(newline="", encoding="utf-8") as source:
        inventory = list(csv.DictReader(source))
    rows_by_name = {
        name: [row for row in inventory if row["name"] == name and row["kind"] == "FunctionDecl"]
        for name in ("sc_dt::concat", "sc_dt::operator,")
    }
    signatures = {name: sorted({row["signature"] for row in rows}) for name, rows in rows_by_name.items()}
    assert len(signatures["sc_dt::concat"]) == len(signatures["sc_dt::operator,"]) == 120
    assert signatures["sc_dt::concat"] == signatures["sc_dt::operator,"]
    stale_lines = {"270", "275", "321", "326", "394", "399", "465", "470"}
    stale_parameter_lists = {
        _split_signature(row["signature"])[1]
        for rows in rows_by_name.values()
        for row in rows
        if row["source"].endswith("sc_bit_proxies.h") and row["line"] in stale_lines
    }

    for index, signature in enumerate(signatures["sc_dt::concat"]):
        result_type, parameter_text = _split_signature(signature)
        parameters = [
            _concrete_signature_type(parameter)
            for parameter in _split_cpp_arguments(parameter_text)
        ]
        assert len(parameters) == 2
        left, right = (_matrix_operand(parameter, objects) for parameter in parameters)
        stale_declaration = parameter_text in stale_parameter_lists
        if stale_declaration:
            # cppyy's normal adapter selects the linked BV-return overload for
            # the eight stale LV declarations frozen by the inventory audit.
            result = dt.concat(left, right)
        else:
            selector = _cppyy_selector_signature(parameters)
            result = dt.concat.__overload__(selector)(left, right)
        spelling = result.to_string(dt.SC_BIN) if "sc_concatref" in result_type else render(result)
        result_type_name = _python_cpp_type_name(result)
        emit(f"concat_{index:03d}", result.length(), spelling, result_type_name)
        emit(f"comma_{index:03d}", result.length(), spelling, result_type_name)

    bit_pair = dt.concat[dt.sc_bv_base, dt.sc_lv_base](readonly_bits[vector_types[0]], readonly_bits[vector_types[1]])
    emit("parent_bit_pair", bit_pair.length(), render(bit_pair), _python_cpp_type_name(bit_pair))
    emit("comma_parent_bit_pair", bit_pair.length(), render(bit_pair), _python_cpp_type_name(bit_pair))

    bit_concat = dt.concat[dt.sc_bv_base, dt.sc_bv_base, dt.sc_lv_base](
        readonly_bits[vector_types[0]], readonly_pairs[(vector_types[0], vector_types[1])]
    )
    emit("parent_bit_concat", bit_concat.length(), render(bit_concat), _python_cpp_type_name(bit_concat))
    emit("comma_parent_bit_concat", bit_concat.length(), render(bit_concat), _python_cpp_type_name(bit_concat))

    for vector_type, label in ((vector_types[0], "bv"), (vector_types[1], "lv")):
        selected_class = vector_classes[vector_type]
        bit_text = dt.concat[selected_class](readonly_bits[vector_type], ctypes.c_char_p(b"10XZ01"))
        record = f"parent_bit_text_{label}"
        emit(record, bit_text.length(), render(bit_text), _python_cpp_type_name(bit_text))
        emit(record.replace("parent_", "comma_parent_", 1), bit_text.length(), render(bit_text), _python_cpp_type_name(bit_text))

    nested_pair = dt.concat[dt.sc_bv_base, dt.sc_lv_base, dt.sc_bv_base, dt.sc_lv_base](
        readonly_pairs[(vector_types[0], vector_types[1])],
        readonly_pairs[(vector_types[0], vector_types[1])],
    )
    emit("parent_nested_pair", nested_pair.length(), render(nested_pair), _python_cpp_type_name(nested_pair))
    emit("comma_parent_nested_pair", nested_pair.length(), render(nested_pair), _python_cpp_type_name(nested_pair))

    concat_text = dt.concat[dt.sc_bv_base, dt.sc_lv_base](
        readonly_pairs[(vector_types[0], vector_types[1])], ctypes.c_char_p(b"10XZ01")
    )
    emit("parent_concat_text", concat_text.length(), render(concat_text), _python_cpp_type_name(concat_text))
    emit("comma_parent_concat_text", concat_text.length(), render(concat_text), _python_cpp_type_name(concat_text))


bv = dt.sc_bv_base(137)
bv.__assign__("0x123456789abcdef123456789abcdef")
lv = dt.sc_lv_base(191)
lv.__assign__("10ZX01ZX001101101")

writable = dt.concat(bv, lv)
emit("mutable", writable.length(), render(writable))
owned_snapshot = dt.sc_lv_base(writable)
snapshot_before = render(owned_snapshot)
writable.__assign__("101001")
emit("mutable_write", bv.to_string(), lv.to_string(), render(writable), snapshot_before, render(owned_snapshot))

comma_writable = dt.concat(bv, lv)
emit("comma_mutable", comma_writable.length(), render(comma_writable))

readonly_bv = PROBE.readonly_bv_range(bv, bv.length() - 1, 0)
readonly_lv = PROBE.readonly_lv_range(lv, lv.length() - 1, 0)
readonly = dt.concat(readonly_bv, readonly_lv)
emit("readonly", readonly.length(), render(readonly))
comma_readonly = dt.concat(readonly_bv, readonly_lv)
emit("comma_readonly", comma_readonly.length(), render(comma_readonly))

bit = bv.bit(3)
range_view = lv.range(19, 7)
mutable_parts = dt.concat(bit, range_view)
emit("mutable_parts", mutable_parts.length(), render(mutable_parts))
mutable_parts.__assign__("101100101")
emit("mutable_parts_write", bv.to_string(), lv.to_string(), render(mutable_parts))

bit_read = PROBE.readonly_bv_range(bv, 3, 3)
range_read = PROBE.readonly_lv_range(lv, 19, 7)
readonly_parts = dt.concat(bit_read, range_read)
emit("readonly_parts", readonly_parts.length(), render(readonly_parts))

nested = dt.concat(writable, bit)
emit("nested", nested.length(), render(nested))
nested_readonly = dt.concat(readonly, bit_read)
emit("nested_readonly", nested_readonly.length(), render(nested_readonly))

text_tail = dt.concat(bv, "10XZ01")
emit("string_leaf", text_tail.length(), render(text_tail))
logic = dt.sc_logic(dt.SC_LOGIC_Z)
logic_tail = dt.concat(lv, logic)
emit("logic_leaf", logic_tail.length(), render(logic_tail))
bool_tail = dt.concat(bv, True)
emit("bool_leaf", bool_tail.length(), render(bool_tail))
bool_head = dt.concat(False, lv)
emit("bool_head", bool_head.length(), render(bool_head))

custom_type = cppyy.gbl.sc_dt_proxy_probe.CustomProxy
custom = custom_type(191)
custom.assign_("10XZ01")
custom_view = dt.concat(custom, bv)
emit("custom", custom_view.length(), render(custom_view))

signed_value = dt.sc_signed(137)
signed_value.__assign__("-123456789")
unsigned_value = dt.sc_unsigned(191)
unsigned_value.__assign__("0x123456789abcdef")
integer_view = dt.concat(signed_value, unsigned_value)
emit("integer_concat", integer_view.length(), integer_view.to_string(dt.SC_BIN))
integer_comma = dt.concat(signed_value, unsigned_value)
emit("integer_comma", integer_comma.length(), integer_comma.to_string(dt.SC_BIN))
integer_bool = dt.concat(unsigned_value, True)
emit("integer_bool", integer_bool.length(), integer_bool.to_string(dt.SC_BIN))


def parent_lifetime_observations():
    """Copy a borrowed result and its value while temporary owners are live."""
    local_bv = dt.sc_bv_base(137)
    local_bv.__assign__("0x123456789abcdef")
    local_lv = dt.sc_lv_base(191)
    local_lv.__assign__("10ZX01ZX")
    local_view = dt.concat(local_bv, local_lv)
    parent_live = render(local_view)
    owned = dt.sc_lv_base(local_view)
    return parent_live, render(owned)


emit("parent_lifetime", *parent_lifetime_observations())
run_signature_matrix()
