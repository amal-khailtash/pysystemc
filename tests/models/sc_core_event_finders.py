"""Compare event finder construction, caching, lookup, and reports with C++."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


cppyy.cppdef(
    r"""
    #include <systemc>
    #include "sysc/communication/sc_communication_ids.h"
    namespace pysystemc_event_finder_test {
    using wide_if = sc_core::sc_signal_in_if<sc_dt::sc_biguint<137>>;
    /** @brief Expose protected finder construction/reporting for this reference. */
    class ReportProbe : public sc_core::sc_event_finder_t<wide_if> {
    public:
      static int destroyed;
      explicit ReportProbe(const sc_core::sc_port_base& port)
        : sc_event_finder_t<wide_if>(port, &wide_if::value_changed_event) {}
      ~ReportProbe() override { ++destroyed; }
      void emit_report(const char* add_msg = 0) const { report_error(sc_core::SC_ID_FIND_EVENT_, add_msg); }
    };
    int ReportProbe::destroyed = 0;

    /** @brief Define overloaded const event and scalar methods for selector tests. */
    class OverloadedReadyIf : public virtual sc_core::sc_interface {
    public:
      OverloadedReadyIf() : ready_event_() {}
      const sc_core::sc_event& ready() const { return ready_event_; }
      int ready(int value) const { return value + 1; }
    private:
      sc_core::sc_event ready_event_;
    };

    /** @brief Inherit the overload set for derived-interface selector tests. */
    class DerivedReadyIf : public OverloadedReadyIf {};
    }
    """
)


def report_reference(label, finder, interface=None):
    """Print the native report fields produced by one invalid event lookup.

    Args:
        label: Stable label shared with the native reference.
        finder: Event finder whose lookup should report an error.
        interface: Optional explicit interface passed to ``find_event``.

    Returns:
        True when SystemC raises the expected unbound or incompatible-interface report.
    """
    try:
        finder.find_event() if interface is None else finder.find_event(interface)
    except sc.sc_report as report:
        message = report.get_msg()
        print("FINDER_REPORT\t" + "\t".join((label, str(report.get_id()), report.get_msg_type(), message)))
        return (
            report.get_id() == 118
            and report.get_msg_type() == "find event failed"
            and message.startswith("port is not bound: port '")
        )
    return False


def probe_report(label, probe, add_message, expected_message):
    """Capture a derived probe's public call to protected native report_error.

    Args:
        label: Stable reference label.
        probe: Python-owned native derived finder.
        add_message: Optional report prefix; an empty value selects the native default.
        expected_message: Expected native report message text.

    Returns:
        True when all public native report fields match the expected report.
    """
    try:
        probe.emit_report() if add_message is None else probe.emit_report(add_message)
    except sc.sc_report as report:
        message = report.get_msg()
        fields = (label, str(report.get_id()), report.get_msg_type(), message)
        print("FINDER_PROBE_REPORT\t" + "\t".join(fields))
        return report.get_id() == 118 and report.get_msg_type() == "find event failed" and message == expected_message
    return False


wide_type = sc_dt.sc_biguint[137]
wide_if = sc.sc_signal_in_if[wide_type]
finder_type = sc.sc_event_finder_t[wide_if]
module_name = sc.sc_module_name("finder")
module = sc.sc_module(module_name)
wide_port = sc.sc_in[wide_type]("wide")
unbound_port = sc.sc_in[wide_type]("unbound")
source = sc.sc_signal[wide_type]("source")
alternate = sc.sc_signal[wide_type]("alternate")
incompatible = sc.sc_signal[int]("incompatible")
wide_port.bind(source)

cached_first = wide_port.value_changed()
cached_second = wide_port.value_changed()
cached_identity = cppyy.addressof(cached_first) == cppyy.addressof(cached_second)
port_identity = cppyy.addressof(cached_first.port()) == cppyy.addressof(wide_port)
bound_lookup = cppyy.addressof(cached_first.find_event()) == cppyy.addressof(source.value_changed_event())
explicit_interface = cppyy.bind_object(cppyy.addressof(alternate), sc.sc_interface)
explicit_lookup = cppyy.addressof(cached_first.find_event(explicit_interface)) == cppyy.addressof(
    alternate.value_changed_event()
)

wide_cache = sc.sc_event_finder_cache()
wide_cache_empty = cppyy.addressof(wide_cache.pointer) == 0
wide_cached_create = sc.sc_event_finder.cached_create[wide_if]
wide_cached_first = wide_cached_create(wide_cache, wide_port, wide_if.value_changed_event)
wide_cached_second = wide_cached_create(wide_cache, wide_port, wide_if.value_changed_event)
wide_cached_identity = cppyy.addressof(wide_cached_first) == cppyy.addressof(wide_cached_second)
wide_cached_pointer = cppyy.addressof(wide_cache.pointer) == cppyy.addressof(wide_cached_first)
wide_cached_lookup = cppyy.addressof(wide_cached_first.find_event()) == cppyy.addressof(source.value_changed_event())

# Exercise the public templated constructor and its owned proxy destructor. The
# member-function pointer is the native IF method used by the standard port API.
direct_finder = finder_type(wide_port, wide_if.value_changed_event)
direct_lookup = (
    cppyy.addressof(direct_finder.find_event()) == cppyy.addressof(source.value_changed_event())
    and cppyy.addressof(direct_finder.find_event(explicit_interface))
    == cppyy.addressof(alternate.value_changed_event())
    and cppyy.addressof(direct_finder.port()) == cppyy.addressof(wide_port)
)
direct_type = isinstance(direct_finder, finder_type)
direct_owned = bool(direct_finder.__python_owns__)
assert not hasattr(direct_finder, "report_error")
direct_finder.__destruct__()

overloaded_if = cppyy.gbl.pysystemc_event_finder_test.OverloadedReadyIf
overloaded_interface = overloaded_if()
overloaded_port = sc.sc_port[overloaded_if]("overloaded")
overloaded_port.bind(overloaded_interface)
overloaded_method = overloaded_if.ready
overloaded_constructor = sc.sc_event_finder_t[overloaded_if]
overloaded_name_constructor = sc.sc_event_finder_t[overloaded_if.__cpp_name__]
overloaded_type_spelling_match = overloaded_constructor is overloaded_name_constructor
overloaded_direct = overloaded_constructor(overloaded_port, overloaded_method)
overloaded_direct_match = cppyy.addressof(overloaded_direct.find_event()) == cppyy.addressof(
    overloaded_interface.ready()
)
overloaded_direct_lifecycle = bool(overloaded_direct.__python_owns__)
overloaded_direct.__destruct__()
overloaded_cache = sc.sc_event_finder_cache()
overloaded_cached_create = sc.sc_event_finder.cached_create[overloaded_if]
overloaded_cached_first = overloaded_cached_create(overloaded_cache, overloaded_port, overloaded_method)
overloaded_cached_second = sc.sc_event_finder.cached_create[overloaded_if.__cpp_name__](
    overloaded_cache, overloaded_port, overloaded_method
)
overloaded_cache_match = cppyy.addressof(overloaded_cached_first) == cppyy.addressof(
    overloaded_cached_second
) and cppyy.addressof(overloaded_cached_first.find_event()) == cppyy.addressof(overloaded_interface.ready())
overloaded_scalar_overload = overloaded_interface.ready(19) == 20

derived_ready_if = cppyy.gbl.pysystemc_event_finder_test.DerivedReadyIf
derived_ready_interface = derived_ready_if()
derived_ready_port = sc.sc_port[derived_ready_if]("derived_ready")
derived_ready_port.bind(derived_ready_interface)
inherited_method = derived_ready_if.ready
inherited_direct = sc.sc_event_finder_t[derived_ready_if](derived_ready_port, inherited_method)
inherited_direct_match = cppyy.addressof(inherited_direct.find_event()) == cppyy.addressof(
    derived_ready_interface.ready()
)
inherited_direct_lifecycle = bool(inherited_direct.__python_owns__)
inherited_direct.__destruct__()
inherited_cache = sc.sc_event_finder_cache()
inherited_cached_create = sc.sc_event_finder.cached_create[derived_ready_if]
inherited_cached_first = inherited_cached_create(inherited_cache, derived_ready_port, inherited_method)
inherited_cached_second = inherited_cached_create(inherited_cache, derived_ready_port, inherited_method)
inherited_cache_match = cppyy.addressof(inherited_cached_first) == cppyy.addressof(
    inherited_cached_second
) and cppyy.addressof(inherited_cached_first.find_event()) == cppyy.addressof(derived_ready_interface.ready())
inherited_scalar_overload = derived_ready_interface.ready(20) == 21

try:
    finder_type(wide_port, sc.sc_signal_in_if[bool].value_changed_event)
except TypeError:
    invalid_method_owner_rejected = True
else:
    invalid_method_owner_rejected = False

try:
    finder_type(wide_port, wide_if.read)
except TypeError:
    invalid_event_method_rejected = True
else:
    invalid_event_method_rejected = False

bool_type = bool
bool_port = sc.sc_in[bool_type]("bool")
bool_source = sc.sc_signal[bool_type]("bool_source")
bool_port.bind(bool_source)
bool_if = sc.sc_signal_in_if[bool_type]
bool_cache = sc.sc_event_finder_cache()
bool_cached_create = sc.sc_event_finder.cached_create[bool_if]
bool_cached_first = bool_cached_create(bool_cache, bool_port, bool_if.value_changed_event)
bool_cached_second = bool_cached_create(bool_cache, bool_port, bool_if.posedge_event)
bool_cached_identity = cppyy.addressof(bool_cached_first) == cppyy.addressof(bool_cached_second)
bool_first_method = cppyy.addressof(bool_cached_first.find_event()) == cppyy.addressof(
    bool_source.value_changed_event()
)
logic_type = sc_dt.sc_logic
logic_port = sc.sc_in[logic_type]("logic")
logic_source = sc.sc_signal[logic_type]("logic_source")
logic_port.bind(logic_source)
logic_if = sc.sc_signal_in_if[logic_type]
logic_cache = sc.sc_event_finder_cache()
logic_cached_create = sc.sc_event_finder.cached_create[logic_if]
logic_cached_first = logic_cached_create(logic_cache, logic_port, logic_if.value_changed_event)
logic_cached_second = logic_cached_create(logic_cache, logic_port, logic_if.value_changed_event)
logic_cached_identity = cppyy.addressof(logic_cached_first) == cppyy.addressof(logic_cached_second)
logic_cached_lookup = cppyy.addressof(logic_cached_first.find_event()) == cppyy.addressof(
    logic_source.value_changed_event()
)
bool_typedef_lookup = cppyy.addressof(bool_port.pos().find_event()) == cppyy.addressof(bool_source.posedge_event())
logic_typedef_lookup = cppyy.addressof(logic_port.pos().find_event()) == cppyy.addressof(logic_source.posedge_event())

bool_finder_type = sc.sc_event_finder_t[bool_if]
bool_direct_finder = bool_finder_type(bool_port, bool_if.posedge_event)
bool_direct = cppyy.addressof(bool_direct_finder.find_event()) == cppyy.addressof(bool_source.posedge_event())
bool_direct_lifecycle = isinstance(bool_direct_finder, bool_finder_type) and bool(bool_direct_finder.__python_owns__)
bool_direct_finder.__destruct__()
logic_finder_type = sc.sc_event_finder_t[logic_if]
logic_direct_finder = logic_finder_type(logic_port, logic_if.posedge_event)
logic_direct = cppyy.addressof(logic_direct_finder.find_event()) == cppyy.addressof(logic_source.posedge_event())
logic_direct_lifecycle = isinstance(logic_direct_finder, logic_finder_type) and bool(
    logic_direct_finder.__python_owns__
)
logic_direct_finder.__destruct__()

incompatible_interface = cppyy.bind_object(cppyy.addressof(incompatible), sc.sc_interface)
incompatible_report = report_reference("incompatible", cached_first, incompatible_interface)
unbound_finder = unbound_port.value_changed()
unbound_report = report_reference("unbound", unbound_finder)

previous_error_actions = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_DO_NOTHING)
try:
    incompatible_continuation = cppyy.addressof(cached_first.find_event(incompatible_interface)) == cppyy.addressof(
        sc.sc_event.none
    )
    unbound_continuation = cppyy.addressof(unbound_finder.find_event()) == cppyy.addressof(sc.sc_event.none)
finally:
    sc.sc_report_handler.set_actions(sc.SC_ERROR, previous_error_actions)
assert incompatible_continuation and unbound_continuation
print("FINDER_CONTINUATION\t1\t1")

result = (
    wide_cache_empty,
    wide_cached_identity,
    wide_cached_pointer,
    wide_cached_lookup,
    bool_cached_identity,
    bool_first_method,
    logic_cached_identity,
    logic_cached_lookup,
    cached_identity,
    port_identity,
    bound_lookup,
    explicit_lookup,
    direct_lookup,
    bool_typedef_lookup,
    logic_typedef_lookup,
    incompatible_report,
    unbound_report,
)
assert all(result), result
print("FINDER_CACHE_REFERENCE\t" + "\t".join(str(int(value)) for value in result[:8]))
print("FINDER_REFERENCE\t137\t" + "\t".join(str(int(value)) for value in result[8:]))
assert direct_type and direct_owned
assert invalid_method_owner_rejected and invalid_event_method_rejected
print(
    "FINDER_FACTORY_CONTRACT\t"
    + "\t".join(
        str(int(value))
        for value in (direct_type, direct_owned, invalid_method_owner_rejected, invalid_event_method_rejected)
    )
)
assert bool_direct and bool_direct_lifecycle and logic_direct and logic_direct_lifecycle
print(
    "FINDER_SPECIALIZATION_REFERENCE\t"
    + "\t".join(str(int(value)) for value in (bool_direct, bool_direct_lifecycle, logic_direct, logic_direct_lifecycle))
)
overloaded_results = (
    overloaded_type_spelling_match,
    overloaded_direct_match,
    overloaded_direct_lifecycle,
    overloaded_cache_match,
    overloaded_scalar_overload,
    inherited_direct_match,
    inherited_direct_lifecycle,
    inherited_cache_match,
    inherited_scalar_overload,
)
assert all(overloaded_results), overloaded_results
print("FINDER_OVERLOADED_METHODS\t" + "\t".join(str(int(value)) for value in overloaded_results))

probe_type = cppyy.gbl.pysystemc_event_finder_test.ReportProbe
probe = probe_type(wide_port)
probe_owned = bool(probe.__python_owns__)
default_probe_message = "port 'finder.wide' (sc_in)"
added_probe_message = "extra context: " + default_probe_message
default_probe_report = probe_report("default", probe, None, default_probe_message)
added_probe_report = probe_report("added", probe, "extra context", added_probe_message)
probe.__destruct__()
probe_destroyed = int(probe_type.destroyed) == 1
assert probe_owned and default_probe_report and added_probe_report and probe_destroyed
print("FINDER_PROBE_LIFETIME\t" + "\t".join(str(int(value)) for value in (probe_owned, probe_destroyed)))

# Cached references do not own the finder; release them before destroying each
# cache, which is the native owner of the cached finder allocation.
del wide_cached_first, wide_cached_second
del bool_cached_first, bool_cached_second
del logic_cached_first, logic_cached_second
del overloaded_cached_first, overloaded_cached_second
del inherited_cached_first, inherited_cached_second
wide_cache.__destruct__()
bool_cache.__destruct__()
logic_cache.__destruct__()
overloaded_cache.__destruct__()
inherited_cache.__destruct__()
