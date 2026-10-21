"""Compare native time configuration, immutable limits and rejection recovery."""

from amal.eda.systemc import sc_core as sc

errors = []


def rejected(function, *args):
    """Require a native configuration diagnostic without accepting invalid state."""
    try:
        function(*args)
    except Exception as error:
        assert "Error" in str(error)
        errors.append(True)
    else:
        raise AssertionError("invalid configuration accepted")


rejected(sc.sc_set_time_resolution, -1, sc.SC_PS)
rejected(sc.sc_set_time_resolution, 3, sc.SC_PS)
sc.sc_set_time_resolution(10, sc.SC_PS)
rejected(sc.sc_set_default_time_unit, -1, sc.SC_NS)
rejected(sc.sc_set_default_time_unit, 3, sc.SC_NS)
sc.sc_set_default_time_unit(10, sc.SC_NS)
assert sc.sc_get_time_resolution() == sc.sc_time(10, sc.SC_PS)
assert sc.sc_get_default_time_unit() == sc.sc_time(10, sc.SC_NS)
rejected(sc.sc_set_time_resolution, 10, sc.SC_PS)
rejected(sc.sc_set_default_time_unit, 10, sc.SC_NS)
limit = sc.sc_max_time()
assert limit.value() == 2**64 - 1
sc.sc_start(25, sc.SC_NS, sc.SC_RUN_TO_TIME)
assert sc.sc_simulation_time() == 2.5
assert sc.sc_max_time().value() == limit.value()
rejected(sc.sc_set_time_resolution, 10, sc.SC_PS)
rejected(sc.sc_set_default_time_unit, 10, sc.SC_NS)
sc.sc_stop()
print(
    "TIME_CONFIGURATION_REFERENCE",
    sc.sc_get_time_resolution().value(),
    sc.sc_get_default_time_unit().value(),
    limit.value(),
    sc.sc_time_stamp().value(),
    sc.sc_simulation_time(),
    len(errors),
)
