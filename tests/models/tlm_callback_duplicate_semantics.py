"""Verify native duplicate callback policy and Python callback retention."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils


records = []
handler = sc.sc_report_handler
previous_warning_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
module_name = sc.sc_module_name("duplicate_callbacks")
top = sc.sc_module(module_name)
target_cases = (
    ("simple_target", tlm_utils.callback_target_socket("simple_target", policy=sc.SC_ZERO_OR_MORE_BOUND), False, False),
    (
        "tagged_target",
        tlm_utils.callback_target_socket("tagged_target", tagged=True, policy=sc.SC_ZERO_OR_MORE_BOUND),
        True,
        False,
    ),
    (
        "passthrough_target",
        tlm_utils.callback_passthrough_target_socket("passthrough_target", policy=sc.SC_ZERO_OR_MORE_BOUND),
        False,
        False,
    ),
    (
        "tagged_passthrough_target",
        tlm_utils.callback_passthrough_target_socket(
            "tagged_passthrough_target", tagged=True, policy=sc.SC_ZERO_OR_MORE_BOUND
        ),
        True,
        False,
    ),
    (
        "multi_target",
        tlm_utils.callback_multi_target_socket("multi_target", policy=sc.SC_ZERO_OR_MORE_BOUND),
        False,
        True,
    ),
)
initiator_cases = (
    (
        "simple_initiator",
        tlm_utils.callback_initiator_socket("simple_initiator", policy=sc.SC_ZERO_OR_MORE_BOUND),
        False,
        False,
    ),
    (
        "tagged_initiator",
        tlm_utils.callback_initiator_socket("tagged_initiator", tagged=True, policy=sc.SC_ZERO_OR_MORE_BOUND),
        True,
        False,
    ),
    (
        "multi_initiator",
        tlm_utils.callback_multi_initiator_socket("multi_initiator", policy=sc.SC_ZERO_OR_MORE_BOUND),
        False,
        True,
    ),
)
multi_target_driver = tlm_utils.callback_initiator_socket("multi_target_driver", policy=sc.SC_ZERO_OR_MORE_BOUND)
multi_initiator_target = tlm_utils.callback_target_socket("multi_initiator_target", policy=sc.SC_ZERO_OR_MORE_BOUND)
throwing_warning_socket = tlm_utils.callback_target_socket("throwing_warning", policy=sc.SC_ZERO_OR_MORE_BOUND)
del module_name
payload = tlm.tlm_generic_payload()
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(0, sc.SC_NS)


class Callback:
    """Record callback identity and return the contract for one TLM slot."""

    def __init__(self, label, slot, result, indexed=False):
        """Store a callback label, slot kind and distinguishable result."""
        self.label = label
        self.slot = slot
        self.result = result
        self.indexed = indexed

    def __call__(self, *args):
        """Record optional native ID and update native output parameters."""
        offset = int(args[0]) if self.indexed else None
        records.append((self.label, offset))
        if self.slot in ("register_nb_transport_fw", "register_nb_transport_bw"):
            return self.result
        if self.slot == "register_transport_dbg":
            return self.result
        if self.slot == "register_get_direct_mem_ptr":
            result = args[-1]
            result.set_start_address(self.result)
            result.set_end_address(self.result + 3)
            result.allow_none()
            return False
        return None


def target_call(socket, is_multi, slot):
    """Dispatch one registered target callback through its native bridge."""
    if is_multi:
        if slot == "register_b_transport":
            socket.deliver(0, payload, delay)
            return None
        if slot == "register_nb_transport_fw":
            return socket.forward(0, payload, phase, delay)
        if slot == "register_transport_dbg":
            return socket.debug_transport(0, payload)
        result = tlm.tlm_dmi()
        value = socket.direct_memory(0, payload, result)
        return value, result

    interface = socket.get_base_export().get_interface()
    if slot == "register_b_transport":
        interface.b_transport(payload, delay)
        return None
    if slot == "register_nb_transport_fw":
        return interface.nb_transport_fw(payload, phase, delay)
    if slot == "register_transport_dbg":
        return interface.transport_dbg(payload)
    result = tlm.tlm_dmi()
    value = interface.get_direct_mem_ptr(payload, result)
    return value, result


def check_target(socket, family, tagged, slot, is_multi):
    """Check one target slot's duplicate policy, warning and retained owner."""
    indexed = tagged or is_multi
    socket_name = f"{family}_{slot}"
    first_result = tlm.TLM_ACCEPTED if slot == "register_nb_transport_fw" else 7
    duplicate_result = tlm.TLM_UPDATED if slot == "register_nb_transport_fw" else 9
    first = Callback(f"{socket_name}.first", slot, first_result, indexed=indexed)
    first_ref = weakref.ref(first)
    assert getattr(socket, slot)(first, *(11,) if tagged else ()) is None
    del first

    duplicate = Callback(f"{socket_name}.duplicate", slot, duplicate_result, indexed=indexed)
    duplicate_ref = weakref.ref(duplicate)
    warnings_before = handler.get_count(sc.SC_WARNING)
    assert getattr(socket, slot)(duplicate, *(22,) if tagged else ()) is None
    assert handler.get_count(sc.SC_WARNING) == warnings_before + 1
    del duplicate
    gc.collect()

    replaces = tagged and slot == "register_get_direct_mem_ptr" and family == "tagged_target"
    assert (first_ref() is None) is replaces
    assert (duplicate_ref() is not None) is replaces

    records.clear()
    result = target_call(socket, is_multi, slot)
    # Tagged native registration updates the stored user ID even when the
    # duplicate callback pointer itself is rejected.
    expected_id = 22 if tagged else 0 if is_multi else None
    expected_callback = f"{socket_name}.duplicate" if replaces else f"{socket_name}.first"
    assert records == [(expected_callback, expected_id)], (family, slot, records)
    if family == "simple_target" and slot == "register_b_transport":
        print("DUPLICATE_REF simple", records[0][0].rsplit(".", 1)[1], -1)
    if family == "tagged_target" and slot == "register_get_direct_mem_ptr":
        print("DUPLICATE_REF tagged_dmi", records[0][0].rsplit(".", 1)[1], records[0][1])
    if slot == "register_nb_transport_fw":
        assert result == tlm.TLM_UPDATED if replaces else result == tlm.TLM_ACCEPTED
    elif slot == "register_transport_dbg":
        assert result == (9 if replaces else 7)
    elif slot == "register_get_direct_mem_ptr":
        _, dmi = result
        assert dmi.get_start_address() == (9 if replaces else 7)
    return socket, first_ref, duplicate_ref


def initiator_call(socket, is_multi, slot):
    """Dispatch one registered initiator callback through its native bridge."""
    if is_multi:
        if slot == "register_nb_transport_bw":
            return socket.backward(3, payload, phase, delay)
        socket.invalidate_memory(3, 0, 15)
        return None
    interface = socket.get_base_export().get_interface()
    if slot == "register_nb_transport_bw":
        return interface.nb_transport_bw(payload, phase, delay)
    interface.invalidate_direct_mem_ptr(0, 15)
    return None


def check_initiator(socket, family, tagged, slot, is_multi):
    """Check initiator duplicate policy, warning and callback release."""
    indexed = tagged or is_multi
    socket_name = f"{family}_{slot}"
    first = Callback(f"{socket_name}.first", slot, tlm.TLM_ACCEPTED, indexed=indexed)
    first_ref = weakref.ref(first)
    assert getattr(socket, slot)(first, *(11,) if tagged else ()) is None
    del first

    duplicate = Callback(f"{socket_name}.duplicate", slot, tlm.TLM_UPDATED, indexed=indexed)
    duplicate_ref = weakref.ref(duplicate)
    warnings_before = handler.get_count(sc.SC_WARNING)
    assert getattr(socket, slot)(duplicate, *(22,) if tagged else ()) is None
    assert handler.get_count(sc.SC_WARNING) == warnings_before + 1
    del duplicate
    gc.collect()
    assert first_ref() is not None and duplicate_ref() is None

    records.clear()
    result = initiator_call(socket, is_multi, slot)
    # Native tagged registration stores a fresh user ID after its duplicate
    # pointer check, so the first callable observes the duplicate ID.
    expected_id = 22 if tagged else 3 if is_multi else None
    assert records == [(f"{socket_name}.first", expected_id)], records
    if slot == "register_nb_transport_bw":
        assert result == tlm.TLM_ACCEPTED
    return socket, first_ref, duplicate_ref


retained = []
for family, socket, tagged, is_multi in target_cases:
    for slot in (
        "register_b_transport",
        "register_nb_transport_fw",
        "register_transport_dbg",
        "register_get_direct_mem_ptr",
    ):
        retained.append(check_target(socket, family, tagged, slot, is_multi))

for family, socket, tagged, is_multi in initiator_cases:
    for slot in ("register_nb_transport_bw", "register_invalidate_direct_mem_ptr"):
        retained.append(check_initiator(socket, family, tagged, slot, is_multi))

# Bind after registration so the native end-of-elaboration callback binder
# receives the completed slot table.
multi_target_socket = target_cases[-1][1]
multi_target_driver.bind(multi_target_socket)
multi_initiator = initiator_cases[-1][1]
multi_initiator.bind(multi_initiator_target)


def throwing_warning_check():
    """Require warning exceptions to preserve installed callback ownership."""
    socket = throwing_warning_socket
    first = Callback("throwing_warning.first", "register_b_transport", None)
    first_ref = weakref.ref(first)
    socket.register_b_transport(first)
    del first
    duplicate = Callback("throwing_warning.duplicate", "register_b_transport", None)
    duplicate_ref = weakref.ref(duplicate)
    previous = handler.set_actions(sc.SC_WARNING, sc.SC_THROW)
    before = handler.get_count(sc.SC_WARNING)
    try:
        socket.register_b_transport(duplicate)
    except sc.sc_report as error:
        assert error.get_severity() == sc.SC_WARNING
    else:
        raise AssertionError("warning action did not throw")
    finally:
        handler.set_actions(sc.SC_WARNING, previous)
    assert handler.get_count(sc.SC_WARNING) == before + 1
    del duplicate
    gc.collect()
    assert first_ref() is not None and duplicate_ref() is None
    socket.get_base_export().get_interface().b_transport(payload, delay)
    assert records[-1] == ("throwing_warning.first", None)


throwing_warning_check()
sc.sc_start(sc.SC_ZERO_TIME)

multi_target_interface = multi_target_driver.get_interface()
for slot, expected_id, expected_result in (
    ("register_b_transport", 0, None),
    ("register_nb_transport_fw", 0, tlm.TLM_ACCEPTED),
    ("register_transport_dbg", 0, 7),
    ("register_get_direct_mem_ptr", 0, False),
):
    records.clear()
    if slot == "register_b_transport":
        multi_target_interface.b_transport(payload, delay)
        result = None
    elif slot == "register_nb_transport_fw":
        result = multi_target_interface.nb_transport_fw(payload, phase, delay)
    elif slot == "register_transport_dbg":
        result = multi_target_interface.transport_dbg(payload)
    else:
        dmi = tlm.tlm_dmi()
        result = multi_target_interface.get_direct_mem_ptr(payload, dmi)
        assert dmi.get_start_address() == 7
    assert records == [(f"multi_target_{slot}.first", expected_id)], records
    assert result == expected_result

multi_initiator_interface = multi_initiator_target.get_base_port().get_interface()
records.clear()
assert multi_initiator_interface.nb_transport_bw(payload, phase, delay) == tlm.TLM_ACCEPTED
assert records == [("multi_initiator_register_nb_transport_bw.first", 0)], records
records.clear()
multi_initiator_interface.invalidate_direct_mem_ptr(0, 15)
assert records == [("multi_initiator_register_invalidate_direct_mem_ptr.first", 0)]
handler.set_actions(sc.SC_WARNING, previous_warning_actions)
print(f"CALLBACK_DUPLICATE_PYTHON_OK {len(retained)}")
