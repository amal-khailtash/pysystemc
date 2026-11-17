/**
 * @file wide_signal_contracts.cpp
 * @brief Native reference for arbitrary-width signal update and event contracts.
 * @details Each process owns its signal and callback state for the full kernel
 * lifetime. No Python references, callbacks, GIL state, or cross-thread work
 * are involved in the native reference.
 */
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

/** @brief Store one value-change observation in native-comparable form. */
struct wide_signal_record
{
  int width;
  int index;
  std::uint64_t ticks;
  std::uint64_t delta;
  std::string value;
};

/** @brief Exercise deferred writes and value-change sensitivity for one width. */
template <int Width>
struct wide_signal_model : sc_core::sc_module
{
  using value_type = sc_dt::sc_biguint<Width>;

  sc_core::sc_signal<value_type> signal{"signal"};
  std::vector<wide_signal_record> records;

  SC_HAS_PROCESS(wide_signal_model);

  /** @brief Register a non-initializing observer that retains no external state. */
  explicit wide_signal_model(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_METHOD(observe_value_change);
    sensitive << signal;
    dont_initialize();
  }

  /** @brief Record the committed full value, timestamp, and current delta. */
  void observe_value_change()
  {
    records.push_back({Width, static_cast<int>(records.size() + 1), sc_core::sc_time_stamp().value(),
                       sc_core::sc_delta_count(), signal.read().to_string(sc_dt::SC_DEC)});
  }
};

/** @brief Validate one instantiated width and print its observation sequence. */
template <int Width>
int run_width()
{
  using namespace sc_core;
  using value_type = sc_dt::sc_biguint<Width>;
  wide_signal_model<Width> top("wide_signal_model");

  value_type high = 0;
  high[Width - 1] = true;
  top.signal.write(high);
  if (top.signal.read() != 0) throw std::runtime_error("wide signal write was visible before update");
  sc_start(SC_ZERO_TIME);
  if (top.records.size() != 1 || top.records.back().value != high.to_string(sc_dt::SC_DEC))
    throw std::runtime_error("high-bit update did not produce exactly one event");

  top.signal.write(high);
  sc_start(1, SC_NS);
  if (top.records.size() != 1) throw std::runtime_error("unchanged wide signal write produced an event");

  value_type high_and_low = high;
  high_and_low[0] = true;
  if (top.signal.read() != high) throw std::runtime_error("unexpected value before low-bit update");
  top.signal.write(high_and_low);
  if (top.signal.read() != high) throw std::runtime_error("low-bit write was visible before update");
  sc_start(1, SC_NS);
  if (top.records.size() != 2 || top.records.back().value != high_and_low.to_string(sc_dt::SC_DEC))
    throw std::runtime_error("low-bit update did not produce exactly one event");

  value_type low = 1;
  if (top.signal.read() != high_and_low) throw std::runtime_error("unexpected value before high-bit clear");
  top.signal.write(low);
  if (top.signal.read() != high_and_low) throw std::runtime_error("high-bit clear was visible before update");
  sc_start(1, SC_NS);
  if (top.records.size() != 3 || top.records.back().value != low.to_string(sc_dt::SC_DEC))
    throw std::runtime_error("high-bit clear did not produce exactly one event");

  std::uint64_t previous_delta = 0;
  std::uint64_t previous_ticks = 0;
  const auto ns = sc_time(1, SC_NS).value();
  if (top.records[0].ticks != 0 || top.records[1].ticks != ns || top.records[2].ticks != 2 * ns)
    throw std::runtime_error("wide signal callbacks occurred at unexpected times");
  for (const auto& record : top.records)
  {
    if (record.delta < previous_delta || record.ticks < previous_ticks)
      throw std::runtime_error("wide signal event order did not advance monotonically");
    previous_delta = record.delta;
    previous_ticks = record.ticks;
    std::cout << "WIDE_SIGNAL_EVENT|" << record.width << '|' << record.index << '|' << record.ticks << '|'
              << record.delta << '|' << record.value << '\n';
  }
  if (top.records[0].delta > top.records[1].delta || top.records[1].delta >= top.records[2].delta)
    throw std::runtime_error("wide signal delta order did not advance across updates");
  std::cout << "WIDE_SIGNAL_RESULT|" << Width << "|events=" << top.records.size() << "|lifetime=native\n";
  return 0;
}

/** @brief Run the requested width in an independent SystemC kernel process. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::invalid_argument("expected one signal width argument");
  const int width = std::atoi(argv[1]);
  switch (width)
  {
  case 65:
    return run_width<65>();
  case 137:
    return run_width<137>();
  case 257:
    return run_width<257>();
  default:
    throw std::invalid_argument("unsupported test width");
  }
}
