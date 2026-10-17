"""Verify five TLM bool callback contracts without implicit truthiness conversion."""

from amal.eda.systemc import sc_core as sc, tlm


class Owner:
    """Supply configurable booleans to native structural TLM interfaces."""

    def __init__(self):
        """Initialize a valid readiness/permission result."""
        self.result = False
        self.calls = 0

    def __getattr__(self, name):
        """Return a callable for each required native callback."""

        def callback(*args):
            """Record dispatch and return owner state."""
            self.calls += 1
            return self.result

        return callback


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for interface in (
    tlm.tlm_nonblocking_put_if[int],
    tlm.tlm_nonblocking_get_if[int],
    tlm.tlm_nonblocking_peek_if[int],
    tlm.tlm_fw_direct_mem_if[tlm.tlm_generic_payload],
):
    owner = Owner()
    port = sc.sc_port[interface]()
    port(owner)
    entries.append((owner, port))
del name
put, get, peek, dmi_port = [(owner, port.get_interface()) for owner, port in entries]
payload = tlm.tlm_generic_payload()
dmi = tlm.tlm_dmi()
payload.set_address(73)
dmi.set_start_address(17)
checks = (
    (put[0], lambda: put[1].nb_put(29)),
    (put[0], put[1].nb_can_put),
    (get[0], get[1].nb_can_get),
    (peek[0], peek[1].nb_can_peek),
    (dmi_port[0], lambda: dmi_port[1].get_direct_mem_ptr(payload, dmi)),
)
for owner, invoke in checks:
    for result in (None, 0, 1, "yes", [], object()):
        owner.result = result
        before = owner.calls
        try:
            invoke()
        except TypeError as error:
            assert "must return bool" in str(error)
        else:
            raise AssertionError(f"invalid TLM bool accepted: {result!r}")
        assert owner.calls == before + 1
        assert payload.get_address() == 73 and dmi.get_start_address() == 17
    for result in (False, True, False):
        owner.result = result
        assert invoke() == result
sc.sc_start(sc.SC_ZERO_TIME)
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_stop()
print("TLM_BOOLEAN_CONTRACTS_OK")
