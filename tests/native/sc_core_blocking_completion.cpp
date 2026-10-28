/** @file sc_core_blocking_completion.cpp
 * @brief Compare native reset/kill completion for first-party channel blocks.
 * @details Native channels and borrowed output/input values outlive every
 * operation. Cleanup records are emitted only after native unwind is caught;
 * the original scheduler exception is always rethrown to SystemC.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <array>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>

/** @brief Own channel storage and independently block each audited route. */
struct blocking_completion_model : sc_core::sc_module
{
  using wide_type = sc_dt::sc_biguint<137>;
  using signed_type = sc_dt::sc_bigint<137>;
  static constexpr std::array<const char*, 17> names{"core_read_value",
                                                     "core_read_output",
                                                     "core_write",
                                                     "tlm_get_value",
                                                     "tlm_get_output",
                                                     "tlm_peek_value",
                                                     "tlm_peek_output",
                                                     "tlm_put",
                                                     "tlm_peek_interface_output",
                                                     "scoped_lock",
                                                     "tlm_get_tag_value",
                                                     "tlm_get_null_tag_value",
                                                     "tlm_peek_tag_value",
                                                     "tlm_peek_null_tag_value",
                                                     "core_bigint_read_value",
                                                     "core_bigint_read_output",
                                                     "core_bigint_write"};
  sc_core::sc_fifo<int> core_read_value{"core_read_value", 1};
  sc_core::sc_fifo<int> core_read_output{"core_read_output", 1};
  sc_core::sc_fifo<int> core_write{"core_write", 1};
  sc_core::sc_fifo<signed_type> core_bigint_read_value{"core_bigint_read_value", 1};
  sc_core::sc_fifo<signed_type> core_bigint_read_output{"core_bigint_read_output", 1};
  sc_core::sc_fifo<signed_type> core_bigint_write{"core_bigint_write", 1};
  tlm::tlm_fifo<wide_type> tlm_get_value{"tlm_get_value", 1};
  tlm::tlm_fifo<wide_type> tlm_get_output{"tlm_get_output", 1};
  tlm::tlm_fifo<wide_type> tlm_peek_value{"tlm_peek_value", 1};
  tlm::tlm_fifo<wide_type> tlm_peek_output{"tlm_peek_output", 1};
  tlm::tlm_fifo<wide_type> tlm_put{"tlm_put", 1};
  tlm::tlm_fifo<wide_type> tlm_get_tag_value{"tlm_get_tag_value", 1};
  tlm::tlm_fifo<wide_type> tlm_get_null_tag_value{"tlm_get_null_tag_value", 1};
  tlm::tlm_fifo<wide_type> tlm_peek_tag_value{"tlm_peek_tag_value", 1};
  tlm::tlm_fifo<wide_type> tlm_peek_null_tag_value{"tlm_peek_null_tag_value", 1};
  tlm::tlm_fifo<wide_type> tlm_peek_interface{"tlm_peek_interface", 1};
  sc_core::sc_port<tlm::tlm_blocking_peek_if<wide_type>> peek_port{"peek_port"};
  sc_core::sc_mutex mutex{"mutex"};
  std::array<sc_core::sc_process_handle, names.size()> handles{};
  std::array<int, names.size()> starts{};
  std::array<int, names.size()> unwinds{};
  sc_core::sc_process_handle holder, controller;

  /** @brief Construct a wide payload with an observable bit above the low word. */
  static wide_type wide(unsigned long low)
  {
    wide_type result(low);
    result[136] = true;
    return result;
  }

  /** @brief Construct a signed wide payload with its sign bit set. */
  static signed_type signed_wide(unsigned long low)
  {
    signed_type result(low);
    result[136] = true;
    return result;
  }

  /** @brief Seed full channels and register each operation and controller. */
  explicit blocking_completion_model(sc_core::sc_module_name name) : sc_module(name)
  {
    if (!core_write.nb_write(17) || !core_bigint_write.nb_write(signed_wide(17)) || !tlm_put.nb_put(wide(17)))
      throw std::runtime_error("could not seed full channels");
    peek_port.bind(tlm_peek_interface);
    holder = sc_core::sc_spawn([this] { hold_mutex(); });
    for (std::size_t index = 0; index < names.size(); ++index)
      handles[index] = sc_core::sc_spawn([this, index] { consume(index); });
    controller = sc_core::sc_spawn([this] { control(); });
  }

  /** @brief Record cleanup, verify borrowed storage, and rethrow scheduler unwind. */
  void consume(std::size_t index)
  {
    const sc_core::sc_time expected_start(starts[index] == 0 ? 0 : 2, sc_core::SC_NS);
    if (sc_core::sc_time_stamp() != expected_start) throw std::runtime_error("unexpected start time");
    ++starts[index];
    int core_output = 71;
    int core_input = 99;
    wide_type output = wide(71), input = wide(99);
    signed_type signed_output = signed_wide(71), signed_input = signed_wide(99);
    tlm::tlm_tag<wide_type> tag;
    try
    {
      switch (index)
      {
      case 0:
        (void)core_read_value.read();
        break;
      case 1:
        core_read_output.read(core_output);
        break;
      case 2:
        core_write.write(core_input);
        break;
      case 3:
        (void)tlm_get_value.get();
        break;
      case 4:
        static_cast<tlm::tlm_blocking_get_if<wide_type>&>(tlm_get_output).get(output);
        break;
      case 5:
        (void)tlm_peek_value.peek();
        break;
      case 6:
        static_cast<tlm::tlm_blocking_peek_if<wide_type>&>(tlm_peek_output).peek(output);
        break;
      case 7:
        tlm_put.put(input);
        break;
      case 8:
        peek_port->peek(output);
        break;
      case 9:
      {
        sc_core::sc_scoped_lock lock(mutex);
      }
      break;
      case 10:
        (void)tlm_get_tag_value.get(&tag);
        break;
      case 11:
        (void)tlm_get_null_tag_value.get(nullptr);
        break;
      case 12:
        (void)tlm_peek_tag_value.peek(&tag);
        break;
      case 13:
        (void)tlm_peek_null_tag_value.peek(nullptr);
        break;
      case 14:
        (void)core_bigint_read_value.read();
        break;
      case 15:
        core_bigint_read_output.read(signed_output);
        break;
      case 16:
        core_bigint_write.write(signed_input);
        break;
      default:
        throw std::runtime_error("invalid blocking case");
      }
      throw std::runtime_error("blocked native call returned");
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      const bool resetting = unwinds[index] == 0;
      const auto expected_time = sc_core::sc_time(resetting ? 2 : 3, sc_core::SC_NS);
      if (unwind.is_reset() != resetting || sc_core::sc_time_stamp() != expected_time)
        throw std::runtime_error("unwind kind/time mismatch");
      if (core_output != 71 || output.to_uint64() != 71 || !output[136] || core_input != 99 ||
          input.to_uint64() != 99 || !input[136] || signed_output.to_uint64() != 71 || !signed_output[136] ||
          signed_input.to_uint64() != 99 || !signed_input[136])
        throw std::runtime_error("borrowed input/output changed during unwind");
      ++unwinds[index];
      throw;
    }
  }

  /** @brief Retain mutex ownership until interrupted scoped-lock attempts end. */
  void hold_mutex()
  {
    if (mutex.lock() != 0) throw std::runtime_error("holder lock failed");
    sc_core::wait(4, sc_core::SC_NS);
    if (mutex.unlock() != 0) throw std::runtime_error("holder unlock failed");
  }

  /** @brief Reset each blocker at two ns, then kill its restarted process. */
  void control()
  {
    sc_core::wait(2, sc_core::SC_NS);
    for (auto& handle : handles) handle.reset();
    sc_core::wait(1, sc_core::SC_NS);
    for (auto& handle : handles) handle.kill();
  }

  /** @brief Check every route and print its native lifecycle oracle records. */
  void verify()
  {
    for (std::size_t index = 0; index < names.size(); ++index)
    {
      if (starts[index] != 2 || unwinds[index] != 2 || !handles[index].terminated())
        throw std::runtime_error("process lifecycle mismatch");
      const bool retained_input = index == 2 || index == 7 || index == 16;
      std::cout << "BLOCKING_REFERENCE|" << names[index] << "|start|0 s\n"
                << "BLOCKING_REFERENCE|" << names[index] << "|cleanup|2 ns|71|" << (retained_input ? 99 : 0) << '\n'
                << "BLOCKING_REFERENCE|" << names[index] << "|start|2 ns\n"
                << "BLOCKING_REFERENCE|" << names[index] << "|cleanup|3 ns|71|" << (retained_input ? 99 : 0) << '\n';
    }
    if (!holder.terminated() || !controller.terminated() || core_write.num_available() != 1 ||
        core_bigint_write.num_available() != 1 || tlm_put.used() != 1)
      throw std::runtime_error("channel/resource state mismatch");
    wide_type put_output = wide(-1);
    const auto signed_seed = core_bigint_write.read();
    if (core_write.read() != 17 || signed_seed.to_uint64() != 17 || !signed_seed[136] || !tlm_put.nb_get(put_output) ||
        put_output.to_uint64() != 17)
      throw std::runtime_error("preexisting channel values changed");
    if (mutex.trylock() != 0 || mutex.unlock() != 0) throw std::runtime_error("mutex state mismatch");
  }
};

/** @brief Run one native reference model in a fresh SystemC kernel. */
int sc_main(int, char**)
{
  blocking_completion_model model("top");
  sc_core::sc_start(5, sc_core::SC_NS);
  model.verify();
  sc_core::sc_stop();
  return 0;
}
