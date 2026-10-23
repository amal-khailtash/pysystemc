/** @file sc_core_event_finders.cpp
 * @brief Compare event finder construction, caching, lookup, and reports.
 *
 * The finder borrows its port and interface. Keep both alive for every lookup;
 * the explicitly allocated finder is deleted before its port is destroyed.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sysc/communication/sc_communication_ids.h"

namespace
{
using wide_value = sc_dt::sc_biguint<137>;
using wide_if = sc_core::sc_signal_in_if<wide_value>;
using wide_finder = sc_core::sc_event_finder_t<wide_if>;

/** @brief Native interface with an event/scalar overload set. */
class overloaded_ready_if : public virtual sc_core::sc_interface
{
 public:
  /** @brief Return the event selected by the zero-argument overload. */
  const sc_core::sc_event& ready() const
  {
    return ready_event_;
  }

  /** @brief Return a scalar result from the overloaded non-event method. */
  int ready(int value) const
  {
    return value + 1;
  }

 private:
  sc_core::sc_event ready_event_;
};

/** @brief Inherit both ready overloads for derived interface selection. */
class derived_ready_if : public overloaded_ready_if
{
};

using overloaded_ready_alias = overloaded_ready_if;
static_assert(std::is_same<overloaded_ready_if, overloaded_ready_alias>::value,
              "qualified type spelling must resolve to the same interface");

static_assert(std::is_base_of<sc_core::sc_event_finder, wide_finder>::value,
              "sc_event_finder_t must derive from sc_event_finder");

/** @brief Observe virtual destruction through the public finder base.
 *
 * The finder keeps a borrowed port reference and a native member pointer. The
 * counter has process lifetime and does not retain either object.
 */
class tracked_finder : public wide_finder
{
 public:
  static int destroyed;

  /** @brief Construct a concrete finder using the native interface method. */
  explicit tracked_finder(const sc_core::sc_port_base& port) : wide_finder(port, &wide_if::value_changed_event)
  {
  }

  /** @brief Count deletion through the virtual base destructor. */
  ~tracked_finder() override
  {
    ++destroyed;
  }
};

int tracked_finder::destroyed = 0;

/** @brief Reach the protected base constructor and report helper for comparison. */
class report_probe : public wide_finder
{
 public:
  static int destroyed;

  /** @brief Construct through the protected base finder constructor. */
  explicit report_probe(const sc_core::sc_port_base& port) : wide_finder(port, &wide_if::value_changed_event)
  {
  }

  /** @brief Call the protected native report helper with its default argument. */
  void emit_report(const char* add_msg = 0) const
  {
    report_error(sc_core::SC_ID_FIND_EVENT_, add_msg);
  }

  /** @brief Count deletion of the probe instance. */
  ~report_probe() override
  {
    ++destroyed;
  }
};

int report_probe::destroyed = 0;

/** @brief Capture the exact identifier and message from a failed lookup. */
template <typename Lookup>
bool report_reference(const char* label, Lookup&& lookup)
{
  try
  {
    lookup();
  }
  catch (const sc_core::sc_report& report)
  {
    const std::string message = report.get_msg();
    std::cout << "FINDER_REPORT\t" << label << '\t' << report.get_id() << '\t' << report.get_msg_type() << '\t'
              << message << "\n";
    return report.get_id() == 118 && std::string(report.get_msg_type()) == "find event failed" &&
           message.find("port is not bound: port '") == 0;
  }
  catch (...)
  {
    return false;
  }
  return false;
}

/** @brief Compare one call through the derived probe's public report wrapper. */
template <typename Lookup>
bool probe_report_reference(const char* label, Lookup&& lookup, const std::string& expected_message)
{
  try
  {
    lookup();
  }
  catch (const sc_core::sc_report& report)
  {
    const std::string message = report.get_msg();
    std::cout << "FINDER_PROBE_REPORT\t" << label << '\t' << report.get_id() << '\t' << report.get_msg_type() << '\t'
              << message << "\n";
    return report.get_id() == 118 && std::string(report.get_msg_type()) == "find event failed" &&
           message == expected_message;
  }
  catch (...)
  {
    return false;
  }
  return false;
}

/** @brief Own compatible, incompatible, and unbound test ports and channels. */
struct finder_model : sc_core::sc_module
{
  sc_core::sc_in<wide_value> wide_port;
  sc_core::sc_in<wide_value> unbound_port;
  sc_core::sc_signal<wide_value> source;
  sc_core::sc_signal<wide_value> alternate;
  sc_core::sc_signal<int> incompatible;
  sc_core::sc_in<bool> bool_port;
  sc_core::sc_signal<bool> bool_source;
  sc_core::sc_in<sc_dt::sc_logic> logic_port;
  sc_core::sc_signal<sc_dt::sc_logic> logic_source;
  sc_core::sc_port<overloaded_ready_if> overloaded_port;
  overloaded_ready_if overloaded_source;
  sc_core::sc_port<derived_ready_if> derived_ready_port;
  derived_ready_if derived_ready_source;

  /** @brief Bind compatible interfaces while retaining one intentionally open port. */
  explicit finder_model(sc_core::sc_module_name name)
    : sc_module(name)
    , wide_port("wide")
    , unbound_port("unbound")
    , source("source")
    , alternate("alternate")
    , incompatible("incompatible")
    , bool_port("bool")
    , bool_source("bool_source")
    , logic_port("logic")
    , logic_source("logic_source")
    , overloaded_port("overloaded")
    , overloaded_source()
    , derived_ready_port("derived_ready")
    , derived_ready_source()
  {
    wide_port.bind(source);
    bool_port.bind(bool_source);
    logic_port.bind(logic_source);
    overloaded_port.bind(overloaded_source);
    derived_ready_port.bind(derived_ready_source);
  }
};
}  // namespace

/** @brief Check native template and inherited API behavior without simulation. */
int sc_main(int, char**)
{
  using namespace sc_core;
  finder_model model("finder");
  sc_event_finder& cached_first = model.wide_port.value_changed();
  sc_event_finder& cached_second = model.wide_port.value_changed();
  const bool cached_identity = &cached_first == &cached_second;
  const bool port_identity = &cached_first.port() == &model.wide_port;
  const bool bound_lookup = &cached_first.find_event() == &model.source.value_changed_event();
  const bool explicit_lookup = &cached_first.find_event(&model.alternate) == &model.alternate.value_changed_event();

  bool direct_lookup = false;
  {
    wide_finder direct(model.wide_port, &wide_if::value_changed_event);
    direct_lookup = &direct.find_event() == &model.source.value_changed_event() &&
                    &direct.find_event(&model.alternate) == &model.alternate.value_changed_event() &&
                    &direct.port() == &model.wide_port;
  }

  sc_event_finder* generic_cache = nullptr;
  const bool generic_cache_empty = generic_cache == nullptr;
  sc_event_finder& generic_first =
    sc_event_finder::cached_create(generic_cache, model.wide_port, &wide_if::value_changed_event);
  sc_event_finder& generic_second =
    sc_event_finder::cached_create(generic_cache, model.wide_port, &wide_if::value_changed_event);
  const bool generic_cache_pointer = generic_cache == &generic_first;
  const bool generic_cache_lookup = &generic_first.find_event() == &model.source.value_changed_event();
  const bool generic_cache_identity = &generic_first == &generic_second && generic_cache_lookup;
  delete generic_cache;
  sc_event_finder* bool_cache = nullptr;
  sc_event_finder& bool_first =
    sc_event_finder::cached_create(bool_cache, model.bool_port, &sc_signal_in_if<bool>::value_changed_event);
  sc_event_finder& bool_second =
    sc_event_finder::cached_create(bool_cache, model.bool_port, &sc_signal_in_if<bool>::posedge_event);
  const bool bool_cache_identity = &bool_first == &bool_second;
  const bool bool_first_method = &bool_first.find_event() == &model.bool_source.value_changed_event();
  delete bool_cache;
  sc_event_finder* logic_cache = nullptr;
  sc_event_finder& logic_first = sc_event_finder::cached_create(logic_cache, model.logic_port,
                                                                &sc_signal_in_if<sc_dt::sc_logic>::value_changed_event);
  sc_event_finder& logic_second = sc_event_finder::cached_create(
    logic_cache, model.logic_port, &sc_signal_in_if<sc_dt::sc_logic>::value_changed_event);
  const bool logic_cache_identity = &logic_first == &logic_second;
  const bool logic_cache_lookup = &logic_first.find_event() == &model.logic_source.value_changed_event();
  delete logic_cache;

  sc_event_finder* polymorphic = new tracked_finder(model.wide_port);
  const bool polymorphic_lookup = &polymorphic->find_event() == &model.source.value_changed_event();
  delete polymorphic;
  const bool virtual_destructor = tracked_finder::destroyed == 1;

  const bool bool_typedef_lookup = &model.bool_port.pos().find_event() == &model.bool_source.posedge_event();
  const bool logic_typedef_lookup = &model.logic_port.pos().find_event() == &model.logic_source.posedge_event();
  bool bool_direct_lookup = false;
  {
    using bool_if = sc_signal_in_if<bool>;
    sc_event_finder_t<bool_if> direct(model.bool_port, &bool_if::posedge_event);
    bool_direct_lookup = &direct.find_event() == &model.bool_source.posedge_event();
  }
  bool logic_direct_lookup = false;
  {
    using logic_if = sc_signal_in_if<sc_dt::sc_logic>;
    sc_event_finder_t<logic_if> direct(model.logic_port, &logic_if::posedge_event);
    logic_direct_lookup = &direct.find_event() == &model.logic_source.posedge_event();
  }
  const bool incompatible_report =
    report_reference("incompatible", [&] { cached_first.find_event(&model.incompatible); });
  const bool unbound_report = report_reference("unbound", [&] { model.unbound_port.value_changed().find_event(); });

  using overloaded_method_type = const sc_event& (overloaded_ready_if::*)() const;
  const overloaded_method_type overloaded_method = static_cast<overloaded_method_type>(&overloaded_ready_if::ready);
  bool overloaded_direct_match = false;
  {
    sc_event_finder_t<overloaded_ready_if> direct(model.overloaded_port, overloaded_method);
    overloaded_direct_match = &direct.find_event() == &model.overloaded_source.ready();
  }
  sc_event_finder* overloaded_cache = nullptr;
  sc_event_finder& overloaded_cached_first =
    sc_event_finder::cached_create(overloaded_cache, model.overloaded_port, overloaded_method);
  sc_event_finder& overloaded_cached_second =
    sc_event_finder::cached_create(overloaded_cache, model.overloaded_port, overloaded_method);
  const bool overloaded_cache_match = &overloaded_cached_first == &overloaded_cached_second &&
                                      &overloaded_cached_first.find_event() == &model.overloaded_source.ready();
  delete overloaded_cache;
  const bool overloaded_scalar_method = model.overloaded_source.ready(19) == 20;

  using inherited_method_type = const sc_event& (derived_ready_if::*)() const;
  const inherited_method_type inherited_method = static_cast<inherited_method_type>(&derived_ready_if::ready);
  bool inherited_direct_match = false;
  {
    sc_event_finder_t<derived_ready_if> direct(model.derived_ready_port, inherited_method);
    inherited_direct_match = &direct.find_event() == &model.derived_ready_source.ready();
  }
  sc_event_finder* inherited_cache = nullptr;
  sc_event_finder& inherited_cached_first =
    sc_event_finder::cached_create(inherited_cache, model.derived_ready_port, inherited_method);
  sc_event_finder& inherited_cached_second =
    sc_event_finder::cached_create(inherited_cache, model.derived_ready_port, inherited_method);
  const bool inherited_cache_match = &inherited_cached_first == &inherited_cached_second &&
                                     &inherited_cached_first.find_event() == &model.derived_ready_source.ready();
  delete inherited_cache;
  const bool inherited_scalar_method = model.derived_ready_source.ready(20) == 21;

  const sc_actions previous_error_actions = sc_report_handler::set_actions(SC_ERROR, SC_DO_NOTHING);
  const bool incompatible_continuation = &cached_first.find_event(&model.incompatible) == &sc_event::none;
  const bool unbound_continuation = &model.unbound_port.value_changed().find_event() == &sc_event::none;
  sc_report_handler::set_actions(SC_ERROR, previous_error_actions);

  bool default_probe_report = false;
  bool added_probe_report = false;
  {
    report_probe probe(model.wide_port);
    const std::string default_message = "port 'finder.wide' (sc_in)";
    default_probe_report = probe_report_reference(
      "default", [&] { probe.emit_report(); }, default_message);
    added_probe_report = probe_report_reference(
      "added", [&] { probe.emit_report("extra context"); }, "extra context: " + default_message);
  }
  const bool probe_destructor = report_probe::destroyed == 1;

  std::cout << "FINDER_REFERENCE\t137\t" << cached_identity << '\t' << port_identity << '\t' << bound_lookup << '\t'
            << explicit_lookup << '\t' << direct_lookup << '\t' << bool_typedef_lookup << '\t' << logic_typedef_lookup
            << '\t' << incompatible_report << '\t' << unbound_report << "\n";
  std::cout << "FINDER_CACHE_REFERENCE\t" << generic_cache_empty << '\t' << generic_cache_identity << '\t'
            << generic_cache_pointer << '\t' << generic_cache_lookup << '\t' << bool_cache_identity << '\t'
            << bool_first_method << '\t' << logic_cache_identity << '\t' << logic_cache_lookup << "\n";
  std::cout << "FINDER_CONTINUATION\t" << incompatible_continuation << '\t' << unbound_continuation << "\n";
  std::cout << "FINDER_NATIVE_LIFETIME\t" << polymorphic_lookup << '\t' << virtual_destructor << "\n";
  std::cout << "FINDER_PROBE_LIFETIME\t1\t" << probe_destructor << "\n";
  std::cout << "FINDER_SPECIALIZATION_REFERENCE\t" << bool_direct_lookup << "\t1\t" << logic_direct_lookup << "\t1\n";
  std::cout << "FINDER_OVERLOADED_METHODS\t1\t" << overloaded_direct_match << "\t1\t" << overloaded_cache_match << '\t'
            << overloaded_scalar_method << '\t' << inherited_direct_match << "\t1\t" << inherited_cache_match << '\t'
            << inherited_scalar_method << "\n";
  return generic_cache_empty && generic_cache_identity && bool_cache_identity && bool_first_method &&
             logic_cache_identity && logic_cache_lookup && default_probe_report && added_probe_report &&
             probe_destructor && bool_direct_lookup && logic_direct_lookup && incompatible_continuation &&
             unbound_continuation && cached_identity && port_identity && bound_lookup && explicit_lookup &&
             direct_lookup && generic_cache_identity && polymorphic_lookup && virtual_destructor &&
             bool_typedef_lookup && logic_typedef_lookup && incompatible_report && unbound_report &&
             overloaded_direct_match && overloaded_cache_match && overloaded_scalar_method && inherited_direct_match &&
             inherited_cache_match && inherited_scalar_method
           ? 0
           : 1;
}
