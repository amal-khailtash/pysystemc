"""Exercise live native concatenation shells, exact overloads and copied values."""

import array
import gc
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_concatref_probe.h"))
probe = cppyy.gbl.concatref_probe


def emit(label, *values):
    """Print a stable record for the independent native reference comparison."""
    print("CCR\t" + "\t".join(map(str, (label, *values))))


def render(node):
    """Copy full-width decimal text while the shell's borrowed leaves are live."""
    return str(node.to_string(dt.SC_DEC))


def assign(node, label, signature, source):
    """Invoke one exact native assignment and verify its borrowed self result."""
    node.__assign__.__overload__("const char*")("0")
    result = node.__assign__.__overload__(signature)(source)
    emit("assign", label, render(node), int(cppyy.addressof(result) == cppyy.addressof(node)))
    assert not result.__python_owns__, "assignment result is a borrowed pool shell"


def set_value(node, label, signature, source, low):
    """Scatter an exact typed source at a native bit offset into live leaves."""
    node.__assign__("0")
    node.concat_set.__overload__(signature)(source, low)
    emit("set", label, low, render(node))


def packed(node, label, low=7):
    """Pack data/control digits at a valid offset and preserve outside sentinels."""
    for control in (False, True):
        words = array.array("I", [0x15555555] * probe.packing_digits(node, low))
        method = node.concat_get_ctrl if control else node.concat_get_data
        result = method(words, low)
        emit("pack", label, int(control), int(result), *words)


def conversions(node, label):
    """Check scalar, owned, borrowed and string conversions of a live shell."""
    emit(
        "convert",
        label,
        node.to_uint64(),
        node.to_int64(),
        node.to_uint(),
        node.to_int(),
        node.to_ulong(),
        node.to_long(),
        probe.implicit_integer(node),
        node.concat_get_uint64(),
    )
    assert int(node) == probe.implicit_integer(node)
    stream = cppyy.gbl.std.ostringstream()
    node.print(stream)
    emit("string", label, node.to_string(), node.to_string(dt.SC_HEX), node.to_string(dt.SC_BIN, False), stream.str())
    value = node.value()
    assert not value.__python_owns__, "value returns a borrowed unsigned shell"
    copy = dt.sc_unsigned(value)
    borrowed = probe.implicit_reference(node)
    assert not borrowed.__python_owns__
    emit(
        "value", label, copy.length(), copy.to_string(), borrowed.to_string(), int(node.to_double() == copy.to_double())
    )
    sign = dt.sc_signed(349)
    unsign = dt.sc_unsigned(349)
    node.to_sc_signed(sign)
    node.to_sc_unsigned(unsign)
    emit("output", label, sign.to_string(), unsign.to_string())
    positive, negative, complement = +node, -node, ~node
    assert positive.__python_owns__ and negative.__python_owns__ and complement.__python_owns__
    emit(
        "unary",
        label,
        positive.length(),
        positive.to_string(),
        negative.length(),
        negative.to_string(),
        complement.length(),
        complement.to_string(),
    )


def reductions(node, label):
    """Emit every reduction member for zero, ones and both parity states."""
    emit(
        "reduce",
        label,
        *(
            int(getattr(node, name)())
            for name in (
                "and_reduce",
                "nand_reduce",
                "or_reduce",
                "nor_reduce",
                "xor_reduce",
                "xnor_reduce",
            )
        ),
    )


left = dt.sc_signed(137)
left.__assign__("0x123456789abcdef123456789abcdef")
right = dt.sc_unsigned(191)
right.__assign__("0xabcdef00112233445566778899aabbccddeeff")
node = probe.select(left, right)
assert not node.__python_owns__, "global concat allocator returns a borrowed shell"
emit(
    "capacity",
    probe.capacity(),
    int(probe.traits()),
    node.length(),
    node.bitwidth(),
    node.concat_length(cppyy.nullptr),
    int(probe.has_xz(node)),
    int(dt.sc_concatref.cf_none),
    int(dt.sc_concatref.cf_xz_present),
)
const_node = probe.const_select(left, right)
emit("const", const_node.length(), render(const_node))
# Select both exact initialize overloads through the public Python method.
node.initialize.__overload__("sc_dt::sc_value_base&, sc_dt::sc_value_base&")(left, right)
const_node.initialize.__overload__("const sc_dt::sc_value_base&, const sc_dt::sc_value_base&")(left, right)
conversions(node, "wide")
packed(node, "wide")
small_left = dt.sc_uint[11](0x555)
small_right = dt.sc_uint[7](0x53)
small = probe.select(small_left, small_right)
conversions(small, "small")
signed_source = dt.sc_signed(349)
signed_source.__assign__("-81985529216486895")
unsigned_source = dt.sc_unsigned(349)
unsigned_source.__assign__("0x123456789abcdef001122334455667788")
source_left = dt.sc_unsigned(173)
source_left.__assign__("0x13579abcdef")
source_right = dt.sc_unsigned(155)
source_right.__assign__("0xfedcba98765")
source_concat = probe.select(source_left, source_right)
bv = dt.sc_bv_base("101001001101011001011001110101011")
lv = dt.sc_lv_base("101101001101011001011001110101011")
for label, signature, source in (
    ("int", "int", -13579),
    ("long", "long", -0x123456789),
    ("int64", "sc_dt::int64", -0x123456789ABCDE),
    ("unsigned_int", "unsigned int", 0xA5A55A5A),
    ("unsigned_long", "unsigned long", 0x123456789ABCDEF),
    ("uint64", "sc_dt::uint64", 0xFEDCBA9876543210),
    ("concatref", "const sc_dt::sc_concatref&", source_concat),
    ("signed", "const sc_dt::sc_signed&", signed_source),
    ("unsigned", "const sc_dt::sc_unsigned&", unsigned_source),
    ("text", "const char*", "0x123456789abcdef00123456789"),
    ("bv", "const sc_dt::sc_bv_base&", bv),
    ("lv", "const sc_dt::sc_lv_base&", lv),
):
    assign(node, label, signature, source)
for low in (0, 9):
    for label, signature, source in (
        ("int64", "sc_dt::int64, int", -0x123456789ABCDE),
        ("uint64", "sc_dt::uint64, int", 0xFEDCBA9876543210),
        ("signed", "const sc_dt::sc_signed&, int", signed_source),
        ("unsigned", "const sc_dt::sc_unsigned&, int", unsigned_source),
    ):
        set_value(node, label, signature, source, low)
for label, text in (("zero", "0"), ("ones", "0x3ffff"), ("odd", "7"), ("even", "3")):
    small.__assign__(text)
    reductions(small, label)
stream = cppyy.gbl.std.istringstream("0x13579 extra")
small.scan(stream)
emit("scan", render(small))
previous = render(small)
handler = cppyy.gbl.sc_core.sc_report_handler
old_actions = handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, cppyy.gbl.sc_core.SC_THROW)
rejected = False
try:
    small.__assign__.__overload__("const char*")("invalid-number")
except cppyy.gbl.sc_core.sc_report:
    rejected = True
finally:
    handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, old_actions)
emit("error", int(rejected), int(render(small) == previous))
assert rejected
ol, oright = dt.sc_uint[8](0xA5), dt.sc_uint[8](0x3C)
overlap, reverse = probe.select(ol, oright), probe.select(oright, ol)
overlap.__assign__.__overload__("const sc_dt::sc_concatref&")(reverse)
emit("overlap", ol.to_uint(), oright.to_uint(), render(overlap))
logic_left, logic_right = probe.LogicLeaf("10XZ01"), probe.LogicLeaf("Z110")
logic = probe.select(logic_left, logic_right)
emit("logic", logic.length(), int(probe.has_xz(logic)), logic.concat_get_uint64(), render(logic))
packed(logic, "xz")
packed(logic, "xz_cross", 27)
logic.concat_clear_data(False)
emit("clear", 0, logic_left.bits, logic_right.bits, int(probe.has_xz(logic)))
packed(logic, "clear0")
logic.concat_clear_data(True)
emit("clear", 1, logic_left.bits, logic_right.bits, int(probe.has_xz(logic)))
packed(logic, "clear1")
logic.initialize(logic_left, logic_right)
emit("reinit", int(probe.has_xz(logic)), logic.length())
retained = probe.select(small_left, small_right)
stable = probe.snapshot(retained)
positive, negative, complement = +retained, -retained, ~retained
reuse_left, reuse_right = dt.sc_uint[11](7), dt.sc_uint[7](9)
for _ in range(probe.capacity()):
    last = probe.select(reuse_left, reuse_right)
emit(
    "shell_wrap",
    int(cppyy.addressof(last) == cppyy.addressof(retained)),
    render(retained),
    stable.to_string(),
    positive.to_string(),
    negative.to_string(),
    complement.to_string(),
)
value_left, value_right = dt.sc_uint[8](45), dt.sc_uint[8](67)
value_node = probe.select(value_left, value_right)
borrowed = retained.value()
owned = dt.sc_unsigned(borrowed)
for _ in range(probe.value_capacity()):
    last_value = value_node.value()
emit(
    "value_wrap",
    probe.value_capacity(),
    int(cppyy.addressof(last_value) == cppyy.addressof(borrowed)),
    borrowed.to_string(),
    owned.to_string(),
)
before = probe.LogicLeaf.destroyed
pool_copy = dt.sc_unsigned(10)
pool = probe.OwnedPool()
pooled = pool.allocate()
assert not pooled.__python_owns__
pooled.initialize(logic_left, logic_right)
pool_copy.__assign__(pooled.value())
# The test-owned array invokes public concat destructors; leaves stay live.
pool.__destruct__()
del pool, pooled
gc.collect()
emit("destruction", int(probe.LogicLeaf.destroyed == before), pool_copy.to_string())
surviving, surviving_unary = dt.sc_unsigned(16), dt.sc_unsigned(16)
a, b = dt.sc_uint[8](0xA5), dt.sc_uint[8](0x3C)
live = probe.select(a, b)
surviving.__assign__(live.value())
surviving_unary.__assign__(+live)
surviving_positive, surviving_negative, surviving_complement = +live, -live, ~live
del a, b, live
gc.collect()
emit(
    "owners_gone",
    surviving.to_string(),
    surviving_unary.to_string(),
    surviving_positive.length(),
    surviving_positive.to_string(),
    surviving_negative.length(),
    surviving_negative.to_string(),
    surviving_complement.length(),
    surviving_complement.to_string(),
)
print("SC_DT_CONCATREF_PUBLIC_OK")
