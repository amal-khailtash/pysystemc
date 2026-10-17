"""Validate structural native signal interfaces and borrowed wide/event identity."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


class Owner:
    """Provide native signal storage and event callbacks."""

    def __init__(self):
        """Create stable native state."""
        self.value = sc_dt.sc_biguint[137](17)
        self.changed = sc.sc_event("changed")

    def read(self):
        """Borrow current wide storage."""
        return self.value

    def get_data_ref(self):
        """Borrow trace storage."""
        return self.value

    def value_changed_event(self):
        """Borrow the actual notification event."""
        return self.changed

    def event(self):
        """Query native event delta identity."""
        return self.changed.triggered()

    def write(self, value):
        """Assign storage and notify next delta."""
        self.value.__assign__(value)
        self.changed.notify(sc.SC_ZERO_TIME)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Owner()
port = sc.sc_port[sc.sc_signal_inout_if[sc_dt.sc_biguint[137]]]("port")
port(owner)
reader = sc.sc_port[sc.sc_signal_in_if[sc_dt.sc_biguint[137]]]("reader")
reader(owner)
edge_ports = []
for value_type in (bool, sc_dt.sc_logic):
    backing = sc.sc_signal[value_type]("edge_backing")

    class EdgeOwner:
        """Delegate signal state while preserving native edge events."""

        def __init__(self, backing):
            """Retain the backing signal."""
            self.backing = backing

        def __getattr__(self, method):
            """Expose native signal callbacks as ordinary Python methods."""
            return getattr(self.backing, method)

    edge_owner = EdgeOwner(backing)
    edge_port = sc.sc_port[sc.sc_signal_in_if[value_type]]("edge_port")
    edge_port(edge_owner)
    edge_ports.append((edge_owner, edge_port, value_type))
del name
native = port.get_interface()
assert cppyy.addressof(native.read()) == cppyy.addressof(owner.value)
assert cppyy.addressof(native.get_data_ref()) == cppyy.addressof(owner.value)
assert cppyy.addressof(native.value_changed_event()) == cppyy.addressof(owner.changed)
assert cppyy.addressof(reader.get_interface().value_changed_event()) == cppyy.addressof(owner.changed)
assert native.get_writer_policy() == sc.SC_ONE_WRITER
owner.get_writer_policy = lambda: sc.SC_MANY_WRITERS
assert native.get_writer_policy() == sc.SC_MANY_WRITERS
owner.get_writer_policy = lambda: -1
try:
    native.get_writer_policy()
except ValueError:
    pass
else:
    raise AssertionError("invalid inout writer policy accepted")
owner.get_writer_policy = lambda: sc.SC_UNCHECKED_WRITERS
assert native.get_writer_policy() == sc.SC_UNCHECKED_WRITERS
seen = []
top.sc_method("observe", lambda: seen.append((native.read().to_uint(), native.event())))
top.sensitive(native.default_event())
top.dont_initialize()
native.write(sc_dt.sc_biguint[137](29))
sc.sc_start(1, sc.SC_NS)
assert seen == [(29, True)], seen
assert native.read().to_uint() == 29
for edge_owner, edge_port, value_type in edge_ports:
    edge = edge_port.get_interface()
    assert cppyy.addressof(edge.posedge_event()) == cppyy.addressof(edge_owner.backing.posedge_event())
    edge_owner.backing.write(True if value_type is bool else sc_dt.sc_logic("1"))
sc.sc_start(1, sc.SC_NS)
for edge_owner, edge_port, value_type in edge_ports:
    edge = edge_port.get_interface()
    assert bool(edge.read()) if value_type is bool else edge.read().to_char() == "1"
sc.sc_stop()
print("NATIVE_STRUCTURAL_SIGNAL_OK")
