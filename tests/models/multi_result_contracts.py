"""Verify five multi socket result contracts with independent connection routing."""

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
initiator = tlm_utils.callback_multi_initiator_socket("initiator", connections=2)
back_owners = {}
for method in ("register_invalidate_direct_mem_ptr", "register_nb_transport_bw"):
    owner = Results()
    getattr(initiator, method)(owner)
    back_owners[method] = owner
for index in range(2):
    target = tlm_utils.callback_multi_target_socket(f"target_{index}", connections=1)
    owners = dict(back_owners)
    for method in (
        "register_b_transport",
        "register_get_direct_mem_ptr",
        "register_transport_dbg",
        "register_nb_transport_fw",
    ):
        owner = Results()
        getattr(target, method)(owner)
        owners[method] = owner
    initiator.bind(target)
    entries.append((target, owners))
del name
payload = tlm.tlm_generic_payload()
payload.set_address(73)
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(2, sc.SC_NS)
dmi = tlm.tlm_dmi()
sc.sc_start(sc.SC_ZERO_TIME)
assert initiator.size() == 2
for index, (target, owners) in enumerate(entries):
    assert target.size() == 1
    forward = initiator[index]
    backward = target[0]
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
        first_call = len(owner.arguments)
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
        expected_index = index if method in ("register_invalidate_direct_mem_ptr", "register_nb_transport_bw") else 0
        assert all(args[0] == expected_index for args in owner.arguments[first_call:])
        assert payload.get_address() == 73 and delay == sc.sc_time(2, sc.SC_NS)
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("MULTI_RESULT_CONTRACTS_OK")
