#include <stdexcept>
#include <systemc>

/** @brief Native FIFO reference owns channel storage; no Python/GIL callbacks. */
struct top : sc_core::sc_module
{
  sc_core::sc_fifo<sc_dt::sc_biguint<137>> fifo;
  int calls = 0;
  SC_HAS_PROCESS(top);
  explicit top(sc_core::sc_module_name name) : sc_module(name), fifo("fifo", 2)
  {
    SC_THREAD(consume);
    SC_THREAD(produce);
  }
  /** @brief Verify both blocking read overloads and timed wide values. */
  void consume()
  {
    sc_dt::sc_biguint<137> value;
    fifo.read(value);
    if (!value[136] || value.to_uint() != 17 || sc_core::sc_time_stamp() != sc_core::sc_time(2, sc_core::SC_NS))
      throw std::runtime_error("first read");
    ++calls;
    value = fifo.read();
    if (value != 29 || sc_core::sc_time_stamp() != sc_core::sc_time(3, sc_core::SC_NS))
      throw std::runtime_error("second read");
    ++calls;
  }
  /** @brief Produce values through blocking/nonblocking write paths. */
  void produce()
  {
    sc_core::wait(2, sc_core::SC_NS);
    sc_dt::sc_biguint<137> value = 17;
    value[136] = true;
    fifo.write(value);
    sc_core::wait(1, sc_core::SC_NS);
    if (!fifo.nb_write(29)) throw std::runtime_error("write");
  }
};

/** @brief Assert failed output preservation and final native state. */
int sc_main(int, char**)
{
  top model("top");
  sc_dt::sc_biguint<137> value = 73;
  if (model.fifo.nb_read(value) || value != 73) throw std::runtime_error("failed output");
  sc_core::sc_start(4, sc_core::SC_NS);
  if (model.calls != 2 || model.fifo.num_available() || model.fifo.num_free() != 2)
    throw std::runtime_error("final state");
  std::cout << "FIFO_REFERENCE 2 3 4 ns\n";
  return 0;
}
