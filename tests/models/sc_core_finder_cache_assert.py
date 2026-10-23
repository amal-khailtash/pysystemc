"""Trigger the native same-port cache assertion in an isolated subprocess."""

import resource
import signal

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
# cppyy can install a diagnostic SIGABRT handler; let the native assertion
# terminate this dedicated process with the operating system's signal status.
signal.signal(signal.SIGABRT, signal.SIG_DFL)

cppyy.cppdef(
    r"""
    namespace pysystemc_event_finder_test {
    #if defined(NDEBUG) && !defined(SC_ENABLE_ASSERTIONS)
    constexpr bool finder_assertions_enabled = false;
    #else
    constexpr bool finder_assertions_enabled = true;
    #endif
    }
    """
)

wide_type = sc_dt.sc_biguint[137]
wide_if = sc.sc_signal_in_if[wide_type]
module_name = sc.sc_module_name("finder_cache_assert")
module = sc.sc_module(module_name)
first_port = sc.sc_in[wide_type]("first_port")
second_port = sc.sc_in[wide_type]("second_port")
first_source = sc.sc_signal[wide_type]("first_source")
second_source = sc.sc_signal[wide_type]("second_source")
first_port.bind(first_source)
second_port.bind(second_source)

cache = sc.sc_event_finder_cache()
create = sc.sc_event_finder.cached_create[wide_if]
first_finder = create(cache, first_port, wide_if.value_changed_event)
assert cppyy.addressof(first_finder.port()) == cppyy.addressof(first_port)
assert cppyy.addressof(first_finder.find_event()) == cppyy.addressof(first_source.value_changed_event())
assert first_finder.__python_owns__ is False
assert first_finder._pysystemc_cache_owner is cache
assertions_enabled = bool(cppyy.gbl.pysystemc_event_finder_test.finder_assertions_enabled)
print("FINDER_CACHE_ASSERT_CONFIG", "enabled" if assertions_enabled else "disabled", flush=True)
print("FINDER_CACHE_ASSERT_ENTER", flush=True)
second_finder = create(cache, second_port, wide_if.value_changed_event)
if assertions_enabled:
    raise AssertionError("same-port cache assertion returned")
same_cached_finder = cppyy.addressof(first_finder) == cppyy.addressof(second_finder)
retains_first_port = cppyy.addressof(second_finder.port()) == cppyy.addressof(first_port)
retains_first_interface = cppyy.addressof(second_finder.find_event()) == cppyy.addressof(
    first_source.value_changed_event()
)
assert same_cached_finder and retains_first_port and retains_first_interface
print("FINDER_CACHE_ASSERT_RELEASE\t1\t1\t1", flush=True)
