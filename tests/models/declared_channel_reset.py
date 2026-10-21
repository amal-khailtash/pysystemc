"""Verify arbitrary declared methods release GIL and preserve callback unwind."""

from amal.eda.systemc import sc_core as sc
from typing import Protocol
import sys

interface = sc.declare_interface("DeclaredReset", {"acquire_resource": ("int", ""), "inspect_resource": ("int", "")})
records = []


class Channel(sc.channel_interface(interface)):
    """Own callbacks borrowed by a native port throughout simulation."""

    def __init__(self):
        """Construct both native channel and declared-interface bases."""
        name = sc.sc_module_name("channel")
        super().__init__(name)

    def acquire_resource(self):
        """Suspend inside an arbitrarily named method until reset and kill."""
        try:
            sc.wait(100, sc.SC_NS)
        finally:
            records.append(str(sc.sc_time_stamp()))
        return 0

    def inspect_resource(self):
        """Raise a normal callback error before recovering a valid value."""
        if self.fail:
            raise ValueError("declared inspection failure")
        return 17


name = sc.sc_module_name("top")
top = sc.sc_module(name)
if len(sys.argv) > 1:

    @sc.interface(methods={"acquire_resource": ("int", ""), "inspect_resource": ("int", "")})
    class Contract(Protocol):
        """Declare explicit native signatures for structural callbacks."""

    class Owner:
        """Use the same arbitrary callbacks without native inheritance."""

        acquire_resource = Channel.acquire_resource
        inspect_resource = Channel.inspect_resource

    channel = Owner()
    port = sc.sc_port[Contract]("port")
else:
    channel = Channel()
    port = sc.sc_port[interface]("port")
channel.fail = True
port(channel)
del name
try:
    port.get_interface().inspect_resource()
except ValueError as error:
    assert "declared inspection failure" in str(error)
else:
    raise AssertionError("declared callback error lost")
channel.fail = False
assert port.get_interface().inspect_resource() == 17


def worker():
    """Invoke a borrowed declared interface from a Python thread process."""
    port.get_interface().acquire_resource()
    raise AssertionError("interrupted operation returned")


handle = sc.sc_spawn(worker)


def control():
    """Reset then kill the suspended custom callback."""
    sc.wait(2, sc.SC_NS)
    handle.reset()
    sc.wait(1, sc.SC_NS)
    handle.kill()


controller = sc.sc_spawn(control)
sc.sc_start(4, sc.SC_NS)
assert records == ["2 ns", "3 ns"], records
assert handle.terminated() and controller.terminated()
assert port.get_interface().inspect_resource() == 17
sc.sc_stop()
print("DECLARED_CHANNEL_RESET_OK")
