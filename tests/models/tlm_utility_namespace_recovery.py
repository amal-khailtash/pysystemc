"""Check public TLM utility lookup rejection and subsequent native use."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy


def expect_missing(module, name):
    """Require ordinary module lookup to reject a name with its identity."""
    try:
        getattr(module, name)
    except AttributeError as error:
        assert error.name == name, (name, error.name, error)
    else:
        raise AssertionError(f"unexpected public resolution for {name!r}")


missing_names = (
    "python_user_extension",  # Eligible spelling with no native utility.
    "_missing_tlm_utils_member",  # Private spelling rejected by presentation policy.
    "python_wait",  # A bridge helper in sc_core, not tlm_utils.
    "CallbackQuantumKeeper",  # A bridge helper in pysystemc_interfaces.
)

# Confirm that the bridge candidates exist elsewhere before exercising public
# tlm_utils lookup. Their presence in other namespaces must not leak here.
assert cppyy.gbl.sc_core.python_wait
assert cppyy.gbl.pysystemc_interfaces.CallbackQuantumKeeper

before = dict(vars(tlm_utils))
for missing_name in missing_names:
    assert missing_name not in vars(tlm_utils)
    expect_missing(tlm_utils, missing_name)
    assert missing_name not in vars(tlm_utils)
    assert missing_name not in tlm_utils.__all__
assert vars(tlm_utils) == before

# A failed dynamic lookup must leave normal native resolution usable. Exercise
# real C++ construction and temporal-decoupling values after all rejections.
quantumkeeper_type = getattr(tlm_utils, "tlm_quantumkeeper")
assert quantumkeeper_type is tlm_utils.tlm_quantumkeeper
keeper = quantumkeeper_type()
quantum = sc.sc_time(7, sc.SC_NS)
keeper.set_global_quantum(quantum)
keeper.reset()
assert keeper.get_global_quantum() == quantum
assert keeper.get_local_time() == sc.SC_ZERO_TIME
keeper.inc(sc.sc_time(2, sc.SC_NS))
assert keeper.get_current_time() == sc.sc_time(2, sc.SC_NS)
assert not keeper.need_sync()
keeper.inc(sc.sc_time(5, sc.SC_NS))
assert keeper.get_current_time() == quantum
assert keeper.need_sync()

# Exercise a second real native utility template after the same failed lookups.
queue_type = getattr(tlm_utils, "peq_with_get")[tlm.tlm_generic_payload]
queue = queue_type("namespace_recovery_queue")
assert queue
print("TLM_UTILITY_NAMESPACE_RECOVERY_OK")
