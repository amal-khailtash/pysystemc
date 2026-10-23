"""Exercise public sc_interface dispatch through a native SystemC port."""

from amal.eda.systemc import sc_core as sc
import cppyy

event_interface = sc.declare_interface(
    "ScInterfaceDefaultEventProbe",
    {"default_event": ("const sc_core::sc_event&", "")},
)


class PythonInterface(sc.sc_interface):
    """Implement public native interface callbacks with retained Python state."""

    def __init__(self):
        """Create stable event storage and callback counters."""
        super().__init__()
        self.register_count = 0
        self.port_name = ""
        self.interface_type = ""

    def register_port(self, port, if_typename):
        """Record the native port registration arguments."""
        self.register_count += 1
        self.port_name = str(port.name())
        self.interface_type = str(if_typename)


class EventSource(event_interface):
    """Return stable Python-owned event storage through the native ref adapter."""

    def __init__(self):
        """Keep the returned event alive for the native port lifetime."""
        super().__init__()
        self.event = sc.sc_event("python_ready")

    def python_reference_default_event(self):
        """Return the retained event pointer for the native reference trampoline."""
        return self.event


name = sc.sc_module_name("interface_top")
module = sc.sc_module(name)
base = sc.sc_interface()
base_port = sc.sc_port[sc.sc_interface]("base_port")
base_port.bind(base)
interface = PythonInterface()
port = sc.sc_port[sc.sc_interface]("interface_port")
port.bind(interface)
event_source = EventSource()
event_port = sc.sc_port[event_interface]("event_port")
event_port.bind(event_source)
del name
sc.sc_start(sc.SC_ZERO_TIME)

event = event_port.get_interface().default_event()
fallback = base_port.get_interface().default_event()
assert interface.register_count == 1, interface.register_count
assert interface.port_name == "interface_top.interface_port", interface.port_name
assert interface.interface_type, "native register_port type name was empty"
assert str(event.basename()) == "python_ready", event.name()
assert cppyy.addressof(fallback) == cppyy.addressof(sc.sc_event.none)

sc.sc_stop()
del base_port
base.__destruct__()
print("INTERFACE_REFERENCE 1 1 1 1 python_ready")
