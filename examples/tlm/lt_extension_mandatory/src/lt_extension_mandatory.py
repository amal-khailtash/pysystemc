"""Accellera mandatory extension/DMI port (Apache-2.0 LICENSE/NOTICE).

Native extension/data storage stays retained through invalidation callbacks.
Independent runs need fresh interpreters; this reproduces source timing.
"""

import sys
from pathlib import Path
from array import array

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from models import ns
from runtime import report
from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy


class Top(sc.sc_module):
    """Own native extension, payload, DMI memory and source socket callbacks."""

    def __init__(self, name):
        """Register native extension before payload construction and bind sockets."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        common = Path(__file__).resolve().parents[4] / "submodules/systemc/examples/tlm/common"
        cppyy.add_include_path(str(common / "include"))
        cppyy.cppdef((common / "src/extension_initiator_id.cpp").read_text())
        self.extension = cppyy.gbl.extension_initiator_id()
        self.extension.m_initiator_id = "generic ID"
        self.payload = tlm.tlm_generic_payload()
        self.payload.resize_extensions()
        self.payload.set_extension[cppyy.gbl.extension_initiator_id](self.extension)
        self.data = array("B", [0] * 4)
        self.memory = array("B", [0] * 400)
        self.payload.set_data_ptr(self.data)
        self.payload.set_data_length(4)
        self.payload.set_streaming_width(4)
        self.properties = tlm.tlm_dmi()
        self.properties.set_start_address(1)
        self.properties.set_end_address(0)
        self.timeout = sc.sc_event("invalidate_dmi_event")
        self.target = tlm_utils.callback_target_socket("target")
        self.target.register_nb_transport_fw(self.transport)
        self.target.register_get_direct_mem_ptr(self.dmi)
        self.initiator = tlm_utils.callback_initiator_socket("initiator")
        self.initiator.register_invalidate_direct_mem_ptr(self.invalidate)
        self.initiator.bind(self.target)
        self.sc_method("invalidate_dmi_method", self.expire)
        self.sensitive(self.timeout)
        self.dont_initialize()
        self.sc_thread("initiator_thread", self.run)
        self.reads = []

    def extension_check(self, gp, function):
        """Require native initiator-ID extension on transport and DMI access."""
        extension = gp.get_extension[cppyy.gbl.extension_initiator_id]()
        assert extension and str(extension.m_initiator_id) == "generic ID"
        report("lt_target_extension_mandatory.cpp", function, f"Extension present, Data: {extension.m_initiator_id}")

    def transport(self, gp, phase, delay):
        """Perform checked memory transfer with source 10/100 ns annotations."""
        assert phase == tlm.BEGIN_REQ
        self.extension_check(gp, "nb_transport_fw")
        address = int(gp.get_address())
        write = gp.get_command() == tlm.TLM_WRITE_COMMAND
        pointer = gp.get_data_ptr()
        if write:
            for i in range(4):
                self.memory[address + i] = pointer[i]
        else:
            for i in range(4):
                pointer[i] = self.memory[address + i]
        report(
            "lt_target_extension_mandatory.cpp",
            "nb_transport_fw",
            f"{'Write' if write else 'Read'} request - Addr: 0x{address:016X}",
        )
        delay.__assign__(delay + ns(10 if write else 100))
        gp.set_response_status(tlm.TLM_OK_RESPONSE)
        gp.set_dmi_allowed(True)
        return tlm.TLM_COMPLETED

    def dmi(self, gp, result):
        """Grant native read/write pointer and schedule source 25 ns invalidation."""
        self.timeout.notify(ns(25))
        self.extension_check(gp, "get_dmi_ptr")
        result.allow_read_write()
        result.set_start_address(0)
        result.set_end_address(399)
        result.set_dmi_ptr(self.memory)
        result.set_read_latency(ns(100))
        result.set_write_latency(ns(10))
        return True

    def expire(self):
        """Deliver native backward invalidation after the source timeout."""
        self.target.get_base_port().get_interface().invalidate_direct_mem_ptr(0, 399)

    def invalidate(self, first, last):
        """Invalidate overlapping region and print source lifecycle evidence."""
        valid = first <= self.properties.get_end_address() and last >= self.properties.get_start_address()
        if valid:
            self.properties.set_start_address(1)
            self.properties.set_end_address(0)
        report(
            "lt_initiator_extension_mandatory.cpp",
            "invalidate_direct_mem_ptr",
            "Received invalidate request - " + ("valid request" if valid else "not in our DMI address range"),
        )

    def run(self):
        """Write/read five native words and exercise pointer expiry during waits."""
        gp = self.payload
        for command in (tlm.TLM_WRITE_COMMAND, tlm.TLM_READ_COMMAND):
            for index in range(5):
                address = 4 * index
                write = command == tlm.TLM_WRITE_COMMAND
                self.data[:] = array("B", (index if write else 0).to_bytes(4, "little"))
                gp.set_address(address)
                gp.set_command(command)
                gp.set_dmi_allowed(False)
                message = f"Creating {'write' if write else 'read'} transaction - Addr: 0x{address:016X}"
                if write:
                    message += f" Data: 0x{index:08X}"
                report("lt_initiator_extension_mandatory.cpp", "log_start", message)
                direct = self.properties.get_start_address() <= address <= self.properties.get_end_address()
                if direct:
                    pointer = self.properties.get_dmi_ptr()
                    for i in range(4):
                        if write:
                            pointer[address + i] = self.data[i]
                        else:
                            self.data[i] = pointer[address + i]
                    gp.set_response_status(tlm.TLM_OK_RESPONSE)
                    sc.wait(self.properties.get_write_latency() if write else self.properties.get_read_latency())
                else:
                    phase, delay = tlm.tlm_phase(tlm.BEGIN_REQ), ns(0)
                    assert self.initiator.get_interface().nb_transport_fw(gp, phase, delay) == tlm.TLM_COMPLETED
                    sc.wait(delay)
                assert gp.get_response_status() == tlm.TLM_OK_RESPONSE
                value = int.from_bytes(bytes(self.data), "little")
                if not write:
                    assert value == index
                    self.reads.append(value)
                report(
                    "lt_initiator_extension_mandatory.cpp",
                    "log_end",
                    "Received TLM_OK_RESPONSE" + (f", Data: 0x{value:08X}" if not write else ""),
                )
                if not direct and gp.is_dmi_allowed():
                    gp.set_write()
                    assert self.initiator.get_interface().get_direct_mem_ptr(gp, self.properties)


def run():
    """Run source transaction/DMI lifecycle and verify all five readbacks."""
    top = Top("top")
    sc.sc_start()
    assert top.reads == list(range(5))
    top.payload.clear_extension[cppyy.gbl.extension_initiator_id]()
    assert sc.sc_time_stamp() == ns(550)


if __name__ == "__main__":
    run()
