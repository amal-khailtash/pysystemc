"""Exercise native reference-return callbacks on a declared interface."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


interface = sc.declare_interface(
    "DeclaredReferenceLifetime",
    {
        "value": ("sc_dt::sc_biguint<137>&", ""),
        "changed": ("const sc_core::sc_event&", ""),
    },
)


class Channel(sc.channel_interface(interface)):
    """Implement the interface with explicitly retained native result storage."""

    def __init__(self, name):
        """Construct stable value and event storage for reference callbacks."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.storage = sc_dt.sc_biguint[137](17)
        self.event_storage = sc.sc_event("changed")

    def python_reference_value(self):
        """Return the address of storage retained by this channel."""
        return self.storage

    def python_reference_changed(self):
        """Return the address of the retained notification event."""
        return self.event_storage


module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
channel = Channel("channel")
port = sc.sc_port[interface]("port")
port.bind(channel)
del module_name

native = port.get_interface()
value = native.value()
assert cppyy.addressof(value) == cppyy.addressof(channel.storage)
assert value.to_uint64() == 17
value[136] = True
assert channel.storage[136]
assert cppyy.addressof(native.value()) == cppyy.addressof(channel.storage)
assert cppyy.addressof(native.changed()) == cppyy.addressof(channel.event_storage)

observed = []
top.sc_method("observe", lambda: observed.append(str(sc.sc_time_stamp())))
top.sensitive(native.changed())
top.dont_initialize()
channel.event_storage.notify(sc.sc_time(2, sc.SC_NS))
sc.sc_start(3, sc.SC_NS)
assert observed == ["2 ns"], observed
assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
assert value[136] and value.to_uint64() == 17
sc.sc_stop()

print("DECLARED_REFERENCE_LIFETIME_OK")
