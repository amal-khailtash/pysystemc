"""Validate structural TLM enum/count results and absent failed get/peek values."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm


class Owner:
    """Supply configured results without mutating borrowed native arguments."""

    def __init__(self):
        """Initialize callback result and invocation count."""
        self.result = None
        self.calls = 0

    def __getattr__(self, method):
        """Provide all required structural callbacks."""

        def callback(*args):
            """Record dispatch and return the selected contract result."""
            self.calls += 1
            if method in ("nb_can_get", "nb_can_peek"):
                return False
            return self.result

        return callback


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for interface in (
    tlm.tlm_fw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase],
    tlm.tlm_bw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase],
    tlm.tlm_fw_transport_if[tlm.tlm_base_protocol_types],
    tlm.tlm_bw_transport_if[tlm.tlm_base_protocol_types],
    tlm.tlm_transport_dbg_if[tlm.tlm_generic_payload],
):
    owner = Owner()
    port = sc.sc_port[interface]()
    port(owner)
    entries.append((owner, port))
value_ports = []
for family in (
    tlm.tlm_nonblocking_get_if,
    tlm.tlm_nonblocking_peek_if,
    tlm.tlm_nonblocking_get_peek_if,
    tlm.tlm_get_if,
    tlm.tlm_peek_if,
    tlm.tlm_get_peek_if,
):
    owner = Owner()
    port = sc.sc_port[family[sc_dt.sc_biguint[137]]]()
    port(owner)
    value_ports.append((owner, port))
del name
payload = tlm.tlm_generic_payload()
payload.set_address(73)
payload.set_data_length(8)
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(2, sc.SC_NS)
for owner, port in entries:
    native = port.get_interface()
    for method in ("nb_transport_fw", "nb_transport_bw", "transport_dbg"):
        if not hasattr(native, method):
            continue
        invoke = getattr(native, method)
        args = (payload,) if method == "transport_dbg" else (payload, phase, delay)
        invalid = (None, True, 1.5, -1, 9, 2**32) if method == "transport_dbg" else (None, True, 0, 7)
        valid = (0, 1, 8) if method == "transport_dbg" else (tlm.TLM_ACCEPTED, tlm.TLM_UPDATED, tlm.TLM_COMPLETED)
        for result in invalid:
            owner.result = result
            try:
                invoke(*args)
            except TypeError, ValueError:
                pass
            else:
                raise AssertionError(f"invalid {method}: {result!r}")
        for result in valid:
            owner.result = result
            assert invoke(*args) == result
        assert payload.get_data_length() == 8
        assert payload.get_address() == 73 and phase == tlm.BEGIN_REQ and delay == sc.sc_time(2, sc.SC_NS)
for owner, port in value_ports:
    native = port.get_interface()
    for method in ("nb_get", "nb_peek"):
        if not hasattr(native, method):
            continue
        value = sc_dt.sc_biguint[137](73)
        value[136] = True
        expected = value.to_string()
        owner.result = (False, None)
        assert not getattr(native, method)(value)
        assert value.to_string() == expected
        owner.result = (True, sc_dt.sc_biguint[137](17))
        assert getattr(native, method)(value) and value.to_uint() == 17
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("TLM_RESULT_COMPLETION_OK")
