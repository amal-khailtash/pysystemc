"""Verify self-reset and self-kill unwind at the process control boundary."""

from amal.eda.systemc import sc_core as sc

entries = 0
cleanups = 0


def worker():
    """Reset once, then kill self while executing exactly one cleanup per entry."""
    global entries, cleanups
    entries += 1
    try:
        handle = sc.sc_get_current_process_handle()
        if entries == 1:
            handle.reset()
        else:
            handle.kill()
        raise AssertionError("self control returned without unwind")
    finally:
        cleanups += 1


handle = sc.sc_spawn(worker, "self_control")
sc.sc_start(1, sc.SC_NS)
assert entries == cleanups == 2, (entries, cleanups)
assert handle.terminated()
sc.sc_stop()
print("SELF_CONTROL_OK")
