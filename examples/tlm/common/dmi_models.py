"""Accellera LT DMI components in Python (Apache-2.0 LICENSE/NOTICE).

Preserves source range calculations, including its ineffective invalidation test.
Native pointers borrow retained buffers and remain alive until all callbacks end.
"""

from array import array
from amal.eda.systemc import sc_core as sc, tlm
from lt_models import Bus as LTBus, LTTarget, LTInitiator
from models import ns
from runtime import report


class Bus(LTBus):
    """Forward native DMI requests and remap invalidation ranges to bus windows."""

    def __init__(self, name):
        """Register pointer forwarding and per-destination invalidation callbacks."""
        super().__init__(name)
        for socket in self.targets:
            socket.register_get_direct_mem_ptr(self.dmi)
        for i, socket in enumerate(self.initiators):
            socket.register_invalidate_direct_mem_ptr(lambda first, last, index=i: self.invalidate(index, first, last))

    def dmi(self, gp, result):
        """Mask native address then restore granted region into the bus address map."""
        index = int(gp.get_address()) >> 28
        gp.set_address(int(gp.get_address()) & 0xFFFFFFF)
        granted = self.initiators[index].get_interface().get_direct_mem_ptr(gp, result)
        result.set_start_address(int(result.get_start_address()) + (index << 28))
        result.set_end_address(int(result.get_end_address()) + (index << 28))
        return granted

    def invalidate(self, index, first, last):
        """Broadcast remapped source invalidation to both native initiator callbacks."""
        for socket in self.targets:
            socket.get_base_port().get_interface().invalidate_direct_mem_ptr(
                first + (index << 28), last + (index << 28)
            )


class Target(LTTarget):
    """Grant retained native memory while preserving source enable/disable times."""

    def __init__(self, name, identifier):
        """Create memory and register source initializing toggle method."""
        super().__init__(name, identifier, 0)
        self.memory = array("B", [0] * 4096)
        self.enabled = True
        self.toggles = 0
        self.accept_delay = 20
        self.read_delay, self.write_delay = (20, 15) if identifier == 201 else (50, 30)
        self.socket.register_get_direct_mem_ptr(self.dmi)
        self.sc_method("toggle_dmi_method", self.toggle)

    def transport(self, gp, delay):
        """Copy native memory and expose the source DMI hint at request time."""
        self.operation(gp)
        old = str(delay)
        latency = self.latency(gp)
        delay.__assign__(delay + ns(20) + latency)
        gp.set_dmi_allowed(self.enabled)
        report(
            "lt_dmi_target.cpp",
            "custom_b_transport",
            f"Target: {self.identifier} returned delay of {old} + 20 ns + {latency} = {delay}",
        )

    def operation(self, gp):
        """Reuse source transfer while adapting array storage to bytes slice writes."""
        # The native memory owner must retain its address for all DMI grants.
        address = int(gp.get_address())
        data = gp.get_data_ptr()
        write = gp.get_command() == tlm.TLM_WRITE_COMMAND
        for i in range(4):
            if write:
                self.memory[address + i] = data[i]
            else:
                data[i] = self.memory[address + i]
        value = int.from_bytes(bytes(data[i] for i in range(4)), "little")
        report(
            "memory.cpp",
            "print",
            f"ID: {self.identifier} COMMAND: {'WRITE' if write else 'READ'} Length: 04\n      Addr: 0x{address:016X} Data: 0x{value:08X}",
        )
        gp.set_response_status(tlm.TLM_OK_RESPONSE)

    def dmi(self, gp, result):
        """Grant source inclusive 0..4096 region and native read/write pointer."""
        if not self.enabled:
            return False
        result.allow_read_write()
        result.set_start_address(0)
        result.set_end_address(4096)
        result.set_dmi_ptr(self.memory)
        result.set_read_latency(ns(self.read_delay))
        result.set_write_latency(ns(self.write_delay))
        report(
            "lt_dmi_target.cpp",
            "get_direct_mem_ptr",
            f"Target: {self.identifier} passing DMI pointer back to initiator",
        )
        return True

    def toggle(self):
        """Disable at 0 ns, enable at100 ns and disable permanently at1100 ns."""
        self.toggles += 1
        self.enabled = not self.enabled
        if not self.enabled:
            report(
                "lt_dmi_target.cpp", "toggle_dmi_method", f"Target: {self.identifier} invalidate_direct_ptr (0, 4096);"
            )
            self.socket.get_base_port().get_interface().invalidate_direct_mem_ptr(0, 4096)
            sc.next_trigger(ns(100))
        else:
            report(
                "lt_dmi_target.cpp",
                "toggle_dmi_method",
                f"Target: {self.identifier} DMI has been enabled in this target",
            )
            sc.next_trigger(ns(1000))
        if self.toggles >= 3:
            self.enabled = False
            sc.next_trigger()


class Initiator(LTInitiator):
    """Retain one source DMI region, check every native pointer access and latency."""

    def __init__(self, name, identifier):
        """Register invalidation and use source shared zero-offset address windows."""
        super().__init__(name, identifier)
        self.base_offset = 0
        self.properties = tlm.tlm_dmi()
        self.base = self.size = 0
        self.granted = False
        self.direct_count = 0
        self.socket.register_invalidate_direct_mem_ptr(self.invalidate)

    def invalidate(self, first, last):
        """Preserve source unsigned subtraction in its invalidation range check."""
        invalid = first <= last and first >= self.base and last <= ((self.base - self.size) & ((1 << 64) - 1))
        report(
            "dmi_memory.cpp",
            "invalidate_dmi_ptr",
            f"Initiator:{self.identifier} DMI Pointer {'invalidated' if invalid else 'not invalidated'} for ({first}, {last})",
        )
        # Source changes temporary bounds here; subsequent is_address_dmi
        # recomputes them and retains the granted region.

    def initiate(self):
        """Use native DMI pointers where source bounds permit, otherwise transport."""
        while True:
            while not self.request_queue:
                sc.wait(self.request_event)
            gp = self.request_queue.pop(0)
            address = int(gp.get_address())
            delay = ns(0)
            report(
                "lt_dmi_initiator.cpp",
                "initiator_thread",
                f"Initiator: {self.identifier} b_transport with delay of 0 s);",
            )
            direct = self.granted and self.base <= address and address + 4 <= self.base + self.size
            write = gp.get_command() == tlm.TLM_WRITE_COMMAND
            description = (
                f" correct address and appropriate access for a GP {'Write' if write else 'Read'} Command \n      "
                if direct
                else " address is not a dmi address"
            )
            report("dmi_memory.cpp", "is_address_dmi", f"Initiator:{self.identifier}{description}")
            if direct:
                pointer = self.properties.get_dmi_ptr()
                data = gp.get_data_ptr()
                offset = address - self.base
                for i in range(4):
                    if write:
                        pointer[offset + i] = data[i]
                    else:
                        data[i] = pointer[offset + i]
                gp.set_response_status(tlm.TLM_OK_RESPONSE)
                delay = self.properties.get_write_latency() if write else self.properties.get_read_latency()
                value = int.from_bytes(bytes(data[i] for i in range(4)), "little")
                report(
                    "dmi_memory.cpp",
                    "print",
                    f"ID: {self.identifier} COMMAND: {'WRITE' if write else 'READ'} Length: 04\n      Addr: 0x{address:016X} Data: 0x{value:08X}",
                )
                report(
                    "lt_dmi_initiator.cpp",
                    "initiator_thread",
                    f"Initiator: {self.identifier} dmi based transaction returned a delay of {delay}",
                )
                self.direct_count += 1
                sc.wait(delay)
            else:
                gp.set_dmi_allowed(False)
                self.socket.get_interface().b_transport(gp, delay)
                assert gp.get_response_status() == tlm.TLM_OK_RESPONSE
                report(
                    "lt_dmi_initiator.cpp",
                    "initiator_thread",
                    f"Initiator: {self.identifier} b_transport returned a delay of {delay}",
                )
                sc.wait(delay)
                report(
                    "lt_dmi_initiator.cpp",
                    "initiator_thread",
                    f"Initiator: {self.identifier} gp_tpr was returned with dmi_allowed set to {int(gp.is_dmi_allowed())}",
                )
                if gp.is_dmi_allowed():
                    gp.set_address(address)
                    self.properties.init()
                    assert self.socket.get_interface().get_direct_mem_ptr(gp, self.properties)
                    self.base = int(self.properties.get_start_address())
                    self.size = int(self.properties.get_end_address()) - self.base
                    self.granted = True
            report("lt_dmi_initiator.cpp", "initiator_thread", f"Initiator: {self.identifier} End of processing")
            self.complete(gp)


class Top(sc.sc_module):
    """Retain source DMI topology and the1000000 ns simulation limiter."""

    def __init__(self, name):
        """Bind native memory pointers and two independent initiator caches."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.bus = Bus("m_bus")
        self.targets = [Target(f"target_{i}", 201 + i) for i in range(2)]
        self.initiators = [Initiator(f"initiator_{i}", 101 + i) for i in range(2)]
        for i in range(2):
            self.initiators[i].socket.bind(self.bus.targets[i])
            self.bus.initiators[i].bind(self.targets[i].socket)
        self.sc_thread("limit_thread", self.limit)

    def limit(self):
        """Stop at source time after target toggles and all traffic finish."""
        sc.wait(sc.SC_ZERO_TIME)
        sc.wait(ns(1000000))
        sc.sc_stop()


def run():
    """Run and assert native direct accesses, readback and terminal time."""
    top = Top("top")
    sc.sc_start()
    assert all(i.done and i.direct_count > 0 for i in top.initiators)
    assert sc.sc_time_stamp() == ns(1000000)
