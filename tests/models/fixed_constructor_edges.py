"""Exercise remaining exact fixed-point constructor adapter branches."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

from amal.eda.systemc.sc_dt import _fixed_constructors as constructors


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_dt_fixed_templates_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.fixed_templates_probe


def render(value, observer):
    """Render a value, its selected policies, observer address, and dispatch.

    Args:
        value: Constructed fixed-point number.
        observer: Live observer borrowed by the number.

    Returns:
        Stable fields for comparison with the independent C++ model.
    """
    rendered = value.to_string(dt.SC_DEC)
    return "|".join(
        map(
            str,
            (
                rendered,
                value.wl(),
                value.iwl(),
                int(value.q_mode()),
                int(value.o_mode()),
                int(value.cast_switch() == dt.SC_ON),
                int(probe.observer_matches(value, observer)),
                observer.constructed,
                observer.reads,
                observer.writes,
            ),
        )
    )


def emit(label, *values):
    """Print one native-comparable or Python-only constructor record."""
    print("FCE\t" + "\t".join(map(str, (label, *values))))


def constructor_cases():
    """Compare primitive, typedef scalar, and fixed-value source overloads."""
    fixed_type = dt.sc_fixed[61, 50]
    observer = probe.Observer()
    cast = dt.sc_fxcast_switch(dt.SC_OFF)

    for label, source in (
        ("small_int", 7),
        ("floating", 1.375),
        ("native_scalar", cppyy.gbl.sc_dt.uint64((1 << 40) + 3)),
        ("fixed_value", dt.sc_fxval("2.375")),
    ):
        value = fixed_type(source, observer)
        emit(label, render(value, observer))
        del value

    source = dt.sc_fxval("3.625")
    value = fixed_type(source, cast, observer)
    emit("template_source_cast_observer", render(value, observer))
    del value, source, observer


def configurable_cases():
    """Compare runtime policy constructor suffixes for slow and fast owners."""
    observer = probe.Observer()
    q_mode, o_mode = dt.SC_RND_CONV, dt.SC_SAT
    value = dt.sc_fix(1.9375, q_mode, o_mode, observer)
    emit("config_modes", render(value, observer))
    del value, observer

    observer = probe.Observer()
    value = dt.sc_fix(2.375, 19, 11, dt.SC_TRN_ZERO, dt.SC_WRAP, 2, observer)
    emit("config_width_modes_n", render(value, observer))
    del value, observer

    fast_observer = probe.FastObserver()
    fast_type = dt.sc_fixed_fast[41, 32]
    fast_value = fast_type(cppyy.gbl.sc_dt.uint64((1 << 30) + 9), fast_observer)
    emit("fast_native_scalar", render(fast_value, fast_observer))
    del fast_value, fast_observer


def default_copy_and_adapter_cases():
    """Check native defaults/copies and idempotent type adaptation."""
    fixed_type = dt.sc_fixed[23, 11]
    default_value = fixed_type()
    emit(
        "default",
        "|".join(map(str, (default_value.to_string(dt.SC_DEC), default_value.wl(), default_value.iwl()))),
    )
    source = fixed_type(7.5)
    copied_value = fixed_type(source)
    del source
    assert str(copied_value.to_string(dt.SC_DEC)) == "7.5"
    emit("copy", "|".join(map(str, (copied_value.to_string(dt.SC_DEC), copied_value.wl(), copied_value.iwl()))))

    native_type = dt.sc_fixed[29, 13]
    original_init = native_type.__init__
    assert native_type.__dict__.get("_pysystemc_exact_constructor") is True
    constructors._adapt(native_type)
    assert native_type.__init__ is original_init
    emit("adapt_idempotent", int(native_type.__init__ is original_init))

    rejected = False
    try:
        native_type(3, dt.sc_fxcast_switch(dt.SC_OFF), 99)
    except TypeError:
        rejected = True
    assert rejected
    emit("malformed_trailing", int(rejected))


constructor_cases()
configurable_cases()
default_copy_and_adapter_cases()
