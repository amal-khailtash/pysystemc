"""Check exact native callback aliases through Python lookup and substitution."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    """
    #include <type_traits>
    namespace f9_core_alias_probe {
    static_assert(std::is_same<sc_core::CFT, int (*)(const void*, const void*)>::value);
    static_assert(std::is_same<sc_core::sc_plist_map_fn, void (*)(void*, void*)>::value);
    static_assert(std::is_class<sc_core::sc_plist_elem>::value);

    inline int call_comparator(sc_core::CFT callback)
    {
      return callback(nullptr, nullptr);
    }
    inline void call_mapper(sc_core::sc_plist_map_fn callback)
    {
      callback(nullptr, nullptr);
    }
    }
    """
)

assert sc.sc_plist_elem.__cpp_name__ == "sc_core::sc_plist_elem"


def alias_exposure(name):
    """Check Python-module and cppyy namespace alias lookup independently."""
    try:
        public_alias = getattr(sc, name)
    except AttributeError:
        public_alias = None
    try:
        cppyy_alias = getattr(cppyy.gbl.sc_core, name)
    except AttributeError:
        cppyy_alias = None
    assert bool(public_alias) == bool(cppyy_alias)
    if public_alias is not None:
        assert public_alias is cppyy_alias
    return public_alias is not None, cppyy_alias is not None


calls = []


def compare(left, right):
    """Return a known result through the exact CFT function-pointer type."""
    calls.append((left, right))
    return -17


mapper_calls = []


def map_item(value, context):
    """Record synchronous dispatch through the exact public map callback type."""
    mapper_calls.append((value, context))


probe = cppyy.gbl.f9_core_alias_probe
assert probe.call_comparator(compare) == -17 and len(calls) == 1
probe.call_mapper(map_item)
assert len(mapper_calls) == 1
cft_public, cft_cppyy = alias_exposure("CFT")
map_public, map_cppyy = alias_exposure("sc_plist_map_fn")
print("F9_CORE_ALIAS_TYPES 1 1 1 CFT", int(cft_public), int(cft_cppyy), "MAP_FN", int(map_public), int(map_cppyy))
