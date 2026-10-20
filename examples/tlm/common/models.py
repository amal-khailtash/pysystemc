"""Accellera approximately timed example components in Python (Apache-2.0).

Native generic payloads and sockets retain protocol semantics. Owners keep data
buffers and callbacks alive through all response timing points; run in subprocesses.
"""

from array import array
from pathlib import Path
from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy
from runtime import Queue, report


def ns(value):
    """Return a native nanosecond duration."""
    return sc.sc_time(value, sc.SC_NS)


def phase_name(phase):
    """Return the upstream textual phase name."""
    return str(phase)


def status_name(status):
    """Return the source synchronization enum text."""
    return {int(tlm.TLM_ACCEPTED): "ACCEPTED", int(tlm.TLM_UPDATED): "UPDATED", int(tlm.TLM_COMPLETED): "COMPLETED"}[
        int(status)
    ]


class Bus(sc.sc_module):
    """Route native payloads through source request/response exclusion rules."""

    def __init__(self, name):
        """Create two upstream/downstream sockets and retained pending maps."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.requests = Queue("requests")
        self.responses = Queue("responses")
        self.end_request = sc.sc_event("end_request")
        self.end_response = sc.sc_event("end_response")
        self.pending = {}
        self.targets = []
        self.initiators = []
        for index in range(2):
            target = tlm_utils.callback_target_socket(f"target_{index}")
            target.register_nb_transport_fw(lambda gp, phase, delay, i=index: self.forward(i, gp, phase, delay))
            self.targets.append(target)
            initiator = tlm_utils.callback_initiator_socket(f"initiator_{index}")
            initiator.register_nb_transport_bw(self.backward)
            self.initiators.append(initiator)
        self.sc_thread("RequestThread", self.request_thread)
        self.sc_thread("ResponseThread", self.response_thread)

    def key(self, gp):
        """Identify the borrowed native payload independently of Python proxies."""
        return cppyy.addressof(gp)

    def forward(self, index, gp, phase, delay):
        """Retain BEGIN_REQ or notify the end-response exclusion event."""
        if phase == tlm.BEGIN_REQ:
            key = self.key(gp)
            assert key not in self.pending
            self.pending[key] = [index, None, gp]
            self.requests.notify(gp, delay)
            return tlm.TLM_ACCEPTED
        assert phase == tlm.END_RESP
        self.end_response.notify(delay)
        return tlm.TLM_COMPLETED

    def backward(self, gp, phase, delay):
        """Queue target responses and release the serialized request path."""
        assert phase in (tlm.END_REQ, tlm.BEGIN_RESP)
        self.end_request.notify(delay)
        if phase == tlm.BEGIN_RESP:
            self.responses.notify(gp, delay)
        return tlm.TLM_ACCEPTED

    def request_thread(self):
        """Decode addresses and honor each target's request timing point."""
        while True:
            sc.wait(self.requests.event)
            while (gp := self.requests.pop()) is not None:
                destination = int(gp.get_address()) >> 28
                assert destination < 2
                gp.set_address(int(gp.get_address()) & 0xFFFFFFF)
                info = self.pending[self.key(gp)]
                info[1] = destination
                phase, delay = tlm.tlm_phase(tlm.BEGIN_REQ), ns(0)
                status = self.initiators[destination].get_interface().nb_transport_fw(gp, phase, delay)
                if status == tlm.TLM_COMPLETED:
                    self.responses.notify(gp, delay)
                    info[1] = None
                    sc.wait(delay)
                else:
                    assert status in (tlm.TLM_ACCEPTED, tlm.TLM_UPDATED)
                    if phase == tlm.BEGIN_REQ:
                        sc.wait(self.end_request)
                    elif phase == tlm.END_REQ:
                        sc.wait(delay)
                    elif phase == tlm.BEGIN_RESP:
                        self.responses.notify(gp, delay)
                        continue
                    else:
                        raise RuntimeError("illegal target phase")
                    if info[0] is not None:
                        self.targets[info[0]].get_base_port().get_interface().nb_transport_bw(
                            gp, tlm.tlm_phase(tlm.END_REQ), ns(0)
                        )

    def response_thread(self):
        """Serialize responses and forward END_RESP to the retained destination."""
        while True:
            sc.wait(self.responses.event)
            while (gp := self.responses.pop()) is not None:
                info = self.pending[self.key(gp)]
                origin = info[0]
                info[0] = None
                phase, delay = tlm.tlm_phase(tlm.BEGIN_RESP), ns(0)
                status = self.targets[origin].get_base_port().get_interface().nb_transport_bw(gp, phase, delay)
                if status == tlm.TLM_COMPLETED:
                    sc.wait(delay)
                else:
                    assert status in (tlm.TLM_ACCEPTED, tlm.TLM_UPDATED)
                    sc.wait(self.end_response)
                if info[1] is not None:
                    result = (
                        self.initiators[info[1]].get_interface().nb_transport_fw(gp, tlm.tlm_phase(tlm.END_RESP), ns(0))
                    )
                    assert result == tlm.TLM_COMPLETED
                del self.pending[self.key(gp)]


class Target(sc.sc_module):
    """Implement source one/two/four-phase target timing and memory operations."""

    def __init__(self, name, identifier, style, optional=False):
        """Create memory and callback socket; retain response scheduling state."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.identifier, self.style, self.optional = identifier, style, optional
        self.memory = bytearray(4096)
        self.count = 1
        self.requests = Queue("end_requests")
        self.responses = Queue("responses")
        self.end_response = sc.sc_event("end_response")
        self.socket = tlm_utils.callback_target_socket("memory_socket_1")
        self.socket.register_nb_transport_fw(self.forward)
        self.sc_method("end_request_method", self.end_requests)
        self.sensitive(self.requests.event)
        self.dont_initialize()
        self.sc_method("begin_response_method", self.begin_responses)
        self.sensitive(self.responses.event)
        self.dont_initialize()

    def log(self, function, message):
        """Print source component reports at the current native time."""
        report(f"at_target_{self.style}_phase.cpp", function, f"Target: {self.identifier} {message}")

    def latency(self, gp):
        """Return the source read/write memory latency."""
        return ns(30 if gp.get_command() == tlm.TLM_WRITE_COMMAND else 50)

    def operation(self, gp):
        """Copy native payload bytes, validate attributes and print memory records."""
        address, length = int(gp.get_address()), int(gp.get_data_length())
        assert 0 <= address < 4096 and address + length <= 4096
        assert not gp.get_byte_enable_ptr() and int(gp.get_streaming_width()) >= length
        data = gp.get_data_ptr()
        if gp.get_command() == tlm.TLM_WRITE_COMMAND:
            self.memory[address : address + length] = bytes(data[i] for i in range(length))
            command = "WRITE"
        else:
            assert gp.get_command() == tlm.TLM_READ_COMMAND
            for i in range(length):
                data[i] = self.memory[address + i]
            command = "READ"
        value = int.from_bytes(bytes(data[i] for i in range(length)), "little")
        report(
            "memory.cpp",
            "print",
            f"ID: {self.identifier} COMMAND: {command} Length: {length:02d}\n      Addr: 0x{address:016X} Data: 0x{value:08X}",
        )
        gp.set_response_status(tlm.TLM_OK_RESPONSE)

    def forward(self, gp, phase, delay):
        """Preserve annotated completion and every twentieth forced response."""
        if self.optional and self.style == 4:
            extension = gp.get_extension[cppyy.gbl.extension_initiator_id]()
            if extension:
                self.log("nb_transport_fw", f"extension data: {extension.m_initiator_id}")
        if phase == tlm.END_RESP:
            self.end_response.notify(delay if self.style == 4 else sc.SC_ZERO_TIME)
            return tlm.TLM_COMPLETED
        assert phase == tlm.BEGIN_REQ
        before = f"nb_transport_fw (GP, {phase_name(phase)}, {delay})"
        if self.style == 4:
            self.requests.notify(gp, delay + ns(10))
            self.log(
                "nb_transport_fw",
                before + f"\n      Target: {self.identifier} ACCEPTED (GP, {phase_name(phase)}, {delay})",
            )
            return tlm.TLM_ACCEPTED
        if self.style == 1 and self.count % 20:
            self.count += 1
            self.log("nb_transport_fw", before)
            self.operation(gp)
            delay.__assign__(delay + self.latency(gp) + ns(10))
            self.log("nb_transport_fw", f"COMPLETED (GP, {phase_name(phase)}, {delay})")
            return tlm.TLM_COMPLETED
        self.count += 1
        self.responses.notify(gp, self.latency(gp) + ns(10))
        delay.__assign__(ns(10))
        phase.__assign__(tlm.END_REQ)
        detail = f"\n      Target: {self.identifier} transaction moved to send-response PEQ " if self.style == 2 else ""
        self.log(
            "nb_transport_fw", before + detail + f"\n      Target: {self.identifier} UPDATED (GP, END_REQ, {delay})"
        )
        return tlm.TLM_UPDATED

    def end_requests(self):
        """Advance four-phase requests and schedule memory response latency."""
        while (gp := self.requests.pop()) is not None:
            self.responses.notify(gp, self.latency(gp))
            self.log(
                "end_request_method",
                f"starting end-request method\n      Target: {self.identifier} transaction moved to send-response PEQ \n      Target: {self.identifier} nb_transport_bw (GP, END_REQ, 0 s)",
            )
            status = self.socket.get_base_port().get_interface().nb_transport_bw(gp, tlm.tlm_phase(tlm.END_REQ), ns(0))
            assert status == tlm.TLM_ACCEPTED
            self.log("end_request_method", "ACCEPTED (GP, END_REQ, 0 s)")

    def begin_responses(self):
        """Perform memory access and obey the source response exclusion rule."""
        sc.next_trigger(self.responses.event)
        while (gp := self.responses.pop()) is not None:
            self.log("begin_response_method", "starting response method")
            self.operation(gp)
            phase, delay = tlm.tlm_phase(tlm.BEGIN_RESP), ns(0)
            self.log("begin_response_method", "nb_transport_bw (GP, BEGIN_RESP, SC_ZERO_TIME)")
            status = self.socket.get_base_port().get_interface().nb_transport_bw(gp, phase, delay)
            self.log("begin_response_method", f"{status_name(status)} (GP, {phase_name(phase)}, {delay})")
            if self.style in (1, 2, 4):
                sc.next_trigger(delay if status == tlm.TLM_COMPLETED else self.end_response)
                return


class Initiator(sc.sc_module):
    """Generate source memory traffic and track native response timing points."""

    def __init__(self, name, identifier, optional=False):
        """Create socket, two payload/data owners and retained completion events."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.identifier = identifier
        self.optional = optional
        self.extensions = []
        if optional:
            common = Path(__file__).resolve().parents[3] / "submodules/systemc/examples/tlm/common"
            cppyy.add_include_path(str(common / "include"))
            if not hasattr(cppyy.gbl, "extension_initiator_id"):
                cppyy.cppdef((common / "src/extension_initiator_id.cpp").read_text())
        self.socket = tlm_utils.callback_initiator_socket("initiator_socket")
        self.socket.register_nb_transport_bw(self.backward)
        self.next_request = sc.sc_event("next_request")
        self.completed = sc.sc_event("completed")
        self.responses = Queue("send_end_rsp_PEQ")
        self.waiting = {}
        self.done = False
        self.payloads = [tlm.tlm_generic_payload() for _ in range(2)]
        self.buffers = [array("B", [0] * 4) for _ in range(2)]
        for gp, data in zip(self.payloads, self.buffers):
            gp.set_data_ptr(data)
            gp.set_data_length(4)
            gp.set_streaming_width(4)
            if optional:
                extension = cppyy.gbl.extension_initiator_id()
                extension.m_initiator_id = f"'Initiator ID: {identifier}'"
                gp.resize_extensions()
                gp.set_extension[cppyy.gbl.extension_initiator_id](extension)
                self.extensions.append(extension)
        self.request_queue = []
        self.request_event = sc.sc_event("request_event")
        self.completion_queue = []
        self.sc_thread("initiator_thread", self.initiate)
        self.sc_method("send_end_rsp_method", self.end_responses)
        self.sensitive(self.responses.event)
        self.dont_initialize()
        self.sc_thread("traffic_generator_thread", self.traffic)

    def log(self, function, message):
        """Print source initiator state transitions."""
        report("select_initiator.cpp", function, f"Initiator: {self.identifier} {message}")

    def traffic(self):
        """Write/read both source address windows and assert every returned word."""
        report("traffic_generator.cpp", "traffic_generator_thread", f"Initiator: {self.identifier} Starting Traffic")
        serial = 0
        for window in range(2):
            for command in (tlm.TLM_WRITE_COMMAND, tlm.TLM_READ_COMMAND):
                for offset in range(16):
                    gp, data = self.payloads[serial % 2], self.buffers[serial % 2]
                    serial += 1
                    address = ((self.identifier - 100) << 8) + offset * 4 + (window << 28)
                    value = address if window == 0 else (~address & 0xFFFFFFFF)
                    for i, byte in enumerate(value.to_bytes(4, "little")):
                        data[i] = byte
                    gp.set_address(address)
                    gp.set_command(command)
                    gp.set_response_status(tlm.TLM_INCOMPLETE_RESPONSE)
                    self.request_queue.append(gp)
                    self.request_event.notify()
                    while not self.completion_queue:
                        sc.wait(self.completed)
                    returned = self.completion_queue.pop(0)
                    assert cppyy.addressof(returned) == cppyy.addressof(gp)
                    assert gp.get_response_status() == tlm.TLM_OK_RESPONSE
                    if command == tlm.TLM_READ_COMMAND:
                        observed = int.from_bytes(bytes(data), "little") & 0xFFFFFFF
                        expected = int(gp.get_address()) & 0xFFFFFFF
                        assert observed in (expected, ~expected & 0xFFFFFFF)
        self.done = True
        report(
            "traffic_generator.cpp",
            "traffic_generator_thread",
            f"Traffic Generator : {self.identifier}\n=========================================================\n            ####  Traffic Generator Complete  #### ",
        )

    def initiate(self):
        """Send native BEGIN_REQ calls and preserve source tracking state."""
        while True:
            while not self.request_queue:
                sc.wait(self.request_event)
            gp = self.request_queue.pop(0)
            phase, delay = tlm.tlm_phase(tlm.BEGIN_REQ), ns(0)
            self.log(
                "initiator_thread",
                f"starting new transaction for Addr:0x{int(gp.get_address()):08X}\n      Initiator: {self.identifier} nb_transport_fw (GP, BEGIN_REQ, 0 s)",
            )
            status = self.socket.get_interface().nb_transport_fw(gp, phase, delay)
            prefix = f"{status_name(status)} (GP, {phase_name(phase)}, {delay})\n"
            key = cppyy.addressof(gp)
            if status == tlm.TLM_COMPLETED:
                sc.wait(delay + ns(7))
                self.log(
                    "initiator_thread",
                    prefix + f"      Initiator: {self.identifier} target returned COMPLETED with annotated time ",
                )
                self.complete(gp)
            elif status == tlm.TLM_UPDATED:
                assert phase == tlm.END_REQ
                self.waiting[key] = "UPDATED"
                sc.wait(delay)
                self.log(
                    "initiator_thread",
                    prefix + f"      Initiator: {self.identifier} transaction waiting begin-response on backward path",
                )
            else:
                assert status == tlm.TLM_ACCEPTED
                self.waiting[key] = "ACCEPTED"
                self.log(
                    "initiator_thread",
                    prefix + f"      Initiator: {self.identifier} transaction waiting end-request on backward-path ",
                )
                sc.wait(self.next_request)

    def complete(self, gp):
        """Return the retained transaction to traffic on the next delta cycle."""
        self.completion_queue.append(gp)
        self.completed.notify(sc.SC_ZERO_TIME)

    def backward(self, gp, phase, delay):
        """Validate backward phases and schedule the source 7 ns END_RESP."""
        key = cppyy.addressof(gp)
        assert key in self.waiting
        state = self.waiting[key]
        prefix = f"nb_transport_bw (GP, {phase_name(phase)}, {delay})from Addr:0x{int(gp.get_address()):08X}\n"
        if phase == tlm.END_REQ:
            assert state == "ACCEPTED"
            self.waiting[key] = "END_REQ"
            self.next_request.notify(sc.SC_ZERO_TIME)
            self.log(
                "nb_transport_bw",
                prefix
                + f"      Initiator: {self.identifier} transaction waiting begin-response on backward path\n      Initiator: {self.identifier} ACCEPTED (GP, END_REQ, {delay})",
            )
            return tlm.TLM_ACCEPTED
        assert phase == tlm.BEGIN_RESP
        del self.waiting[key]
        if state == "UPDATED":
            phase.__assign__(tlm.END_RESP)
            delay.__assign__(ns(7))
            self.next_request.notify(sc.SC_ZERO_TIME)
            self.complete(gp)
            self.log(
                "nb_transport_bw",
                prefix
                + f"      Initiator: {self.identifier} Target response to begin-request was UPDATED\n      Initiator: {self.identifier} indicating 2 phase return COMPLETED and annotate time\n      Initiator: {self.identifier} COMPLETED (GP, END_RESP, 7 ns)",
            )
            return tlm.TLM_COMPLETED
        self.responses.notify(gp, ns(7))
        if state == "ACCEPTED":
            self.next_request.notify(sc.SC_ZERO_TIME)
            detail = f"      Initiator: {self.identifier} target omitted end-request timing-point returning ACCEPTED "
        else:
            detail = f"      Initiator: {self.identifier} transaction moved to send-end-response PEQ \n      Initiator: {self.identifier} ACCEPTED (GP, BEGIN_RESP, {delay})"
        self.log("nb_transport_bw", prefix + detail)
        return tlm.TLM_ACCEPTED

    def end_responses(self):
        """Send END_RESP and return each payload after native completion."""
        while (gp := self.responses.pop()) is not None:
            self.log(
                "send_end_rsp_method",
                f"starting send-end-response method\n      Initiator: {self.identifier} nb_transport_fw (GP, END_RESP, 0 s)",
            )
            result = self.socket.get_interface().nb_transport_fw(gp, tlm.tlm_phase(tlm.END_RESP), ns(0))
            assert result == tlm.TLM_COMPLETED
            self.complete(gp)
            self.log("send_end_rsp_method", "COMPLETED (GP, END_RESP, 0 s)")


def run(styles=(1, 1), optional=False):
    """Build two initiators, two targets and the source AT bus and run to starvation.

    Args:
        styles: Target phase counts, each one of 1, 2 or 4.
        optional: Whether optional initiator-ID extensions are enabled.
    Returns:
        None after both traffic generators verify all 128 transactions.
    """
    module_name = sc.sc_module_name("top")
    top = sc.sc_module(module_name)
    bus = Bus("m_bus")
    targets = [Target(f"m_at_target_{style}_phase_{i + 1}", 201 + i, style, optional) for i, style in enumerate(styles)]
    initiators = [Initiator(f"m_initiator_{i + 1}", 101 + i, optional) for i in range(2)]
    for i in range(2):
        initiators[i].socket.bind(bus.targets[i])
        bus.initiators[i].bind(targets[i].socket)
    del module_name
    sc.sc_start()
    assert all(initiator.done for initiator in initiators)
    assert not bus.pending
    assert top is not None
