"""Exercise exact base getter proxies in an independently configured kernel."""

from pathlib import Path

import cppyy

from amal.eda.systemc import sc_core as sc, tlm

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_covariant_socket_probe.h"))
owner_type = cppyy.gbl.TlmCovariantOwner
owner = owner_type("owner")
assert owner_type.live_count == 1
assert owner.identities()
for socket in (owner.initiator, owner.target, owner.custom_initiator, owner.custom_target):
    assert isinstance(socket, tlm.tlm_base_socket_if)
    base = cppyy.bind_object(cppyy.gbl.tlm_covariant_base_address(socket), tlm.tlm_base_socket_if)
    direct = base.get_base_interface()
    assert not base.__python_owns__
    assert not direct.__python_owns__
    mutable = cppyy.gbl.tlm_covariant_mutable(socket)
    const = cppyy.gbl.tlm_covariant_const(socket)
    assert isinstance(mutable, cppyy.gbl.sc_core.sc_interface)
    assert isinstance(const, cppyy.gbl.sc_core.sc_interface)
    assert cppyy.addressof(direct) == cppyy.addressof(mutable)
    assert cppyy.addressof(mutable) == cppyy.addressof(const)
    assert cppyy.gbl.tlm_covariant_interface_address(mutable) == (
        cppyy.gbl.tlm_covariant_interface_address(socket.get_base_interface())
    )
    assert not mutable.__python_owns__
    assert not const.__python_owns__
sc.sc_start(sc.SC_ZERO_TIME)
assert owner.identities()
# Destroy only after dropping every borrowed proxy; no dangling dereference.
del base, direct, mutable, const, socket
owner.__destruct__()
assert owner_type.live_count == 0
print("TLM_COVARIANT_PYTHON_OK")
