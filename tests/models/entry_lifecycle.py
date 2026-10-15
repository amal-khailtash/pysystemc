"""Verify entry argv ownership and start/pause/stop lifecycle state transitions."""

import sys
from amal.eda.systemc import main, sc_core as sc

records = []


class Model(sc.sc_module):
    """Track bounded simulation and terminal lifecycle behavior."""

    def __init__(self, name):
        """Register a thread that pauses then resumes at a later timestamp."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.sc_thread("run", self.run)

    def run(self):
        """Pause at 1 ns and resume for another timed wait."""
        sc.wait(1, sc.SC_NS)
        records.append(("pause", str(sc.sc_time_stamp())))
        sc.sc_pause()
        sc.wait(1, sc.SC_NS)
        records.append(("resume", str(sc.sc_time_stamp())))

    def end_of_simulation(self):
        """Record the terminal hook exactly once."""
        records.append(("end", str(sc.sc_time_stamp())))


models = []


def sc_main(args):
    """Validate UTF-8 argv copying and independent start/pause/stop state."""
    assert args == sys.argv
    original = list(args)
    args[0] = "changed"
    assert sc.python_argv_at(0) == original[0]
    assert sys.argv == original
    assert sc.sc_argc() == len(original)
    models.append(Model("entry_lifecycle"))
    sc.sc_start(5, sc.SC_NS)
    assert sc.sc_get_status() == sc.SC_PAUSED
    assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
    sc.sc_start(2, sc.SC_NS)
    assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
    sc.sc_stop()
    sc.sc_stop()
    assert records == [("pause", "1 ns"), ("resume", "2 ns"), ("end", "3 ns")], records
    assert sc.sc_get_status() == sc.SC_STOPPED
    print("ENTRY_LIFECYCLE_OK")
    return 0


main()
