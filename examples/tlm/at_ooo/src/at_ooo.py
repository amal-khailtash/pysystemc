"""Accellera out-of-order AT port (Apache-2.0; upstream LICENSE/NOTICE)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from models import Bus, Initiator, Target
from amal.eda.systemc import sc_core as sc


def run():
    """Retain two targets and verify source alternating delayed responses."""
    module_name = sc.sc_module_name("top")
    top = sc.sc_module(module_name)
    bus = Bus("m_bus")
    targets = [Target(f"target_{i}", 201 + i, 2) for i in range(2)]
    targets[1].accept_delay, targets[1].read_delay, targets[1].write_delay = 20, 100, 60
    targets[1].out_of_order = True
    targets[1].component_file = "at_target_ooo_2_phase.cpp"
    initiators = [Initiator(f"initiator_{i}", 101 + i) for i in range(2)]
    for i in range(2):
        initiators[i].socket.bind(bus.targets[i])
        bus.initiators[i].bind(targets[i].socket)
    del module_name
    sc.sc_start()
    assert all(i.done for i in initiators) and not bus.pending
    assert top is not None


if __name__ == "__main__":
    run()
