"""Validate Python channel/interface multiple inheritance and blocking dispatch."""

from amal.eda.systemc import sc_core as sc

write_if = sc.declare_interface("BlockingWriteInterface", {"write": ("void", "int value")})


class Channel(sc.channel_interface(write_if)):
    """Implement a native interface using Python channel state and guarded waits."""

    def __init__(self, name):
        """Construct both native bases and retain callback state."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.records = []

    def write(self, value):
        """Suspend within an interface callback, then record the native argument."""
        sc.wait(1, sc.SC_NS)
        self.records.append((value, str(sc.sc_time_stamp())))


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
channel = Channel("fifo_inst")
port = sc.sc_port[write_if]("out")
port.bind(channel)
del module_name
write_if.write.__release_gil__ = True


def producer():
    """Call Python channel logic through the native interface pointer."""
    port.get_interface().write(17)
    port.get_interface().write(23)


handle = sc.sc_spawn(producer, "producer")
sc.sc_start(3, sc.SC_NS)
assert channel.records == [(17, "1 ns"), (23, "2 ns")], channel.records
assert handle.terminated()
sc.sc_stop()
print("INTERFACE_CHANNEL_OK")
