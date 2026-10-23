/** @file Independent native export binding and lifecycle reference. */
#include <iostream>

#include "sc_core_exports_probe.h"
/** Provide a public native module constructor for the reference. */
struct ExportTop : sc_core::sc_module
{
  explicit ExportTop(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};
int sc_main(int, char**)
{
  auto* name = new sc_core::sc_module_name("top");
  ExportTop top(*name);
  sc_core::sc_signal<sc_dt::sc_biguint<137>> signal("signal");
  sc_core::sc_export<export_probe::IF> a("a"), b;
  auto* probe = new export_probe::Probe("probe");
  auto* default_probe = new export_probe::Probe;
  a.bind(signal);
  b(signal);
  probe->bind(signal);
  default_probe->bind(signal);
  auto error = [](const char* label, auto action)
  {
    try
    {
      action();
    }
    catch (const sc_core::sc_report& r)
    {
      std::cout << "EXPORT_ERROR\t" << label << '\t' << r.get_msg_type() << '\t' << r.get_msg() << '\n';
    }
  };
  error("duplicate", [&] { a.bind(signal); });
  error("probe", [&] { probe->error(true); });
  error("plain", [&] { probe->error(false); });
  delete name;
  signal.write(sc_dt::sc_biguint<137>("0x10000000000000000000000000000000001"));
  sc_core::sc_start(1, sc_core::SC_NS);
  std::cout << "EXPORT_ACCESS\t" << export_probe::access(a) << '\n';
  sc_core::sc_stop();
  std::cout << "EXPORT_HOOKS\t" << probe->hooks << '\t' << default_probe->hooks << '\n';
  delete probe;
  delete default_probe;
  std::cout << "EXPORT_DESTRUCTION\t" << export_probe::Probe::destroyed << '\n';
  return 0;
}
