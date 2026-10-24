/** @file Test-only export probes for protected hooks and const overloads. */
#pragma once
#include <sstream>
#include <systemc>
namespace export_probe
{
using IF = sc_core::sc_signal_in_if<sc_dt::sc_biguint<137>>;
/** Expose base hooks without changing native export lifecycle or ownership. */
struct Probe : sc_core::sc_export<IF>
{
  static int destroyed;
  std::string hooks;
  Probe() : sc_export<IF>()
  {
  }
  explicit Probe(const char* name) : sc_export<IF>(name)
  {
  }
  ~Probe() override
  {
    ++destroyed;
  }
  void before_end_of_elaboration() override
  {
    sc_core::sc_export_base::before_end_of_elaboration();
    hooks += "B";
  }
  void end_of_elaboration() override
  {
    sc_core::sc_export_base::end_of_elaboration();
    hooks += "E";
  }
  void start_of_simulation() override
  {
    sc_core::sc_export_base::start_of_simulation();
    hooks += "S";
  }
  void end_of_simulation() override
  {
    sc_core::sc_export_base::end_of_simulation();
    hooks += "F";
  }
  void error(bool extra) const
  {
    report_error("export-probe", extra ? "detail" : nullptr);
  }
};
int Probe::destroyed = 0;
/** Check both const and mutable export/base overloads on borrowed objects. */
std::string access(sc_core::sc_export<IF>& e)
{
  const auto& c = e;
  sc_core::sc_export_base& b = e;
  const sc_core::sc_export_base& cb = e;
  std::ostringstream out;
  out << (e.get_interface() == c.get_interface()) << ' ' << (b.get_interface() == cb.get_interface()) << ' '
      << (e.operator->() == c.operator->()) << ' ' << (&static_cast<IF&>(e) == &static_cast<const IF&>(c)) << ' '
      << (e.get_interface_type() == b.get_interface_type()) << ' '
      << (cb.get_interface_type() == sc_core::sc_type_index(typeid(IF))) << ' ' << c->read();
  return out.str();
}
}  // namespace export_probe
