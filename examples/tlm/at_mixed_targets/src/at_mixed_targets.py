"""Accellera mixed AT target port (Apache-2.0; upstream LICENSE/NOTICE)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from models import Bus, Initiator, Target, ns
from amal.eda.systemc import sc_core as sc


class Top(sc.sc_module):
    """Retain mixed targets, native sockets and the 10000 ns limiter."""

    def __init__(self, name):
        """Bind source address windows and register the limiter."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.bus = Bus("m_bus", 3)
        self.targets = [Target(f"target_{i}", 201 + i, style) for i, style in enumerate((1, 2, 4))]
        self.initiators = [Initiator(f"initiator_{i}", 101 + i) for i in range(2)]
        self.initiators[1].base_windows = (1, 2)
        for i in range(2):
            self.initiators[i].socket.bind(self.bus.targets[i])
        for i in range(3):
            self.bus.initiators[i].bind(self.targets[i].socket)
        self.sc_thread("limit_thread", self.limit)

    def limit(self):
        """Stop only after the source simulation limit expires."""
        sc.wait(sc.SC_ZERO_TIME)
        sc.wait(ns(10000))
        sc.sc_stop()


def run():
    """Run and verify traffic completion and source terminal time."""
    top = Top("top")
    sc.sc_start()
    assert all(i.done for i in top.initiators)
    assert not top.bus.pending
    assert sc.sc_time_stamp() == ns(10000)


if __name__ == "__main__":
    run()
