"""Verify generic and inventoried SystemC port families through cppyy."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_ports_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.sc_core_ports_probe

module_name = sc.sc_module_name("ports_public")
host = probe.PortHost(module_name)
value_if = probe.ValueIf
zero_policy = sc.SC_ZERO_OR_MORE_BOUND
Port = sc.sc_port[value_if, 0, zero_policy]
PortB = probe.PortBProbe[value_if]
created_ports = []


def retain(port):
    """Keep a newly constructed port in the teardown list and return it."""
    created_ports.append(port)
    return port


default_port = retain(Port())
named_port = retain(Port("named_port"))
default_port.bind(host.left)
named_port(host.right)
interface_port = retain(Port(host.left))
named_interface_port = retain(Port("named_interface_port", host.right))
base_parent = retain(PortB("base_parent", 0, zero_policy))
base_parent.bind(host.left)
base_child = retain(Port(base_parent))
named_base_child = retain(Port("named_base_child", base_parent))
this_parent = retain(Port("this_parent"))
this_parent.bind(host.right)
this_child = retain(Port(this_parent))
named_this_child = retain(Port("named_this_child", this_parent))

assert default_port.kind() == named_port.kind() == "sc_port"
assert interface_port.bind_count() == named_interface_port.bind_count() == 1
assert base_parent.bind_count() == base_child.bind_count() == named_base_child.bind_count() == 1
assert this_parent.bind_count() == this_child.bind_count() == named_this_child.bind_count() == 1
assert base_child.size() == 0 and this_child.size() == 0
assert probe.interface_matches_ref[value_if](interface_port.get_interface(0), host.left)
assert probe.interface_matches_ref[value_if](named_interface_port.get_interface(0), host.right)
assert probe.interface_matches_ref[value_if](base_parent.get_interface(0), host.left)
assert probe.interface_matches_ref[value_if](this_parent.get_interface(0), host.right)

# Exercise both bind overloads and the call operator on a multi-interface port.
operators = retain(Port("operators"))
operators.bind(host.left)
operators(host.right)
assert operators.size() == 2 and operators.bind_count() == 2
assert probe.interface_matches_ref[value_if](operators.get_interface(0), host.left)
assert probe.interface_matches_ref[value_if](operators.get_interface(1), host.right)
assert probe.interface_matches_ref[value_if](operators[0], host.left)
assert probe.interface_matches_ref[value_if](operators[1], host.right)
assert probe.interface_matches_ref[value_if](probe.const_interface_at(operators, 0), host.left)
assert probe.interface_matches_ref[value_if](probe.const_interface_at(operators, 1), host.right)
assert probe.interface_matches_ref[value_if](probe.const_subscript(operators, 0), host.left)
assert probe.interface_matches_ref[value_if](probe.const_subscript(operators, 1), host.right)
assert probe.interface_matches_ref[value_if](operators.get_interface(), host.left)
assert operators.value() == probe.read_port_value[value_if](operators) == 13
assert probe.read_const_port_value[value_if](operators) == 13
assert probe.interface_matches_ref[value_if](probe.const_arrow(operators), host.left)
assert probe.has_interface_type[value_if](operators.get_interface_type())

# All native sc_port specializations are instantiated by SystemC headers for
# signal input/output (bool/logic) and event-queue interfaces. Exercise all
# eight generic constructors for each specialization, including both parent
# port forms, then inspect the live interface through every indexed overload.
specializations = (
    ("in_bool", sc.sc_signal_in_if[bool], host.bool_signal),
    ("in_logic", sc.sc_signal_in_if[sc_dt.sc_logic], host.logic_signal),
    ("inout_bool", sc.sc_signal_inout_if[bool], host.bool_signal),
    ("inout_logic", sc.sc_signal_inout_if[sc_dt.sc_logic], host.logic_signal),
    ("event_queue", sc.sc_event_queue_if, host.event_queue),
)
specialized_count = 0
for label, interface_type, channel in specializations:
    specialized_port = sc.sc_port[interface_type]
    base_probe_type = probe.PortBProbe[interface_type]
    native_port_b_type = sc.sc_port_b[interface_type]
    policy = sc.SC_ZERO_OR_MORE_BOUND
    assert native_port_b_type.base_type.__cpp_name__ == "sc_core::sc_port_base"
    assert native_port_b_type.this_type.__cpp_name__ == native_port_b_type.__cpp_name__
    assert native_port_b_type.port_type.__cpp_name__ == native_port_b_type.__cpp_name__
    default = retain(specialized_port())
    named = retain(specialized_port(f"{label}_named"))
    default.bind(channel)
    named(channel)
    by_interface = retain(specialized_port(channel))
    named_by_interface = retain(specialized_port(f"{label}_by_interface", channel))
    base_unnamed = retain(base_probe_type(1, policy))
    base_parent = retain(base_probe_type(f"{label}_base_parent", 1, policy))
    base_unnamed.bind(channel)
    base_parent.bind(channel)
    from_base = retain(specialized_port(base_parent))
    named_from_base = retain(specialized_port(f"{label}_from_base", base_parent))
    this_parent = retain(specialized_port(f"{label}_this_parent"))
    this_parent.bind(channel)
    from_this = retain(specialized_port(this_parent))
    named_from_this = retain(specialized_port(f"{label}_named_from_this", this_parent))

    assert default.kind() == named.kind() == "sc_port"
    for bound in (by_interface, named_by_interface, base_parent, this_parent):
        assert bound.bind_count() == 1
        assert bound.size() == 1
        assert probe.interface_matches_ref[interface_type](bound.get_interface(0), channel)
        assert probe.interface_matches_ref[interface_type](bound[0], channel)
        assert probe.has_interface_type[interface_type](bound.get_interface_type())
    for parent_bound in (from_base, named_from_base, from_this, named_from_this):
        assert parent_bound.bind_count() == 1
        assert parent_bound.size() == 0
    assert base_unnamed.bind_count() == 1 and base_unnamed.size() == 1
    assert probe.interface_matches_ref[interface_type](base_unnamed.get_interface(0), channel)
    base_bind_child = retain(base_probe_type(f"{label}_base_bind_child", 1, policy))
    base_call_child = retain(base_probe_type(f"{label}_base_call_child", 1, policy))
    base_bind_child.bind(base_parent)
    base_call_child(base_parent)
    for parent_bound in (base_bind_child, base_call_child):
        assert parent_bound.bind_count() == 1 and parent_bound.size() == 0
    assert probe.interface_matches_ref[interface_type](base_parent.base_interface(), channel)
    assert probe.interface_matches_ref[interface_type](base_parent.const_base_interface(), channel)
    assert probe.interface_matches_ref[interface_type](base_parent.get_interface(0), channel)
    assert probe.interface_matches_ref[interface_type](base_parent[0], channel)
    assert probe.interface_matches_ref[interface_type](base_parent.interface_arrow(), channel)
    assert probe.interface_matches_ref[interface_type](base_parent.const_interface_at(0), channel)
    assert probe.interface_matches_ref[interface_type](base_parent.const_subscript(0), channel)
    assert probe.interface_matches_ref[interface_type](base_parent.const_interface_arrow(), channel)
    specialized_count += 1

# The public sc_port_b template exposes its three aliases and all typed/base
# interface views. Derived probes provide its protected constructors/helpers.
native_port_b = sc.sc_port_b[value_if]
assert native_port_b.base_type.__cpp_name__ == "sc_core::sc_port_base"
assert native_port_b.this_type.__cpp_name__ == native_port_b.__cpp_name__
assert native_port_b.port_type.__cpp_name__ == native_port_b.__cpp_name__
base_default = retain(PortB(1, zero_policy))
base_named = retain(PortB("base_named", 1, zero_policy))
base_default.bind(host.left)
base_named(host.right)
base_bind_child = retain(PortB("base_bind_child", 1, zero_policy))
base_call_child = retain(PortB("base_call_child", 1, zero_policy))
base_bind_child.bind(base_default)
base_call_child(base_named)
assert base_bind_child.bind_count() == base_call_child.bind_count() == 1
assert base_bind_child.size() == base_call_child.size() == 0
assert base_default.size() == base_named.size() == 1
assert probe.interface_matches_ref[value_if](base_default.get_interface(0), host.left)
assert probe.interface_matches_ref[value_if](base_named.get_interface(0), host.right)
assert probe.interface_matches_ref[value_if](base_default.interface_arrow(), host.left)
assert probe.interface_matches_ref[value_if](base_default.const_interface_at(0), host.left)
assert probe.interface_matches_ref[value_if](base_default.const_subscript(0), host.left)
assert probe.interface_matches_ref[value_if](base_default.const_interface_arrow(), host.left)
assert probe.interface_matches_ref[value_if](base_default.base_interface(), host.left)
assert probe.interface_matches_ref[value_if](base_default.const_base_interface(), host.left)
assert probe.has_interface_type[value_if](base_default.get_interface_type())

# Access the protected vbind and make_sensitive overloads through the native
# derived probe. Type mismatches return the native status code without binding.
vbind_probe = retain(PortB("vbind_probe", 0, zero_policy))
assert vbind_probe.probe_vbind_interface(host.left) == 0
assert vbind_probe.bind_count() == 1 and vbind_probe.size() == 1
assert vbind_probe.probe_vbind_interface(host.bool_signal) == 2
assert vbind_probe.probe_vbind_port(base_named) == 0
assert vbind_probe.probe_vbind_port(default_port) == 0
assert vbind_probe.bind_count() == 3
sensitivity_probe = retain(PortB("sensitivity_probe", 1, zero_policy))
sensitivity_probe.probe_method_sensitivity()
sensitivity_probe.probe_thread_sensitivity()

print(
    "SC_PORT_FAMILY_REFERENCE "
    f"generic={host.left.value()},{host.right.value()} constructors=8x6 "
    f"specializations={specialized_count} base_aliases=3 operators=2 teardown={len(created_ports)}"
)

# Destroy child ports before their borrowed parents and verify hierarchy lookup
# no longer sees any port. The channels and host remain alive for this check.
for port in reversed(created_ports):
    port_name = str(port.name())
    port.__destruct__()
    assert not sc.sc_find_object(port_name), port_name
del module_name
