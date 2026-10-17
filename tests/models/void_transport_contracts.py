"""Verify void event queue, TLM transport/invalidation and debug results."""

from amal.eda.systemc import sc_core as sc, tlm


class Owner:
    """Record callback arguments without mutating borrowed native state."""

    def __init__(self):
        """Initialize callback result, records and live event storage."""
        self.result = None
        self.calls = []
        self.changed = sc.sc_event()

    def __getattr__(self, method):
        """Supply ordinary structural callbacks and their native result contracts."""

        def callback(*args):
            """Record native arguments and return the configured void result."""
            self.calls.append((method, args))
            if method == "default_event":
                return self.changed
            if method in ("used", "size", "transport_dbg"):
                return 0
            if method in ("nb_peek",):
                return False, 0
            if method in ("nb_poke", "get_direct_mem_ptr"):
                return False
            if method in ("nb_transport_fw", "nb_transport_bw"):
                return tlm.TLM_COMPLETED
            return self.result

        return callback


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for interface in (
    sc.sc_event_queue_if,
    tlm.tlm_blocking_transport_if[tlm.tlm_generic_payload],
    tlm.tlm_fw_transport_if[tlm.tlm_base_protocol_types],
    tlm.tlm_bw_direct_mem_if,
    tlm.tlm_bw_transport_if[tlm.tlm_base_protocol_types],
    tlm.tlm_fifo_debug_if[int],
):
    owner = Owner()
    port = sc.sc_port[interface]()
    port(owner)
    entries.append((owner, port.get_interface()))
del name
payload = tlm.tlm_generic_payload()
payload.set_address(73)
delay = sc.sc_time(2, sc.SC_NS)
checks = []
queue_owner, queue = entries[0]
checks.extend(
    (
        (queue_owner, "notify", lambda: queue.notify(sc.sc_time(3, sc.SC_NS))),
        (queue_owner, "notify", lambda: queue.notify(3, sc.SC_NS)),
        (queue_owner, "cancel_all", queue.cancel_all),
    )
)
for owner, native in entries[1:3]:
    checks.append((owner, "b_transport", lambda native=native: native.b_transport(payload, delay)))
for owner, native in entries[3:5]:
    checks.append((owner, "invalidate_direct_mem_ptr", lambda native=native: native.invalidate_direct_mem_ptr(17, 73)))
owner, native = entries[5]
checks.append((owner, "debug", native.debug))
for owner, method, invoke in checks:
    for result in (True, False, 0, 1, "status", object()):
        owner.result = result
        before = len(owner.calls)
        try:
            invoke()
        except TypeError as error:
            assert f"{method} must return None" in str(error)
        else:
            raise AssertionError(f"void callback accepted {result!r}")
        assert len(owner.calls) == before + 1
        assert payload.get_address() == 73 and delay == sc.sc_time(2, sc.SC_NS)
    owner.result = None
    assert invoke() is None
assert queue_owner.calls[-1][0] == "cancel_all"
assert entries[3][0].calls[-1][1] == (17, 73)
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("VOID_TRANSPORT_CONTRACTS_OK")
