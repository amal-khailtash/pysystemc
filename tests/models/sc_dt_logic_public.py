"""Exercise the public ``sc_bit`` and four-state ``sc_logic`` APIs."""

from amal.eda.systemc import sc_dt as dt
import cppyy
import gc
from pathlib import Path
import sys

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_logic_probe.h"))
probe = cppyy.gbl.sc_dt_logic_probe

if len(sys.argv) > 1 and sys.argv[1].startswith("bit-error"):
    if sys.argv[1] == "bit-error-int":
        dt.sc_bit(2)
    else:
        dt.sc_bit("Q")
    raise AssertionError("invalid sc_bit input unexpectedly returned")


def emit(*fields):
    """Print a normalized record consumed by the independent C++ comparator.

    Args:
        fields: Scalar values describing one constructor, operation, or table.
    """
    print("DT_LOGIC\t" + "\t".join(str(field) for field in fields))


def logic_char(value):
    """Return the printable native four-state character.

    Args:
        value: Borrowed ``sc_logic`` value.

    Returns:
        The corresponding character in ``0``, ``1``, ``Z``, or ``X``.
    """
    return str(value.to_char())


def bit_char(value):
    """Return the printable native bit character.

    Args:
        value: Borrowed ``sc_bit`` value.

    Returns:
        ``0`` or ``1``.
    """
    return str(value.to_char())


ZERO_ONE = ("0", "1")
LOGIC_CHARS = ("0", "1", "Z", "X")
SCALAR_TYPES = (
    ("bool", bool),
    ("char", str),
    ("int", int),
    ("unsigned", probe.Unsigned),
    ("long", probe.Long),
    ("unsigned_long", probe.UnsignedLong),
    ("int64", probe.Int64),
    ("uint64", probe.UInt64),
)

# Constructor overloads: default, copy, all scalar widths, and logic conversion.
bit_default = dt.sc_bit()
emit("bit_ctor", "default", bit_char(bit_default))
emit("bit_ctor", "copy", bit_char(dt.sc_bit(bit_default)))
for text in ZERO_ONE:
    for type_name, scalar_type in SCALAR_TYPES:
        value = scalar_type(text if type_name == "char" else int(text) if type_name != "bool" else text == "1")
        emit("bit_ctor", type_name, text, bit_char(dt.sc_bit(value)))
for char in LOGIC_CHARS:
    value = dt.sc_logic(char)
    emit("bit_ctor", "logic", char, bit_char(dt.sc_bit(value)))

# Assignment overloads are invoked on the Python-bound public object.
BIT_ASSIGN_TYPES = (
    ("char", str),
    ("bool", bool),
    ("int", int),
    ("long", probe.Long),
    ("unsigned_long", probe.UnsignedLong),
    ("int64", probe.Int64),
    ("uint64", probe.UInt64),
)
for text in ZERO_ONE:
    for type_name, scalar_type in BIT_ASSIGN_TYPES:
        argument = scalar_type(text if type_name == "char" else int(text) if type_name != "bool" else text == "1")
        value = dt.sc_bit(False)
        value.__assign__(argument)
        emit("bit_assign", type_name, text, bit_char(value))
for char in LOGIC_CHARS:
    value = dt.sc_bit(False)
    value.__assign__(dt.sc_logic(char))
    emit("bit_assign", "logic", char, bit_char(value))
bit_assigned = dt.sc_bit(False)
bit_assigned.__assign__(dt.sc_bit(True))
emit("bit_assign", "bit", "1", bit_char(bit_assigned))

# Exercise every compound method overload with informative 0/1 operands.
BIT_COMPOUND_TYPES = (("bit", lambda bit: bit), *BIT_ASSIGN_TYPES[:3])
for operator_name in ("and", "or", "xor"):
    method_name = {"and": "__iand__", "or": "__ior__", "xor": "__ixor__"}[operator_name]
    for left_char in ZERO_ONE:
        for type_name, make_value in BIT_COMPOUND_TYPES:
            for right_char in ZERO_ONE:
                left = dt.sc_bit(left_char)
                if type_name == "bit":
                    right = make_value(dt.sc_bit(right_char))
                else:
                    scalar_type = make_value
                    right = scalar_type(
                        right_char
                        if type_name == "char"
                        else int(right_char)
                        if type_name != "bool"
                        else right_char == "1"
                    )
                getattr(left, method_name)(right)
                emit("bit_compound", operator_name, left_char, type_name, right_char, bit_char(left))

# Conversions, logical negation, complement, and the mutating deprecated method.
for bit_char_value in ZERO_ONE:
    value = dt.sc_bit(bit_char_value)
    emit(
        "bit_convert",
        bit_char_value,
        int(bool(value)),
        int(not value),
        int(value.to_bool()),
        str(value.to_char()),
        str(probe.print_bit(value)),
    )
    value.b_not()
    emit("bit_b_not", bit_char_value, bit_char(value))
for scanned in ZERO_ONE:
    emit("bit_scan", scanned, bit_char(probe.scan_bit(scanned)))

# Use catchable native reports for invalid assignment and compound operands.
previous_error_actions = probe.throw_errors()
for operation_name, apply_operation in (
    ("assign", lambda value, argument: value.__assign__(argument)),
    ("and", lambda value, argument: value.__iand__(argument)),
    ("or", lambda value, argument: value.__ior__(argument)),
    ("xor", lambda value, argument: value.__ixor__(argument)),
):
    for type_name, argument in (("int", 2), ("char", "Q")):
        value = dt.sc_bit(False)
        try:
            apply_operation(value, argument)
        except Exception:
            emit("bit_invalid_operation", operation_name, type_name, "caught", bit_char(value))
        else:
            raise AssertionError(f"invalid sc_bit {operation_name} {type_name} unexpectedly returned")
probe.restore_errors(previous_error_actions)

# Constructors, copy, assignments, and scalar conversion for all logic states.
emit("logic_ctor", "default", logic_char(dt.sc_logic()))
for state in LOGIC_CHARS:
    original = dt.sc_logic(state)
    emit("logic_ctor", "copy", state, logic_char(dt.sc_logic(original)))
for state_index, state in enumerate(LOGIC_CHARS):
    emit("logic_ctor", "enum", state, logic_char(dt.sc_logic(dt.sc_logic_value_t(state_index))))
    emit("logic_ctor", "int", state, logic_char(dt.sc_logic(state_index)))
for state in ZERO_ONE:
    emit("logic_ctor", "bool", state, logic_char(dt.sc_logic(state == "1")))
for state in LOGIC_CHARS:
    emit("logic_ctor", "char", state, logic_char(dt.sc_logic(state)))
for bit_value in ZERO_ONE:
    emit("logic_ctor", "bit", bit_value, logic_char(dt.sc_logic(dt.sc_bit(bit_value))))

LOGIC_ASSIGN_TYPES = (
    ("enum", lambda state: dt.sc_logic_value_t(LOGIC_CHARS.index(state))),
    ("bool", lambda state: state == "1"),
    ("char", lambda state: state),
    ("int", lambda state: LOGIC_CHARS.index(state)),
    ("bit", lambda state: dt.sc_bit("1" if state in ("1", "X", "Z") else "0")),
)
for state in LOGIC_CHARS:
    for type_name, make_argument in LOGIC_ASSIGN_TYPES:
        value = dt.sc_logic("0")
        value.__assign__(make_argument(state))
        emit("logic_assign", state, type_name, logic_char(value))
    value = dt.sc_logic("0")
    value.__assign__(dt.sc_logic(state))
    emit("logic_assign", state, "logic", logic_char(value))

# Every logic compound method is checked across all 16 state pairs and all
# operand overloads, including implicit scalar conversion paths.
LOGIC_COMPOUND_TYPES = (
    ("logic", lambda state: dt.sc_logic(state)),
    ("enum", lambda state: dt.sc_logic_value_t(LOGIC_CHARS.index(state))),
    ("bool", lambda state: state != "0"),
    ("char", lambda state: state),
    ("int", lambda state: LOGIC_CHARS.index(state)),
    ("bit", lambda state: dt.sc_bit("1" if state != "0" else "0")),
)
for operator_name, method_name in (("and", "__iand__"), ("or", "__ior__"), ("xor", "__ixor__")):
    for left_char_value in LOGIC_CHARS:
        for type_name, make_argument in LOGIC_COMPOUND_TYPES:
            for right_char_value in LOGIC_CHARS:
                left = dt.sc_logic(left_char_value)
                getattr(left, method_name)(make_argument(right_char_value))
                emit(
                    "logic_compound",
                    operator_name,
                    left_char_value,
                    type_name,
                    right_char_value,
                    logic_char(left),
                )

# Validate instance conversions and both the method and static truth tables.
for state in LOGIC_CHARS:
    value = dt.sc_logic(state)
    emit(
        "logic_convert",
        state,
        int(value.value()),
        int(value.is_01()),
        int(value.to_bool()),
        logic_char(value),
        str(probe.print_logic(value)),
        logic_char(~value),
    )
    value.b_not()
    emit("logic_b_not", state, logic_char(value))
for scanned in LOGIC_CHARS:
    emit("logic_scan", scanned, logic_char(probe.scan_logic(scanned)))

for table_name in ("and_table", "or_table", "xor_table"):
    table = getattr(dt.sc_logic, table_name)
    emit("logic_table", table_name, ",".join(str(int(table[row][column])) for row in range(4) for column in range(4)))
not_table = dt.sc_logic.not_table
emit("logic_table", "not_table", ",".join(str(int(not_table[index])) for index in range(4)))
emit("logic_table", "logic_to_char", "".join(str(dt.sc_logic.logic_to_char[index]) for index in range(4)))
emit("logic_table", "char_to_logic", ",".join(str(int(dt.sc_logic.char_to_logic[index])) for index in range(128)))

# Invalid four-state values report an error and retain the native Log_X fallback.
previous_error_actions = probe.suppress_errors()
emit("logic_invalid", "int", logic_char(dt.sc_logic(4)))
emit("logic_invalid", "char", logic_char(dt.sc_logic("Q")))
probe.restore_errors(previous_error_actions)

# Derived native counters make destruction of Python-owned wrappers observable.
counters = probe.DestructionCounters()
temporary_bits = [probe.CountedBit(counters, bool(index & 1)) for index in range(512)]
temporary_logic = [probe.CountedLogic(counters, LOGIC_CHARS[index & 3]) for index in range(512)]
assert counters.bit_count == counters.logic_count == 0
del temporary_bits, temporary_logic
gc.collect()
assert counters.bit_count == counters.logic_count == 512
emit("lifetime", counters.bit_count, counters.logic_count)
