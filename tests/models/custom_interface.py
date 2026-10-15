"""Validate package-declared custom interface overrides through native ports."""

from amal.eda.systemc import sc_core as sc

interface = sc.declare_interface("ValueInterface", {"read_value": ("int", ""), "write_value": ("void", "int value")})


class Channel(interface):
    """Implement native interface calls in retained Python storage."""

    def __init__(self):
        """Initialize native dispatcher and Python value."""
        super().__init__()
        self.value = 0

    def read_value(self):
        """Return the Python value through native int conversion."""
        return self.value

    def write_value(self, value):
        """Store the native argument on the Python implementation."""
        self.value = value


module_name = sc.sc_module_name("custom")
model = sc.sc_channel(module_name)
channel = Channel()
port = sc.sc_port[interface]("port")
port.bind(channel)
del module_name
sc.sc_start(sc.SC_ZERO_TIME)
port.get_interface().write_value(17)
assert port.get_interface().read_value() == 17
assert channel.value == 17
assert (
    sc.declare_interface("ValueInterface", {"read_value": ("int", ""), "write_value": ("void", "int value")})
    == interface
)
try:
    sc.declare_interface("ValueInterface", {"read_value": ("bool", "")})
except ValueError:
    pass
else:
    raise AssertionError("conflicting declaration accepted")
sc.sc_stop()
print("CUSTOM_INTERFACE_OK")
