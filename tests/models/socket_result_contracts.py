"""Verify convenience socket return contracts through native forward/backward calls."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils


class Results:
    """Retain independently configurable callback results and native IDs."""

    def __init__(self):
        """Initialize the callback result and observed argument counts."""
        self.result = None
        self.arguments = []

    def __call__(self, *args):
        """Record dispatch without mutating borrowed native objects."""
        self.arguments.append(args)
        return self.result


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for index, (factory, tagged) in enumerate(
    (
        (tlm_utils.callback_target_socket, False),
        (tlm_utils.callback_target_socket, True),
        (tlm_utils.callback_passthrough_target_socket, False),
        (tlm_utils.callback_passthrough_target_socket, True),
    )
):
    target = factory(f"target_{index}", tagged=tagged)
    initiator = tlm_utils.callback_initiator_socket(f"initiator_{index}", tagged=tagged)
    owners = {}
    for socket, methods in (
        (
            target,
            (
                "register_b_transport",
                "register_get_direct_mem_ptr",
                "register_transport_dbg",
                "register_nb_transport_fw",
            ),
        ),
        (initiator, ("register_invalidate_direct_mem_ptr", "register_nb_transport_bw")),
    ):
        for method in methods:
            owner = Results()
            getattr(socket, method)(owner, 17) if tagged else getattr(socket, method)(owner)
            owners[method] = owner
    initiator.bind(target)
    entries.append((target, initiator, owners, tagged))
del name
payload = tlm.tlm_generic_payload()
payload.set_address(73)
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(2, sc.SC_NS)
dmi = tlm.tlm_dmi()
for target, initiator, owners, tagged in entries:
    forward = initiator.get_interface()
    backward = target.get_base_port().get_interface()
    checks = (
        ("register_b_transport", lambda: forward.b_transport(payload, delay), (True, 0, "x"), (None,)),
        ("register_invalidate_direct_mem_ptr", lambda: backward.invalidate_direct_mem_ptr(17, 73), (True, 0), (None,)),
        ("register_get_direct_mem_ptr", lambda: forward.get_direct_mem_ptr(payload, dmi), (None, 0, 1), (False, True)),
        ("register_transport_dbg", lambda: forward.transport_dbg(payload), (True, 1.5, -1, 2**32), (0, 2**32 - 1)),
        (
            "register_nb_transport_fw",
            lambda: forward.nb_transport_fw(payload, phase, delay),
            (None, True, 0, 7),
            (tlm.TLM_ACCEPTED, tlm.TLM_UPDATED, tlm.TLM_COMPLETED),
        ),
        (
            "register_nb_transport_bw",
            lambda: backward.nb_transport_bw(payload, phase, delay),
            (None, True, 0, 7),
            (tlm.TLM_ACCEPTED, tlm.TLM_UPDATED, tlm.TLM_COMPLETED),
        ),
    )
    for method, invoke, invalid, valid in checks:
        owner = owners[method]
        for result in invalid:
            owner.result = result
            try:
                invoke()
            except TypeError, ValueError:
                pass
            else:
                raise AssertionError(f"invalid socket result accepted: {method} {result!r}")
        for result in valid:
            owner.result = result
            assert invoke() == result
        if tagged:
            assert all(args[0] == 17 for args in owner.arguments)
        assert payload.get_address() == 73 and delay == sc.sc_time(2, sc.SC_NS)
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("SOCKET_RESULT_CONTRACTS_OK")
