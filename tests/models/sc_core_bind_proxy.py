"""Verify public bind-proxy nil storage and interface/port constructors."""

from amal.eda.systemc import sc_core as sc
import cppyy

nil = sc.SC_BIND_PROXY_NIL
assert not nil.iface and not nil.port
empty = sc.sc_bind_proxy()
assert not empty.iface and not empty.port
name = sc.sc_module_name("top")
top = sc.sc_module(name)
signal = sc.sc_signal[int]("signal")
port = sc.sc_port[sc.sc_signal_in_if[int]]("port")
port(signal)
interface_proxy = sc.sc_bind_proxy(signal)
port_proxy = sc.sc_bind_proxy(port)
assert cppyy.addressof(interface_proxy.iface) == cppyy.addressof(signal)
assert not interface_proxy.port
assert cppyy.addressof(port_proxy.port) == cppyy.addressof(port)
assert not port_proxy.iface
assert not nil.iface and not nil.port
del name
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("BIND_PROXY_REFERENCE 1 1 1")
