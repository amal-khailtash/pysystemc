"""Verify invalid Python registrations leave native callable sockets usable."""

from amal.eda.systemc import sc_core as sc, tlm_utils

name = sc.sc_module_name("top")
top = sc.sc_module(name)
sockets = []
rejections = 0
for factory, methods in (
    (
        tlm_utils.callback_target_socket,
        ("register_b_transport", "register_nb_transport_fw", "register_transport_dbg", "register_get_direct_mem_ptr"),
    ),
    (
        tlm_utils.callback_passthrough_target_socket,
        ("register_b_transport", "register_nb_transport_fw", "register_transport_dbg", "register_get_direct_mem_ptr"),
    ),
    (tlm_utils.callback_initiator_socket, ("register_nb_transport_bw", "register_invalidate_direct_mem_ptr")),
):
    for tagged in (False, True):
        socket = factory(f"socket_{len(sockets)}", tagged=tagged, policy=sc.SC_ZERO_OR_MORE_BOUND)
        sockets.append(socket)
        for method in methods:
            try:
                getattr(socket, method)(None, *([47] if tagged else []))
            except TypeError as error:
                assert "callable" in str(error), error
                rejections += 1
            else:
                raise AssertionError("non-callable socket registration accepted")
for factory, methods in (
    (
        tlm_utils.callback_multi_target_socket,
        ("register_b_transport", "register_nb_transport_fw", "register_transport_dbg", "register_get_direct_mem_ptr"),
    ),
    (tlm_utils.callback_multi_initiator_socket, ("register_nb_transport_bw", "register_invalidate_direct_mem_ptr")),
):
    socket = factory(f"socket_{len(sockets)}", policy=sc.SC_ZERO_OR_MORE_BOUND)
    sockets.append(socket)
    for method in methods:
        try:
            getattr(socket, method)(None)
        except TypeError as error:
            assert "callable" in str(error), error
            rejections += 1
        else:
            raise AssertionError("non-callable multi registration accepted")


def unused(*args):
    """Bind the optional multi target export without receiving a transaction."""
    raise AssertionError("unused optional callback executed")


sockets[-2].register_b_transport(unused)
del name
sc.sc_start(sc.SC_ZERO_TIME)
assert rejections == 26, rejections
assert sc.sc_get_status() == sc.SC_PAUSED
print("SOCKET_REGISTRATION_ERRORS_OK")
