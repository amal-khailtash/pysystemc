"""Accellera loosely timed Python components (Apache-2.0 LICENSE/NOTICE).

Native sockets borrow retained payload/data storage; simulations need subprocesses.
"""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
from models import Initiator, Target, ns
from runtime import report


class Bus(sc.sc_module):
    """Source combinational LT address decoder with native callback sockets."""

    def __init__(self, name):
        """Create two target and initiator sockets and retain callbacks."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.targets = [tlm_utils.callback_target_socket(f"target_{i}") for i in range(2)]
        self.initiators = [tlm_utils.callback_initiator_socket(f"initiator_{i}") for i in range(2)]
        for socket in self.targets:
            socket.register_b_transport(self.transport)

    def transport(self, gp, delay):
        """Mask the source address and pass the mutable delay to its destination."""
        index = int(gp.get_address()) >> 28
        assert index < 2
        gp.set_address(int(gp.get_address()) & 0xFFFFFFF)
        self.initiators[index].get_interface().b_transport(gp, delay)


class LTTarget(Target):
    """Reuse source memory semantics with annotated blocking access delays."""

    def __init__(self, name, identifier, style):
        """Register blocking callback and source target timing."""
        super().__init__(name, identifier, style)
        self.socket.register_b_transport(self.transport)

    def transport(self, gp, delay):
        """Perform memory access now and add source accept/read/write annotations."""
        self.operation(gp)
        memory_delay = self.latency(gp)
        old_delay = str(delay)
        delay.__assign__(delay + ns(self.accept_delay) + memory_delay)
        file = "at_target_1_phase.cpp" if self.style == 1 else "lt_target.cpp"
        function = "b_transport" if self.style == 1 else "custom_b_transport"
        report(
            file,
            function,
            f"Target: {self.identifier} returned delay of {old_delay} + {ns(self.accept_delay)} + {memory_delay} = {delay}",
        )


class LTInitiator(Initiator):
    """Source blocking initiator with retained traffic and delay waits."""

    def initiate(self):
        """Send blocking native transactions, honor annotation and return payloads."""
        while True:
            while not self.request_queue:
                sc.wait(self.request_event)
            gp = self.request_queue.pop(0)
            delay = ns(0)
            report("lt_initiator.cpp", "initiator_thread", f"Initiator: {self.identifier} b_transport(GP, {delay})")
            self.socket.get_interface().b_transport(gp, delay)
            assert gp.get_response_status() == tlm.TLM_OK_RESPONSE
            report(
                "lt_initiator.cpp",
                "initiator_thread",
                f"Initiator: {self.identifier} b_transport returned delay = {delay}",
            )
            sc.wait(delay)
            self.complete(gp)


def run():
    """Run native LT topology and verify both complete 64-transaction streams."""
    module_name = sc.sc_module_name("top")
    top = sc.sc_module(module_name)
    bus = Bus("m_bus")
    targets = [LTTarget("target_1", 201, 1), LTTarget("target_2", 202, 0)]
    targets[0].accept_delay, targets[0].read_delay, targets[0].write_delay = 20, 100, 60
    initiators = [LTInitiator(f"initiator_{i}", 101 + i) for i in range(2)]
    for i in range(2):
        initiators[i].base_offset = 0
        initiators[i].socket.bind(bus.targets[i])
        bus.initiators[i].bind(targets[i].socket)
    del module_name
    sc.sc_start()
    assert all(i.done for i in initiators) and top is not None
