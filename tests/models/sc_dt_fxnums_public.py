"""Exercise protected fixed-point number bases through owned typed storage."""

import gc
import csv
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

ROOT = Path(__file__).resolve().parents[2]
cppyy.include(str(ROOT / "tests/native/sc_dt_fxnums_probe.h"))
probe = cppyy.gbl.sc_dt_fxnums_probe


def emit(kind, family, label, value):
    """Write one deterministic record for comparison with native SystemC."""
    print(f"FXN\t{kind}\t{family}\t{label}\t{value}")


def render(value):
    """Render fixed-point value, primitive conversions, parameters and flags."""
    return "|".join(
        map(
            str,
            (
                format(float(value.to_double()), ".17g"),
                str(value.to_string(dt.SC_DEC)),
                int(value.to_int()),
                int(value.to_uint()),
                value.wl(),
                value.iwl(),
                int(value.q_mode()),
                int(value.o_mode()),
                value.n_bits(),
                int(value.is_neg()),
                int(value.is_zero()),
                int(value.quantization_flag()),
                int(value.overflow_flag()),
                int(value.cast_switch() == dt.SC_ON),
            ),
        )
    )


def render_precision(value):
    """Render arbitrary-width values without narrowing through native integers."""
    return "|".join(
        (
            format(float(value.to_double()), ".17g"),
            str(value.to_string(dt.SC_DEC)),
            str(value.wl()),
            str(value.iwl()),
        )
    )


def source_values(sources):
    """Return every source with an exact SystemC parameter signature."""
    return {
        "int": probe.Int(-3),
        "unsigned int": probe.UnsignedInt(5),
        "long": probe.Long(-30),
        "unsigned long": probe.UnsignedLong(31),
        "float": probe.Float(1.25),
        "double": probe.Double(2.625),
        "const char *": probe.Sources.text(),
        "const sc_dt::sc_fxval &": sources.value,
        "const sc_dt::sc_fxval_fast &": sources.value_fast,
        "const sc_dt::sc_fxnum &": sources.number,
        "const sc_dt::sc_fxnum_fast &": sources.number_fast,
        "sc_dt::int64": dt.int64(-12345),
        "sc_dt::uint64": dt.uint64(54321),
        "const sc_dt::sc_int_base &": sources.sint,
        "const sc_dt::sc_uint_base &": sources.uint,
        "const sc_dt::sc_signed &": sources.signed_value,
        "const sc_dt::sc_unsigned &": sources.unsigned_value,
    }


def select_public_signatures(number_type, class_name):
    """Resolve every public member signature against its Python-facing selector.

    Args:
        number_type: Public owned facade for one native number base.
        class_name: Inventory name of its `sc_dt::sc_fxnum` base.
    Returns:
        Number of exact public member overloads selected.
    Raises:
        AttributeError: A public native member has no Python-facing binding.
        TypeError: The binding cannot select one declaration by exact signature.
    """
    python_name = {
        "operator=": "__assign__",
        "operator+=": "__iadd__",
        "operator-=": "__isub__",
        "operator*=": "__imul__",
        "operator/=": "__itruediv__",
        "operator<<=": "__ilshift__",
        "operator>>=": "__irshift__",
        "operator()": "__call__",
        "operator[]": "__getitem__",
        "operator+": "__pos__",
        "operator-": "__neg__",
    }
    selected = 0
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        rows = list(csv.DictReader(source))
    for row in rows:
        if not row["name"].startswith(class_name + "::") or row["kind"] != "CXXMethodDecl":
            continue
        native_name = row["name"].rsplit("::", 1)[-1]
        if native_name == "observer":
            # Protected access is tested through the native pointer-to-member probe.
            continue
        parameters = row["signature"][row["signature"].index("(") + 1 : row["signature"].rindex(")")]
        parameters = parameters.replace("::std::", "std::").replace(" &", "&")
        exposed_name = python_name.get(native_name, native_name)
        if native_name == "operator++":
            exposed_name = "__postinc__" if parameters.strip() == "int" else "__preinc__"
        elif native_name == "operator--":
            exposed_name = "__postdec__" if parameters.strip() == "int" else "__predec__"
        selector = getattr(number_type, exposed_name)
        selector.__overload__(parameters)
        selected += 1
    assert selected == 140, (class_name, selected)
    return selected


def construct(number_type, family, source_signature, source, params, cast, observer):
    """Initialize exact native storage through its source-typed in-place overload.

    Args:
        number_type: Native arbitrary-precision or fast number base.
        family: Stable observer spelling used by the native constructor.
        source_signature: Exact source parameter, or an empty string for zero.
        source: Borrowed source value passed to the exact constructor.
        params: Owned fixed-point format parameters copied into the number.
        cast: Owned cast switch copied into the number.
        observer: Borrowed observer retained by the number until destruction.
    Returns:
        A Python-owned number initialized in place.
    Raises:
        TypeError: The exact native constructor overload is unavailable.
        Native exception: Native allocation or fixed-point reports fail.
    """
    observer_signature = f"sc_dt::{family}_observer *"
    tail = f"const sc_dt::sc_fxtype_params &, sc_dt::sc_enc, const sc_dt::sc_fxcast_switch &, {observer_signature}"
    signature = f"{source_signature}, {tail}" if source_signature else tail
    number = number_type.__new__(number_type)
    args = [source, params, dt.SC_TC_, cast, observer] if source_signature else [params, dt.SC_TC_, cast, observer]
    number_type.__init__.__overload__(signature)(number, *args)
    assert number.__python_owns__
    return number


def exercise(family, number_type, observer_type, params, cast, sources):
    """Check constructors, exact mutations, proxies, raw references and lifetime."""
    observer = observer_type()
    number = construct(number_type, family, "", None, params, cast, observer)
    emit(
        "CTOR",
        family,
        "zero",
        render(number)
        + "|"
        + str(int(probe.observer_matches(number, observer)))
        + "|1|1",
    )
    del number

    operations = (
        ("add", "__iadd__", "operator+=", 0),
        ("subtract", "__isub__", "operator-=", 1),
        ("multiply", "__imul__", "operator*=", 2),
        ("divide", "__itruediv__", "operator/=", 3),
    )
    for source_signature, source in source_values(sources).items():
        number = construct(number_type, family, source_signature, source, params, cast, observer)
        emit(
            "CTOR",
            family,
            source_signature,
            render(number)
            + "|"
            + str(int(probe.observer_matches(number, observer)))
            + "|1|1",
        )
        del number

        number = construct(number_type, family, "double", 3.125, params, cast, observer)
        assign = number.__assign__.__overload__(source_signature)
        assigned = assign(source)
        emit("ASSIGN", family, source_signature, render(number) + "|" + str(int(cppyy.addressof(assigned) == cppyy.addressof(number))))
        del assigned, number

        for label, method, native_name, _ in operations:
            number = construct(number_type, family, "double", 3.125, params, cast, observer)
            getattr(number, method).__overload__(source_signature)
            result = getattr(number, method)(source)
            emit(label, family, source_signature, render(number) + "|" + str(int(cppyy.addressof(result) == cppyy.addressof(number))))
            del result, number

    number = construct(number_type, family, "double", 3.125, params, cast, observer)
    number.__preinc__()
    emit("prefix", family, "increment", render(number))
    old_increment = number.__postinc__(0)
    emit("postfix", family, "increment", str(old_increment.to_string(dt.SC_DEC)) + "|" + render(number))
    number.__predec__()
    emit("prefix", family, "decrement", render(number))
    old_decrement = number.__postdec__(0)
    emit("postfix", family, "decrement", str(old_decrement.to_string(dt.SC_DEC)) + "|" + render(number))

    number = construct(number_type, family, "double", 3.125, params, cast, observer)
    # Public references into native-owned parameter and cast storage remain borrowed.
    params_ref = number.type_params()
    cast_ref = number.cast_switch()
    stable_params = int(cppyy.addressof(params_ref) == cppyy.addressof(number.type_params()))
    stable_cast = int(cppyy.addressof(cast_ref) == cppyy.addressof(number.cast_switch()))
    assert not params_ref.__python_owns__ and not cast_ref.__python_owns__
    emit("borrow", family, "type_params_cast", f"{stable_params}|{stable_cast}")

    number = construct(number_type, family, "double", 5.25, params, cast, observer)
    number.bit(0).__assign__(True)
    emit("proxy", family, "bit", str(number.to_string(dt.SC_BIN)))
    number = construct(number_type, family, "double", 5.25, params, cast, observer)
    number.range(3, 0).__assign__(dt.sc_bv[4]("1010"))
    emit("proxy", family, "range", str(number.to_string(dt.SC_BIN)))
    emit(
        "format",
        family,
        "representations",
        "|".join(
            str(value)
            for value in (
                number.to_string(),
                number.to_string(dt.SC_BIN),
                number.to_string(dt.SC_OCT),
                number.to_string(dt.SC_HEX),
                number.to_dec(),
            )
        ),
    )
    snapshot_owner = construct(number_type, family, "double", 7.875, params, cast, observer)
    value_snapshot = snapshot_owner.value()
    snapshot = f"{value_snapshot.to_string(dt.SC_DEC)}|{value_snapshot.to_string(dt.SC_BIN)}"
    assert value_snapshot.__python_owns__
    del snapshot_owner
    gc.collect()
    emit("value", family, "copy_after_owner", f"{snapshot}|1")
    del value_snapshot

    for width, iwl, text in ((137, 69, "9007199254740993.25"), (191, 100, "9007199254740995.5")):
        wide_params = dt.sc_fxtype_params(width, iwl, dt.SC_TRN, dt.SC_WRAP)
        wide = construct(number_type, family, "const char *", text, wide_params, cast, observer)
        emit("precision", family, f"wide{width}", render_precision(wide))
        del wide

    del number, params_ref, cast_ref
    gc.collect()
    del observer
    gc.collect()
    emit("lifetime", family, "observer", str(int(observer_type.destroyed)))


sources = probe.Sources()
values = source_values(sources)
for spelling, expected, alias_name, alias in (
    ("int", "int", "Int", probe.Int),
    ("unsigned int", "unsigned int", "UnsignedInt", probe.UnsignedInt),
    ("long", "long", "Long", probe.Long),
    ("unsigned long", "unsigned long", "UnsignedLong", probe.UnsignedLong),
    ("float", "float", "Float", probe.Float),
    ("double", "double", "Double", probe.Double),
):
    value = values[spelling]
    native_name = getattr(type(value), "__cpp_name__", "")
    assert native_name in {expected, f"sc_dt_fxnums_probe::{alias_name}"}, (spelling, native_name)
    assert type(value) is alias
assert type(values["sc_dt::int64"]).__cpp_name__ == "sc_dt::int64"
assert type(values["sc_dt::uint64"]).__cpp_name__ == "sc_dt::uint64"
assert select_public_signatures(dt.sc_fxnum, "sc_dt::sc_fxnum") == 140
assert select_public_signatures(dt.sc_fxnum_fast, "sc_dt::sc_fxnum_fast") == 140

scoped_params = dt.sc_fxtype_params(13, 5, dt.SC_TRN, dt.SC_WRAP)
scoped_cast = dt.sc_fxcast_switch(dt.SC_OFF)
exercise("sc_fxnum", dt.sc_fxnum, probe.Observer, scoped_params, scoped_cast, sources)
exercise("sc_fxnum_fast", dt.sc_fxnum_fast, probe.FastObserver, scoped_params, scoped_cast, sources)
print("SC_DT_FXNUMS_PUBLIC_OK")
