"""Compare selected omitted defaults with explicit native context/template args."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


context = sc.sc_get_curr_simcontext()
functions = (
    "sc_get_top_level_objects",
    "sc_get_top_level_events",
    "sc_is_running",
    "sc_pending_activity_at_current_time",
    "sc_pending_activity_at_future_time",
    "sc_time_to_pending_activity",
)
for name in functions:
    function = getattr(sc, name)
    defaulted = function()
    explicit = function(context)
    native = getattr(cppyy.gbl.sc_core, name)(context)
    if name in ("sc_get_top_level_objects", "sc_get_top_level_events"):
        assert cppyy.addressof(defaulted) == cppyy.addressof(explicit) == cppyy.addressof(native), name
        assert defaulted.size() == 0, name
    elif name == "sc_time_to_pending_activity":
        assert defaulted == explicit == native, name
        assert int(defaulted.value()) == int(native.value()), name
    else:
        assert bool(defaulted) == bool(explicit) == bool(native), name

for name in ("sc_fixed", "sc_fixed_fast", "sc_ufixed", "sc_ufixed_fast"):
    template = getattr(dt, name)
    defaulted = template[16, 4]
    explicit = template[
        16,
        4,
        "sc_dt::SC_DEFAULT_Q_MODE_",
        "sc_dt::SC_DEFAULT_O_MODE_",
        "sc_dt::SC_DEFAULT_N_BITS_",
    ]
    assert defaulted is explicit, name
    value = defaulted(1.5)
    native_value = explicit(1.5)
    assert float(value.to_double()) == float(native_value.to_double()) == 1.5, name
    assert value.wl() == 16 and value.iwl() == 4, name
print("SECTION1_RESIDUAL_DEFAULTS context_calls=6 borrowed_vectors=2 templates=4 value=1.5")
