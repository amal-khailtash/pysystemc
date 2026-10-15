"""Exercise Python lifecycle exceptions through the native entry boundary."""

import sys
from amal.eda.systemc import main, sc_core as sc

hook = sys.argv[1]


class Model(sc.sc_module):
    """Raise at one configured lifecycle boundary while retaining its host."""

    def __init__(self, name):
        """Register a valid method before elaboration callbacks execute."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.sc_method("tick", lambda: None)

    def check(self, current):
        """Raise a diagnostic identifying the exact failing lifecycle stage."""
        if current == hook:
            raise ValueError(f"lifecycle_failure_marker:{current}")

    def before_end_of_elaboration(self):
        """Check the before-elaboration exception boundary."""
        self.check("before_end_of_elaboration")

    def end_of_elaboration(self):
        """Check the elaboration-completion exception boundary."""
        self.check("end_of_elaboration")

    def start_of_simulation(self):
        """Check the simulation-start exception boundary."""
        self.check("start_of_simulation")

    def end_of_simulation(self):
        """Check the kernel-stop exception boundary."""
        self.check("end_of_simulation")


models = []


def sc_main(args):
    """Retain the module through entry failure and interpreter cleanup."""
    models.append(Model("broken_lifecycle"))
    sc.sc_start(1, sc.SC_NS)
    sc.sc_stop()
    return 0


main()
