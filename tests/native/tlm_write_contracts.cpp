/** @file tlm_write_contracts.cpp
 * @brief Independent native plain/delayed write value and time reference.
 * @details Channels own copied samples; ports borrow channels. No GIL callbacks.
 */
#include <systemc>
#include <tlm>
using value_type = sc_dt::sc_biguint<137>;
/** @brief Implement native write-only interface with copied storage. */
struct plain : sc_core::sc_module, tlm::tlm_write_if<value_type>
{
  value_type value;
  plain(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  void write(const value_type& input) override
  {
    value = input;
  }
};
/** @brief Implement native delayed write preserving the time argument. */
template <class Interface>
struct delayed : sc_core::sc_module, Interface
{
  value_type value;
  sc_core::sc_time time;
  delayed(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  void write(const value_type& input, const sc_core::sc_time& delay) override
  {
    value = input;
    time = delay;
  }
};
/** @brief Compare copied wide input through all three native declarations. */
int sc_main(int, char**)
{
  plain a("plain");
  delayed<tlm::tlm_delayed_write_if<value_type>> b("delayed");
  delayed<tlm::tlm_delayed_analysis_if<value_type>> c("analysis");
  value_type sample = 17;
  sample[136] = true;
  a.write(sample);
  b.write(sample, sc_core::sc_time(2, sc_core::SC_NS));
  c.write(sample, sc_core::sc_time(2, sc_core::SC_NS));
  sample[136] = false;
  if (!a.value[136] || !b.value[136] || !c.value[136]) return 2;
  std::cout << "TLM_WRITE_REFERENCE plain 17 1 none\n";
  std::cout << "TLM_WRITE_REFERENCE delayed 17 1 " << b.time << "\n";
  std::cout << "TLM_WRITE_REFERENCE delayed 17 1 " << c.time << "\n";
  sc_core::sc_start(1, sc_core::SC_NS);
  sc_core::sc_stop();
  return 0;
}
