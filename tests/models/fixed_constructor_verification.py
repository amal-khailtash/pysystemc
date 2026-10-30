"""Exercise distinct fixed-point constructor routes through public types."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_dt_fxnums_probe.h"
OBSERVER_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_dt_fixed_templates_probe.h"
cppyy.include(str(PROBE_HEADER))
cppyy.include(str(OBSERVER_HEADER))
probe = cppyy.gbl.sc_dt_fxnums_probe
observer_probe = cppyy.gbl.fixed_templates_probe
WIDE_INTEGER = (1 << 90) + 0x13579
FAST_INTEGER = (1 << 40) + 0x2468B


def render(value, observer):
    """Copy exact fixed-point value, policy, cast, and observer state.

    Args:
        value: Live native fixed-point value.
        observer: Native observer whose borrowed address should be installed.

    Returns:
        A stable pipe-delimited record matching the C++ reference renderer.
    """
    return "|".join(
        map(
            str,
            (
                value.to_string(dt.SC_DEC),
                value.wl(),
                value.iwl(),
                int(value.q_mode()),
                int(value.o_mode()),
                value.n_bits(),
                int(value.cast_switch() == dt.SC_ON),
                int(probe.observer_matches(value, observer)),
            ),
        )
    )


def emit(label, *values):
    """Print one deterministic value or rejected-constructor record."""
    print("FCV\t" + "\t".join(map(str, (label, *values))))


def fixed_template_cases():
    """Check cast-only fallback and exact wide-integer template construction."""
    fixed_type = dt.sc_fixed[137, 100]
    cast = dt.sc_fxcast_switch(dt.SC_OFF)
    observer = observer_probe.Observer()

    cast_only = fixed_type(cast, observer)
    assert probe.observer_matches(cast_only, observer)
    emit("template_cast_only", render(cast_only, observer))
    del cast_only

    wide = fixed_type(WIDE_INTEGER, observer)
    assert probe.observer_matches(wide, observer)
    assert wide.to_string(dt.SC_DEC) == str(WIDE_INTEGER)
    emit("template_wide_integer", render(wide, observer))
    del wide, observer
    gc.collect()


def configurable_cases():
    """Check dynamic configuration-only/source overloads and rejected inputs."""
    parameters = dt.sc_fxtype_params(137, 100, dt.SC_TRN, dt.SC_WRAP, 0)
    cast = dt.sc_fxcast_switch(dt.SC_OFF)
    observer = observer_probe.Observer()

    parameter_only = dt.sc_fix(parameters, cast, observer)
    assert probe.observer_matches(parameter_only, observer)
    emit("config_parameter_only", render(parameter_only, observer))
    del parameter_only

    source = dt.sc_fix(WIDE_INTEGER, parameters, cast, observer)
    assert probe.observer_matches(source, observer)
    assert source.to_string(dt.SC_DEC) == str(WIDE_INTEGER)
    emit("config_wide_integer", render(source, observer))
    del source

    before_invalid = observer.constructed
    invalid_source = invalid_shape = False
    try:
        dt.sc_fix(object(), parameters, cast, observer)
    except TypeError:
        invalid_source = True
    try:
        dt.sc_fix(parameters, dt.SC_TC_, observer)
    except TypeError:
        invalid_shape = True
    assert invalid_source and invalid_shape
    assert observer.constructed == before_invalid
    emit("rejected_fx_config", int(invalid_source and invalid_shape), int(observer.constructed == before_invalid))
    del observer
    gc.collect()


def owner_case(label, number_type, observer_type, integer, width, iwl, encoding):
    """Verify one owned-base source route and returned value storage lifetime.

    Args:
        label: Stable record name for slow or fast owner behavior.
        number_type: Public ``sc_fxnum`` or ``sc_fxnum_fast`` owner facade.
        observer_type: Matching native observer fixture class.
        integer: Exact positive Python integer source.
        width: Native word length in ``sc_fxtype_params``.
        iwl: Native integer word length in ``sc_fxtype_params``.
        encoding: Native sign encoding for the number owner.
    """
    destroyed_before = observer_type.destroyed
    observer = observer_type()
    parameters = dt.sc_fxtype_params(width, iwl, dt.SC_TRN, dt.SC_WRAP, 0)
    cast = dt.sc_fxcast_switch(dt.SC_OFF)
    number = number_type(integer, parameters, encoding, cast, observer)
    assert number.__python_owns__
    assert probe.observer_matches(number, observer)
    assert number.to_string(dt.SC_DEC) == str(integer)
    emit(label, render(number, observer))

    snapshot = number.value()
    assert snapshot.__python_owns__
    del number
    gc.collect()
    assert snapshot.to_string(dt.SC_DEC) == str(integer)
    emit(label + "_snapshot", snapshot.to_string(dt.SC_DEC), snapshot.to_string(dt.SC_BIN))
    del snapshot, observer
    gc.collect()
    assert observer_type.destroyed == destroyed_before + 1


def owner_cases():
    """Check owned slow/fast source conversion, observers, and boundaries."""
    default_rejected = unsupported_rejected = False
    try:
        dt.sc_fxnum()
    except TypeError:
        default_rejected = True
    assert default_rejected

    before = observer_probe.Observer.destroyed
    slow_observer = observer_probe.Observer()
    try:
        dt.sc_fxnum(dt.sc_logic("1"), dt.sc_fxtype_params(16, 8), dt.SC_TC_, dt.sc_fxcast_switch(dt.SC_OFF), slow_observer)
    except TypeError:
        unsupported_rejected = True
    assert unsupported_rejected
    if observer_probe.observers_enabled():
        assert slow_observer.constructed == 0
    del slow_observer
    gc.collect()
    assert observer_probe.Observer.destroyed == before + 1

    owner_case("owner_slow", dt.sc_fxnum, observer_probe.Observer, WIDE_INTEGER, 137, 100, dt.SC_TC_)
    owner_case("owner_fast", dt.sc_fxnum_fast, observer_probe.FastObserver, FAST_INTEGER, 53, 45, dt.SC_TC_)
    emit("rejected_owner_default", int(default_rejected))
    emit("rejected_owner_source", int(unsupported_rejected))


fixed_template_cases()
configurable_cases()
owner_cases()
