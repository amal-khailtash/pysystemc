"""Compare hierarchical external-name registration overloads with native SystemC."""

from amal.eda.systemc import sc_core as sc

name = sc.sc_module_name("registry")
top = sc.sc_module(name)
event = sc.sc_event("event")
del name
for parent in (None, top):

    def call(function, key):
        """Select the global or explicit-parent native overload."""
        return function(key) if parent is None else function(parent, key)

    key = "external"
    full = key if parent is None else "registry.external"
    assert not call(sc.sc_hierarchical_name_exists, key)
    assert not call(sc.sc_get_hierarchical_name, key)
    assert call(sc.sc_register_hierarchical_name, key)
    assert not call(sc.sc_register_hierarchical_name, key)
    assert call(sc.sc_hierarchical_name_exists, key)
    copied = call(sc.sc_get_hierarchical_name, key)
    assert copied == full
    assert not sc.sc_find_object(full) and not sc.sc_find_event(full)
    assert call(sc.sc_unregister_hierarchical_name, key)
    assert not call(sc.sc_unregister_hierarchical_name, key)
    assert not call(sc.sc_get_hierarchical_name, key)
    assert copied == full
    print("NAME_REGISTRY_REFERENCE", full, "1 0 1 1 0 0")
assert sc.sc_hierarchical_name_exists("registry")
assert not sc.sc_unregister_hierarchical_name("registry")
assert sc.sc_hierarchical_name_exists(top, "event")
assert not sc.sc_register_hierarchical_name(top, "event")
assert not sc.sc_unregister_hierarchical_name(top, "event")
print("NAME_REGISTRY_OBJECTS_REFERENCE 1 0 1 0 0")
