"""Verify public and protected sc_port_base declarations through cppyy."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_port_base_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.sc_core_port_base_probe

assert sc.sc_port_base.__cpp_name__ == "sc_core::sc_port_base"
assert sc.sc_port_base.this_type.__cpp_name__ == "sc_core::sc_port_base"
policy = sc.SC_ZERO_OR_MORE_BOUND
tree = probe.PortTreeProbe("port_public")
unnamed = tree.unnamed
default_policy = tree.default_policy
named = tree.named
named_default_policy = tree.named_default_policy
parent = tree.parent
child = tree.child
interface = tree.interface
assert unnamed.name() and unnamed.kind() == "sc_port_base"
assert default_policy.name() and default_policy.bind_count() == 0
assert named.name() == "port_public.named_port" and named.kind() == "sc_port_base"
assert named_default_policy.name() == "port_public.named_default_policy"
assert named.has_binding_state() and parent.has_binding_state()
assert int(named.bind_count()) == 0
assert not named.get_interface() and not probe.const_interface(named)
assert named.get_interface_type().name() == parent.get_interface_type().name()

# The protected overloads take references. These native adapters force the
# interface-reference and parent-port-reference candidates explicitly.
probe.bind_interface(named, interface)
probe.bind_parent(child, parent)
assert named.bind_count() == 1 and parent.bind_count() == 0 and child.bind_count() == 1
assert probe.same_interface(named.get_interface(), interface)
assert probe.same_interface(probe.const_interface(named), interface)
assert not child.get_interface() and not probe.const_interface(child)
assert cppyy.addressof(child.get_parent_object()) == cppyy.addressof(tree)
assert named.get_interface_type().name() == parent.get_interface_type().name()

assert named.run_lifecycle_hooks() == "BESF"
assert child.run_lifecycle_hooks() == "BESF"
diagnostic = named.report_error_text()
assert named.name() in diagnostic and "port probe" in diagnostic

# These protected overloads accept process handles. Null handles are retained
# only in temporary binding records and are safely released by port destruction.
unnamed.probe_method_sensitivity()
unnamed.probe_thread_sensitivity()

print(
    "SC_PORT_BASE_PUBLIC_REFERENCE "
    f"named={named.name()} kind={named.kind()} binds={named.bind_count()},{child.bind_count()} "
    f"interface={int(bool(named.get_interface()))} parent_interface={int(bool(child.get_interface()))} "
    f"hooks={named.run_lifecycle_hooks()} "
    f"error={int(named.name() in diagnostic and 'port probe' in diagnostic)}"
)
tree.__destruct__()
assert not sc.sc_find_object("port_public")
assert not sc.sc_find_object("port_public.named_port")
