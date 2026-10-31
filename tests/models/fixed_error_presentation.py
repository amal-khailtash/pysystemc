"""Exercise real fixed-point constructor failures and a later wide observer."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_dt_fixed_templates_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.fixed_templates_probe
WIDE_VALUE = (1 << 90) + 0x13579


def explicit_shape_error():
    """Return the adapter's explicit unsupported-parameter-shape error.

    Raises:
        AssertionError: The exercised native argument shape is accepted.
    """
    parameters = dt.sc_fxtype_params(17, 9)
    try:
        dt.sc_fix(parameters, dt.SC_TC_)
    except TypeError as error:
        assert error.__cause__ is None
        assert "sc_fix constructor" in str(error)
        assert "(sc_fxtype_params, sc_enc)" in str(error)
        assert "supported configuration forms" in str(error)
        assert "sc_fxcast_switch" in str(error)
        return error
    raise AssertionError("sc_fix accepted an unsupported native parameter shape")


def native_fallback_error():
    """Return cppyy's candidate-rich error for a malformed real constructor call.

    Raises:
        AssertionError: The malformed constructor call is accepted or loses its
            native overload-candidate diagnostic.
    """
    fixed_type = dt.sc_fixed[73, 41]
    cast = dt.sc_fxcast_switch(dt.SC_OFF)
    try:
        fixed_type(3, cast, 99)
    except TypeError as error:
        assert error.__cause__ is None
        assert "none of the" in str(error)
        assert "sc_fixed<73,41" in str(error)
        return error
    raise AssertionError("sc_fixed accepted a malformed native constructor shape")


def wide_observer_recovery():
    """Construct and inspect an arbitrary-width value after constructor errors."""
    observer = probe.Observer()
    fixed_type = dt.sc_fixed[137, 100]
    value = fixed_type(WIDE_VALUE, observer)
    assert value.to_string(dt.SC_DEC) == str(WIDE_VALUE)
    assert probe.observer_matches(value, observer)
    if probe.observers_enabled():
        assert observer.constructed == 1
    rendered = str(value.to_string(dt.SC_DEC))
    del value
    gc.collect()
    if probe.observers_enabled():
        assert observer.destructed == 1
    return rendered, 1


explicit_shape_error()
native_fallback_error()
wide_value, observer_matched = wide_observer_recovery()
print("FEP\texplicit_shape\tsc_fix\t(sc_fxtype_params, sc_enc)")
print("FEP\tnative_candidates\tsc_fixed<73,41>")
print(f"FEP\twide_recovery\t{wide_value}\t{observer_matched}")
