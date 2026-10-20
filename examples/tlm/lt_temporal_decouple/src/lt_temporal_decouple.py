"""Accellera temporal decoupling port (Apache-2.0 LICENSE/NOTICE).

Native quantum keeper and sockets retain source local-time/synchronization behavior.
Callbacks and payload buffers remain alive through native process completion.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from lt_models import Bus, LTTarget, LTInitiator
from models import ns
from runtime import report
from amal.eda.systemc import sc_core as sc, tlm, tlm_utils


class SynchTarget(LTTarget):
    """Consume the complete annotated delay inside the source target callback."""

    def transport(self, gp, delay):
        """Copy memory, force native wait and return zero local delay."""
        self.operation(gp)
        delay.__assign__(delay + ns(self.accept_delay) + self.latency(gp))
        report(
            "lt_synch_target.cpp",
            "custom_b_transport",
            f"Target: {self.identifier} Forcing a synch in a temporal decoupled initiator with wait( {delay}),",
        )
        sc.wait(delay)
        delay.__assign__(sc.SC_ZERO_TIME)
        report(
            "lt_synch_target.cpp",
            "custom_b_transport",
            f"Target: {self.identifier} return from wait will return a delay of {delay}",
        )


class TDInitiator(LTInitiator):
    """Track native local time with the upstream 500 ns global quantum."""

    def __init__(self, name, identifier):
        """Retain native quantum keeper before the source initiator executes."""
        super().__init__(name, identifier)
        tlm_utils.tlm_quantumkeeper.set_global_quantum(ns(500))
        self.keeper = tlm_utils.tlm_quantumkeeper()
        self.sync_count = 0

    def initiate(self):
        """Set target annotation and synchronize only at native quantum boundaries."""
        while True:
            while not self.request_queue:
                sc.wait(self.request_event)
            gp = self.request_queue.pop(0)
            delay = self.keeper.get_local_time()
            report("lt_td_initiator.cpp", "initiator_thread", f"Initiator: {self.identifier} b_transport(GP, {delay})")
            self.socket.get_interface().b_transport(gp, delay)
            assert gp.get_response_status() == tlm.TLM_OK_RESPONSE
            report(
                "lt_td_initiator.cpp",
                "initiator_thread",
                f"Initiator: {self.identifier} b_transport returned delay = {delay} and quantum keeper to be set\n      ",
            )
            self.keeper.set(delay)
            if self.keeper.need_sync():
                report(
                    "lt_td_initiator.cpp",
                    "initiator_thread",
                    f"Initiator: {self.identifier} the quantum keeper needs synching",
                )
                self.keeper.sync()
                self.sync_count += 1
                report(
                    "lt_td_initiator.cpp",
                    "initiator_thread",
                    f"Initiator: {self.identifier} return from quantum keeper synch",
                )
            self.complete(gp)


def run():
    """Run source topology and assert all readbacks and quantum synchronization."""
    module_name = sc.sc_module_name("top")
    top = sc.sc_module(module_name)
    bus = Bus("m_bus")
    targets = [SynchTarget("target_1", 201, 0), LTTarget("target_2", 202, 0)]
    targets[0].accept_delay, targets[0].read_delay, targets[0].write_delay = 20, 100, 60
    initiators = [TDInitiator("td_initiator", 101), LTInitiator("initiator", 102)]
    for i in range(2):
        initiators[i].base_offset = 0
        initiators[i].socket.bind(bus.targets[i])
        bus.initiators[i].bind(targets[i].socket)
    del module_name
    sc.sc_start()
    assert all(i.done for i in initiators)
    assert initiators[0].sync_count > 0 and top is not None


if __name__ == "__main__":
    run()
