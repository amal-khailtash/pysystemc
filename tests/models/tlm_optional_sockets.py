"""Verify native zero-bound policy across callable convenience socket families."""

from amal.eda.systemc import sc_core as sc, tlm_utils
import sys

name = sc.sc_module_name("top")
top = sc.sc_module(name)
optional = len(sys.argv) == 1
policy = sc.SC_ZERO_OR_MORE_BOUND if optional else sc.SC_ONE_OR_MORE_BOUND
sockets = []
for tagged in (False, True):
    sockets.append(tlm_utils.callback_target_socket(f"target_{tagged}", tagged=tagged, policy=policy))
    sockets.append(tlm_utils.callback_initiator_socket(f"initiator_{tagged}", tagged=tagged, policy=policy))
    sockets.append(tlm_utils.callback_passthrough_target_socket(f"passthrough_{tagged}", tagged=tagged, policy=policy))
sockets.append(tlm_utils.callback_multi_target_socket("multi_target", policy=policy))
sockets.append(tlm_utils.callback_multi_initiator_socket("multi_initiator", policy=policy))


def unused(*args):
    """Provide the native multi target export binder without any transaction calls."""
    raise AssertionError("unbound optional socket callback executed")


sockets[-2].register_b_transport(unused)
del name
if optional:
    sc.sc_start(sc.SC_ZERO_TIME)
    assert sockets[-1].size() == 0
    assert sc.sc_get_status() == sc.SC_PAUSED
    print("OPTIONAL_SOCKETS_OK")
else:
    try:
        sc.sc_start(sc.SC_ZERO_TIME)
    except Exception as error:
        assert "complete binding failed" in str(error), error
    else:
        raise AssertionError("required sockets accepted zero bindings")
    print("REQUIRED_SOCKETS_REJECTED")
