#include <stdexcept>
#include <systemc>

/** @brief Native reference model; top owns storage and events throughout simulation.
 * @details No Python/GIL interaction. Identity and timing failures throw to the runner.
 */
struct top : sc_core::sc_module
{
  sc_dt::sc_biguint<137> value;
  sc_core::sc_event changed;
  int calls = 0;
  SC_HAS_PROCESS(top);
  explicit top(sc_core::sc_module_name name) : sc_module(name), value(17)
  {
    SC_METHOD(observe);
    sensitive << changed;
    dont_initialize();
  }
  const sc_dt::sc_biguint<137>& read() const
  {
    return value;
  }
  const sc_core::sc_event& event() const
  {
    return changed;
  }
  void observe()
  {
    ++calls;
  }
};

/** @brief Assert borrowed identity, wide mutation and event timing in a fresh kernel. */
int sc_main(int, char**)
{
  top model("top");
  if (&model.read() != &model.value || &model.event() != &model.changed) throw std::runtime_error("reference identity");
  model.value[136] = true;
  if (!model.read()[136]) throw std::runtime_error("wide mutation");
  model.value = 29;
  model.changed.notify(sc_core::sc_time(2, sc_core::SC_NS));
  sc_core::sc_start(3, sc_core::SC_NS);
  if (model.calls != 1 || model.read() != 29 || sc_core::sc_time_stamp() != sc_core::sc_time(3, sc_core::SC_NS))
    throw std::runtime_error("value/event/time");
  std::cout << "REFERENCE_RESULT 29 1 " << sc_core::sc_time_stamp() << '\n';
  return 0;
}
