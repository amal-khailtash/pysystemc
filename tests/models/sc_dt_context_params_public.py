"""Compare public fixed-point parameter and context bindings with native SystemC."""

from amal.eda.systemc import sc_dt
import cppyy


def flat(text):
    """Flatten captured native stream output into one comparison record."""
    if isinstance(text, bytes):
        text = text.decode()
    return text.replace("\n", "~")


def stream_output(value, method):
    """Call a public native formatter into an independent C++ string stream."""
    output = cppyy.gbl.std.ostringstream()
    getattr(value, method)(output)
    return flat(output.str())


def emit_fx(name, value):
    """Emit all public fixed type parameter values and stream formats."""
    print(
        "FX",
        name,
        value.to_string(),
        value.wl(),
        value.iwl(),
        int(value.q_mode()),
        int(value.o_mode()),
        value.n_bits(),
        stream_output(value, "print"),
        stream_output(value, "dump"),
    )


def emit_length(name, value):
    """Emit public length parameter values and stream formats."""
    print(
        "LENGTH",
        name,
        value.to_string(),
        value.len(),
        stream_output(value, "print"),
        stream_output(value, "dump"),
    )


def emit_cast(name, value):
    """Emit public cast switch value and stream formats."""
    print("CAST", name, value.to_string(), stream_output(value, "print"), stream_output(value, "dump"))


def context_matrix(family, context_type, seed, replacement):
    """Check copied context values and default restoration through destruction."""
    baseline = context_type.default_value()
    print("CTX", family, "BASE", baseline.to_string())
    later = context_type(seed, sc_dt.SC_LATER)
    seed.__assign__(replacement)
    assert later.value() != seed
    later_default = type(seed)()
    assert later_default == baseline
    print(
        "CTX",
        family,
        "LATER",
        context_type.default_value().to_string(),
        later.value().to_string(),
        later_default.to_string(),
    )
    later.begin()
    assert context_type.default_value() == later.value()
    active_default = type(seed)()
    assert active_default == later.value()
    print(
        "CTX",
        family,
        "ACTIVE",
        context_type.default_value().to_string(),
        later.value().to_string(),
        active_default.to_string(),
    )
    now = context_type(replacement, sc_dt.SC_NOW)
    assert context_type.default_value() == now.value()
    nested_default = type(seed)()
    assert nested_default == now.value()
    print(
        "CTX",
        family,
        "NESTED",
        context_type.default_value().to_string(),
        now.value().to_string(),
        nested_default.to_string(),
    )
    now.__destruct__()
    assert context_type.default_value() == later.value()
    later.end()
    assert context_type.default_value() == baseline
    print("CTX", family, "RESTORED", context_type.default_value().to_string())
    active = context_type(seed, sc_dt.SC_LATER)
    active.begin()
    assert context_type.default_value() == active.value()
    active.__destruct__()
    assert context_type.default_value() == baseline
    print("CTX", family, "DESTROYED", context_type.default_value().to_string())
    later.__destruct__()


def fixed_type_matrix():
    """Exercise each public fixed type constructor, operator, accessor and formatter."""
    defaults = sc_dt.sc_fxtype_params()
    widths = sc_dt.sc_fxtype_params(17, 5)
    modes = sc_dt.sc_fxtype_params(sc_dt.SC_RND, sc_dt.SC_SAT, 3)
    full = sc_dt.sc_fxtype_params(19, 7, sc_dt.SC_RND_CONV, sc_dt.SC_WRAP_SM, 2)
    copied = sc_dt.sc_fxtype_params(full)
    changed_widths = sc_dt.sc_fxtype_params(full, 23, 11)
    changed_modes = sc_dt.sc_fxtype_params(full, sc_dt.SC_RND_MIN_INF, sc_dt.SC_SAT_SYM, 4)
    without_context = sc_dt.sc_fxtype_params(sc_dt.sc_without_context())
    for name, value in (
        ("default", defaults),
        ("widths", widths),
        ("modes", modes),
        ("full", full),
        ("copy", copied),
        ("copy_widths", changed_widths),
        ("copy_modes", changed_modes),
        ("without_context", without_context),
    ):
        emit_fx(name, value)
    assert copied == full and not copied != full
    assert widths != full and not widths == full
    assigned = sc_dt.sc_fxtype_params()
    assigned.__assign__(full)
    assigned.__assign__(assigned)
    assert assigned == full
    emit_fx("assigned", assigned)
    widths.wl(21)
    widths.iwl(8)
    widths.q_mode(sc_dt.SC_TRN_ZERO)
    widths.o_mode(sc_dt.SC_SAT_ZERO)
    widths.n_bits(5)
    assert (
        widths.wl() == 21
        and widths.iwl() == 8
        and widths.q_mode() == sc_dt.SC_TRN_ZERO
        and widths.o_mode() == sc_dt.SC_SAT_ZERO
        and widths.n_bits() == 5
    )
    emit_fx("mutated", widths)


def length_matrix():
    """Exercise each public length parameter constructor, operator and accessor."""
    defaults = sc_dt.sc_length_param()
    integer = sc_dt.sc_length_param(13)
    copied = sc_dt.sc_length_param(integer)
    without_context = sc_dt.sc_length_param(sc_dt.sc_without_context())
    assert copied == integer and not copied != integer
    assert defaults != integer and not defaults == integer
    assigned = sc_dt.sc_length_param()
    assigned.__assign__(integer)
    assigned.__assign__(assigned)
    assert assigned == integer
    for name, value in (
        ("default", defaults),
        ("integer", integer),
        ("copy", copied),
        ("without_context", without_context),
        ("assigned", assigned),
    ):
        emit_length(name, value)
    integer.len(27)
    assert integer.len() == 27
    emit_length("mutated", integer)


def cast_matrix():
    """Exercise each public cast switch constructor, operator and formatter."""
    defaults = sc_dt.sc_fxcast_switch()
    selected = sc_dt.sc_fxcast_switch(sc_dt.SC_OFF)
    copied = sc_dt.sc_fxcast_switch(selected)
    without_context = sc_dt.sc_fxcast_switch(sc_dt.sc_without_context())
    assert copied == selected and not copied != selected
    assert defaults != selected and not defaults == selected
    assigned = sc_dt.sc_fxcast_switch()
    assigned.__assign__(selected)
    assigned.__assign__(assigned)
    assert assigned == selected
    for name, value in (
        ("default", defaults),
        ("selected", selected),
        ("copy", copied),
        ("without_context", without_context),
        ("assigned", assigned),
    ):
        emit_cast(name, value)


fixed_type_matrix()
length_matrix()
cast_matrix()
context_matrix(
    "FX",
    sc_dt.sc_fxtype_context,
    sc_dt.sc_fxtype_params(29, 13),
    sc_dt.sc_fxtype_params(31, 15, sc_dt.SC_RND, sc_dt.SC_SAT, 1),
)
context_matrix("LENGTH", sc_dt.sc_length_context, sc_dt.sc_length_param(14), sc_dt.sc_length_param(18))
context_matrix(
    "CAST", sc_dt.sc_fxcast_context, sc_dt.sc_fxcast_switch(sc_dt.SC_OFF), sc_dt.sc_fxcast_switch(sc_dt.SC_ON)
)
print("SC_DT_CONTEXT_PARAMS_PUBLIC_OK")
