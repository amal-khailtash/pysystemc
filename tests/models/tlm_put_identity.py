"""Verify shared TLM put identity, copied samples and delayed readiness."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


class Owner:
    """Retain copied put samples and configurable readiness."""

    def __init__(self):
        """Initialize callback records and readiness."""
        self.values = []
        self.ready = True

    def put(self, value):
        """Retain the value copied at the native callback boundary."""
        self.values.append(value)

    def nb_put(self, value):
        """Store a copied value only when ready."""
        if self.ready:
            self.put(value)
        return self.ready

    def nb_can_put(self):
        """Report readiness without modifying stored samples."""
        return self.ready


cppyy.cppdef("""
/** Return the borrowed virtual interface address; no callbacks or retained references. */
std::uintptr_t put_interface_identity(sc_core::sc_interface* value) {
  return reinterpret_cast<std::uintptr_t>(value);
}
""")
name = sc.sc_module_name("top")
top = sc.sc_module(name)
groups = []
for label, datatype in (("int", "int"), ("long_long", "long long"), ("wide137", sc_dt.sc_biguint[137])):
    for order, contracts in (
        ("combined", (tlm.tlm_put_if, tlm.tlm_blocking_put_if, tlm.tlm_nonblocking_put_if)),
        ("blocking", (tlm.tlm_blocking_put_if, tlm.tlm_nonblocking_put_if, tlm.tlm_put_if)),
        ("nonblocking", (tlm.tlm_nonblocking_put_if, tlm.tlm_blocking_put_if, tlm.tlm_put_if)),
    ):
        owner = Owner()
        ports = []
        for contract in contracts:
            port = sc.sc_port[contract[datatype]]()
            port(owner)
            ports.append(port)
        groups.append((label, datatype, order, owner, ports))
del name
seen = []
for label, datatype, order, owner, ports in groups:
    interfaces = [port.get_interface() for port in ports]
    addresses = [cppyy.addressof(cppyy.bind_object(cppyy.addressof(native), type(native))) for native in interfaces]
    # Virtual base conversion establishes identity even with adjusted subobject pointers.
    roots = [cppyy.gbl.put_interface_identity(native) for native in interfaces]
    assert len(set(roots)) == 1, (label, order, addresses, roots)
    full = next(native for native in interfaces if isinstance(native, tlm.tlm_put_if[datatype]))
    events = [cppyy.addressof(native.ok_to_put()) for native in interfaces if hasattr(native, "ok_to_put")]
    assert len(set(events)) == 1, (label, order, events)
    sample = datatype(17) if label == "wide137" else 17
    if label == "wide137":
        sample[136] = True
    full.put(sample)
    assert full.nb_put(sample)
    owner.ready = False
    assert not full.nb_can_put() and not full.nb_put(sample)
    assert len(owner.values) == 2
    if label == "wide137":
        sample[136] = False
        assert all(bool(value[136]) and value.to_uint64() == 17 for value in owner.values)
    else:
        assert owner.values == [17, 17]
    top.sc_method(
        sc.sc_gen_unique_name("ready", False),
        lambda label=label, order=order: seen.append((label, order, str(sc.sc_time_stamp()))),
    )
    top.sensitive(full.ok_to_put())
    top.dont_initialize()
    full.notify_ready(sc.sc_time(1, sc.SC_NS))
sc.sc_start(2, sc.SC_NS)
assert sorted(seen) == sorted((label, order, "1 ns") for label, datatype, order, owner, ports in groups), seen
sc.sc_stop()
print("TLM_PUT_IDENTITY 9 2 1 ns")
