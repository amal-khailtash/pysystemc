/** @file Native unbound export arrow and elaboration errors. */
#include <iostream>
#include <systemc>
/** Provide a public native module constructor for the reference. */
struct ExportTop : sc_core::sc_module
{
  explicit ExportTop(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};
int sc_main(int, char**)
{
  {
    auto* name = new sc_core::sc_module_name("top");
    ExportTop top(*name);
    sc_core::sc_export<sc_core::sc_signal_in_if<int>> empty("empty");
    try
    {
      empty.operator->();
    }
    catch (const sc_core::sc_report& r)
    {
      std::cout << "EXPORT_UNBOUND\t" << r.get_msg_type() << '\t' << r.get_msg() << '\n';
    }
    delete name;
    try
    {
      sc_core::sc_start(sc_core::SC_ZERO_TIME);
    }
    catch (const sc_core::sc_report& r)
    {
      std::cout << "EXPORT_ELABORATION\t" << r.get_msg_type() << '\t' << r.get_msg() << '\n';
    }
  }
  return 0;
}
