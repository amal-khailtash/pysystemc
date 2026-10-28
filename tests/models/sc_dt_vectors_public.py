"""Exercise owning sc_dt vectors and emit records for the native parity test."""

from pathlib import Path
import gc

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_vectors_probe.h"))

probe = cppyy.gbl.sc_dt_vectors_probe


def emit(family, *fields):
    """Print one record shared with the independent native reference."""
    print("SDV\t" + family + "\t" + "\t".join(str(field) for field in fields))


def construct(owner_type, signature, *arguments):
    """Construct through one selected public native constructor overload."""
    owner = owner_type.__new__(owner_type)
    owner_type.__init__.__overload__(signature)(owner, *arguments)
    return owner


def render(value):
    """Return the SystemC bit-string representation of an owner."""
    return str(value)


def emit_constructor(name, label, owner):
    """Emit a constructed owner's exact native rendering."""
    emit("ctor", name, label, render(owner))


def fixed_constructors(name, owner_type, signatures, source):
    """Exercise every fixed-width constructor represented by the inventory."""
    values = {
        "default": ("", ()),
        "logic": ("const sc_dt::sc_logic&", (dt.sc_logic("Z"),)),
        "bool": ("bool", (True,)),
        "char": ("char", ("1" if name.startswith("sc_bv") else "X",)),
        "char_pointer": ("const char*", ("101101" if name.startswith("sc_bv") else "01ZX10",)),
        "bool_pointer": ("const bool*", (probe.bool_bits(),)),
        "logic_pointer": ("const sc_dt::sc_logic*", (probe.logic_bits(),)),
        "sc_unsigned": ("const sc_dt::sc_unsigned&", (probe.unsigned_value(),)),
        "sc_signed": ("const sc_dt::sc_signed&", (probe.signed_value(),)),
        "sc_uint_base": ("const sc_dt::sc_uint_base&", (probe.uint_value(),)),
        "sc_int_base": ("const sc_dt::sc_int_base&", (probe.int_value(),)),
        "unsigned_long": ("unsigned long", (0x12345678,)),
        "long": ("long", (-0x13579,)),
        "unsigned_int": ("unsigned int", (0xA53C,)),
        "int": ("int", (0x5A35,)),
        "uint64": ("sc_dt::uint64", (dt.uint64(0x123456789ABCDEF0),)),
        "int64": ("sc_dt::int64", (dt.int64(-0x123456789),)),
    }
    if name != "sc_lv38":
        values.pop("logic")
    for label, (signature, arguments) in values.items():
        selected = signatures.get(label, signature)
        emit_constructor(name, label, construct(owner_type, selected, *arguments))
    emit_constructor(name, "proxy_bv", owner_type(probe.bv_proxy()))
    emit_constructor(name, "proxy_lv", owner_type(probe.lv_proxy()))
    emit_constructor(name, "copy", owner_type(source))


def base_constructors(name, owner_type, is_logic):
    """Exercise nondeprecated base constructors and generic proxy construction."""
    if is_logic:
        constructors = (
            ("int_length", "int", (54,)),
            ("logic_length", "const sc_dt::sc_logic&, int", (dt.sc_logic("Z"), 54)),
            ("char_pointer", "const char*", ("01ZX10",)),
            ("char_pointer_length", "const char*", ("01ZX10", 54)),
        )
        copy_source = owner_type("01ZX01", 54)
        proxy = probe.lv_proxy()
        proxy_bv = probe.bv_proxy()
    else:
        constructors = (
            ("int_length", "int", (55,)),
            ("bool_length", "bool, int", (True, 55)),
            ("char_pointer", "const char*", ("101101",)),
            ("char_pointer_length", "const char*", ("101101", 55)),
        )
        copy_source = owner_type("10100101", 55)
        proxy = probe.bv_proxy()
        proxy_bv = probe.bv_proxy()
    for label, signature, arguments in constructors:
        if label == "char_pointer_length":
            owner = owner_type(*arguments)
        else:
            owner = construct(owner_type, signature, *arguments)
        emit_constructor(name, label, owner)
    emit_constructor(name, "copy", owner_type(copy_source))
    if is_logic:
        emit_constructor(name, "proxy_bv", owner_type(proxy_bv))
        emit_constructor(name, "proxy_lv", owner_type(proxy))
    else:
        emit_constructor(name, "proxy_bv", owner_type(proxy_bv))
        emit_constructor(name, "proxy_lv", owner_type(probe.lv_proxy()))
    emit_constructor(name, "proxy_template", owner_type(proxy.range(54 if not is_logic else 53, 0)))


def legacy_base_constructors(name, owner_type):
    """Mirror disabled legacy numeric ctors and flag their direct binding gap."""
    sources = (
        ("sc_signed", "const sc_dt::sc_signed&", probe.signed_value()),
        ("sc_unsigned", "const sc_dt::sc_unsigned&", probe.unsigned_value()),
        ("sc_uint_base", "const sc_dt::sc_uint_base&", probe.uint_value()),
        ("sc_int_base", "const sc_dt::sc_int_base&", probe.int_value()),
    )
    for label, signature, source in sources:
        owner = owner_type(source)
        emit_constructor(name, label, owner)
        emit(
            "compat",
            name,
            label,
            "SC_DT_DEPRECATED constructor uses length+assignment compatibility path",
        )


def assign(owner, signature, value, direct=False):
    """Select one assignment overload, or instantiate the generic proxy form."""
    if direct:
        owner.__assign__(value)
    else:
        owner.__assign__.__overload__(signature)(owner, value)
    return owner


def emit_assignment(name, label, owner, signature, value, direct=False):
    """Apply one assignment and emit the owner's resulting contents."""
    assign(owner, signature, value, direct)
    emit("assign", name, label, render(owner))


def fixed_assignments(name, owner_type, source):
    """Exercise each fixed vector assignment overload with typed operands."""
    owner = owner_type("0")
    emit_assignment(name, "proxy_template", owner, "", probe.bv_proxy(), True)
    fixed_signature = "const sc_dt::sc_bv<37>&" if name == "sc_bv37" else "const sc_dt::sc_lv<38>&"
    emit_assignment(name, "copy", owner, fixed_signature, source)
    values = (
        ("char_pointer", "const char*", "10100101101001011100110000111100"),
        ("bool_pointer", "const bool*", probe.bool_bits()),
        ("logic_pointer", "const sc_dt::sc_logic*", probe.logic_bits()),
        ("sc_unsigned", "const sc_dt::sc_unsigned&", probe.unsigned_value()),
        ("sc_signed", "const sc_dt::sc_signed&", probe.signed_value()),
        ("sc_uint_base", "const sc_dt::sc_uint_base&", probe.uint_value()),
        ("sc_int_base", "const sc_dt::sc_int_base&", probe.int_value()),
        ("unsigned_long", "unsigned long", 0x12345678),
        ("long", "long", -0x13579),
        ("unsigned_int", "unsigned int", 0xA53C),
        ("int", "int", 0x5A35),
        ("uint64", "sc_dt::uint64", dt.uint64(0x123456789ABCDEF0)),
        ("int64", "sc_dt::int64", dt.int64(-0x123456789)),
    )
    for label, signature, value in values:
        emit_assignment(name, label, owner, signature, value)


def base_assignments(name, owner_type, is_logic, source):
    """Exercise proxy, owner, string, pointer, integer, and numeric assignments."""
    owner = owner_type(55 if not is_logic else 54)
    bproxy = probe.bv_proxy()
    lproxy = probe.lv_proxy()
    emit_assignment(name, "proxy_bv", owner, "", bproxy, True)
    emit_assignment(name, "proxy_lv", owner, "", lproxy, True)
    emit_assignment(name, "proxy_template", owner, "", bproxy.range(53, 0), True)
    copy_signature = "const sc_dt::sc_bv_base&" if not is_logic else "const sc_dt::sc_lv_base&"
    emit_assignment(name, "copy", owner, copy_signature, source)
    values = (
        ("char_pointer", "const char*", "10100101101001011100110000111100"),
        ("bool_pointer", "const bool*", probe.bool_bits()),
        ("logic_pointer", "const sc_dt::sc_logic*", probe.logic_bits()),
        ("sc_unsigned", "const sc_dt::sc_unsigned&", probe.unsigned_value()),
        ("sc_signed", "const sc_dt::sc_signed&", probe.signed_value()),
        ("sc_uint_base", "const sc_dt::sc_uint_base&", probe.uint_value()),
        ("sc_int_base", "const sc_dt::sc_int_base&", probe.int_value()),
        ("unsigned_long", "unsigned long", 0x12345678),
        ("long", "long", -0x13579),
        ("unsigned_int", "unsigned int", 0xA53C),
        ("int", "int", 0x5A35),
        ("uint64", "sc_dt::uint64", dt.uint64(0x123456789ABCDEF0)),
        ("int64", "sc_dt::int64", dt.int64(-0x123456789)),
    )
    for label, signature, value in values:
        emit_assignment(name, label, owner, signature, value)


def emit_storage(name, value):
    """Check base accessors and mutations against their independent native values."""
    emit(
        "storage",
        name,
        value.length(),
        value.size(),
        int(value.is_01()),
        int(value.get_bit(0)),
        value.get_word(0),
        value.get_cword(0),
    )
    value.set_bit(0, 1)
    value.set_word(0, 0xA5)
    value.set_cword(0, 0x5A)
    value.clean_tail()
    emit("storage_after", name, render(value), value.get_word(0), value.get_cword(0))


fixed_constructors(
    "sc_bv37",
    dt.sc_bv[37],
    {"default": "", "bool": "bool", "char": "char"},
    dt.sc_bv[37]("101001011010010111001"),
)
fixed_constructors(
    "sc_lv38",
    dt.sc_lv[38],
    {"default": "", "bool": "bool", "char": "char", "logic": "const sc_dt::sc_logic&"},
    dt.sc_lv[38]("01ZX001101"),
)
base_constructors("sc_bv_base55", dt.sc_bv_base, False)
base_constructors("sc_lv_base54", dt.sc_lv_base, True)
legacy_base_constructors("sc_bv_base55", dt.sc_bv_base)
legacy_base_constructors("sc_lv_base54", dt.sc_lv_base)

fixed_assignments("sc_bv37", dt.sc_bv[37], dt.sc_bv[37]("10100101101001011100110000111100"))
fixed_assignments("sc_lv38", dt.sc_lv[38], dt.sc_lv[38]("01ZX101001011100110000111100"))
base_assignments("sc_bv_base55", dt.sc_bv_base, False, dt.sc_bv_base("10100101101001011100110000111100", 55))
base_assignments("sc_lv_base54", dt.sc_lv_base, True, dt.sc_lv_base("01ZX101001011100110000111100", 54))
for owner_name in ("sc_bv37", "sc_lv38", "sc_bv_base55", "sc_lv_base54"):
    for operation in ("constructor_template", "assignment_template"):
        emit(
            "compat",
            owner_name,
            operation,
            "cppyy omits direct template selection; exact typed vector adapters exercise these rows",
        )

bv_storage = dt.sc_bv_base("01010101010101010101010101010101010101010101010101010101")
lv_storage = dt.sc_lv_base("01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01")
emit_storage("sc_bv_base", bv_storage)
emit_storage("sc_lv_base", lv_storage)
emit("protected", "sc_bv_base", probe.bv_storage_state(bv_storage))
emit("protected", "sc_lv_base", probe.lv_storage_state(lv_storage))

for label, value in (
    ("bv137", dt.sc_bv[137]("01")),
    ("lv191", dt.sc_lv[191]("01ZX")),
    ("bv257", dt.sc_bv[257]("10")),
    ("lv257", dt.sc_lv[257]("ZX01")),
):
    emit("wide", label, value.length(), render(value))

wide_lv_proxy = probe.lv_proxy().range(190, 0)
wide_lv_constructed = dt.sc_lv[191](wide_lv_proxy)
wide_lv_assigned = dt.sc_lv[191](191)
wide_lv_assigned.__assign__(wide_lv_proxy)
emit("wide_proxy", "lv191", render(wide_lv_constructed), render(wide_lv_assigned))

bv_copy_source = dt.sc_bv_base("1010010110100101", 55)
bv_copy = dt.sc_bv_base(55)
bv_copy.__assign__(bv_copy_source)
copied_bv = render(bv_copy)
bv_copy_source.set_bit(0, 0)
assert render(bv_copy) == copied_bv
lv_copy_source = dt.sc_lv_base("01ZX1010XZ", 54)
lv_copy = dt.sc_lv_base(54)
lv_copy.__assign__(lv_copy_source)
copied_lv = render(lv_copy)
lv_copy_source.set_bit(0, 0)
assert render(lv_copy) == copied_lv
del bv_copy_source, lv_copy_source
gc.collect()
emit("lifetime", "bv_copy", bv_copy.length(), render(bv_copy))
emit("lifetime", "lv_copy", lv_copy.length(), render(lv_copy))
emit("traits", "owners_destructible", 1)

try:
    dt.sc_bv_base(-1)
except Exception:
    emit("error", "negative_base_length", 1)
else:
    emit("error", "negative_base_length", 0)
