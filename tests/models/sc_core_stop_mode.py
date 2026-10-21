"""Compare public stop-mode configuration and simulator status against native."""

from amal.eda.systemc import sc_core as sc

assert sc.sc_get_stop_mode() == sc.SC_STOP_FINISH_DELTA
sc.sc_set_stop_mode(sc.SC_STOP_IMMEDIATE)
assert sc.sc_get_stop_mode() == sc.SC_STOP_IMMEDIATE
sc.sc_set_stop_mode(sc.SC_STOP_FINISH_DELTA)
assert sc.sc_get_stop_mode() == sc.SC_STOP_FINISH_DELTA
before = sc.sc_get_simulator_status()
sc.sc_start(1, sc.SC_NS, sc.SC_RUN_TO_TIME)
try:
    sc.sc_set_stop_mode(sc.SC_STOP_IMMEDIATE)
except Exception as error:
    assert "stop mode" in str(error)
else:
    raise AssertionError("stop-mode change after start accepted")
assert sc.sc_get_stop_mode() == sc.SC_STOP_FINISH_DELTA
paused = sc.sc_get_simulator_status()
sc.sc_stop()
print("STOP_MODE_REFERENCE", before, paused, sc.sc_get_simulator_status(), int(sc.sc_get_stop_mode()))
