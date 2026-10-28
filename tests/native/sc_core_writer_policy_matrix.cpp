/** @file sc_core_writer_policy_matrix.cpp
 * @brief Native reference for writer contention and per-policy ownership.
 * @details Two native threads write equal pending values, then the second
 * writer changes its value after a zero-time wait, and the first writes later.
 * A custom native report handler copies and suppresses expected E115 errors;
 * it retains no report references or Python callbacks.
 */
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

namespace
{
using namespace sc_core;

struct ReportRecord
{
  int id;
  int severity;
  std::string type;
  std::string time;
  std::uint64_t delta;
  std::string message;
};

struct SampleRecord
{
  std::string policy;
  int value;
  std::string time;
  std::uint64_t delta;
};

std::vector<ReportRecord> reports;
std::vector<SampleRecord> samples;

/** @brief Copy a report into owned storage and suppress its default action. */
void capture_report(const sc_report& report, const sc_actions&)
{
  std::string message = report.get_msg();
  for (char& character : message)
    if (character == '\n') character = '~';
  reports.push_back({report.get_id(), static_cast<int>(report.get_severity()), report.get_msg_type(),
                     sc_time_stamp().to_string(), sc_delta_count(), message});
}

/** @brief Reject a mismatch with a deterministic native exception. */
void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Record one value-change observation with its exact scheduler position. */
void sample(const char* policy, int value)
{
  samples.push_back({policy, value, sc_time_stamp().to_string(), sc_delta_count()});
}

/** @brief Own policy-specific signals, writers and value-change observers. */
struct WriterPolicyMatrix : sc_module
{
  sc_signal<int, SC_ONE_WRITER> signal_one;
  sc_signal<int, SC_MANY_WRITERS> signal_many;
  sc_signal<int, SC_UNCHECKED_WRITERS> signal_unchecked;

  SC_CTOR(WriterPolicyMatrix)
    : signal_one("signal_one"), signal_many("signal_many"), signal_unchecked("signal_unchecked")
  {
    SC_THREAD(first);
    SC_THREAD(second);
    SC_METHOD(observe_one);
    sensitive << signal_one;
    dont_initialize();
    SC_METHOD(observe_many);
    sensitive << signal_many;
    dont_initialize();
    SC_METHOD(observe_unchecked);
    sensitive << signal_unchecked;
    dont_initialize();
  }

  /** @brief Write the common pending value, then the later final value. */
  void first()
  {
    signal_one.write(1);
    signal_many.write(1);
    signal_unchecked.write(1);
    wait(2, SC_NS);
    signal_one.write(3);
    signal_many.write(3);
    signal_unchecked.write(3);
  }

  /** @brief Repeat pending values and change them in the next delta cycle. */
  void second()
  {
    signal_one.write(1);
    signal_many.write(1);
    signal_unchecked.write(1);
    wait(SC_ZERO_TIME);
    signal_one.write(2);
    signal_many.write(2);
    signal_unchecked.write(2);
    wait(1, SC_NS);
  }

  /** @brief Observe a committed SC_ONE_WRITER value. */
  void observe_one()
  {
    sample("one", signal_one.read());
  }

  /** @brief Observe a committed SC_MANY_WRITERS value. */
  void observe_many()
  {
    sample("many", signal_many.read());
  }

  /** @brief Observe a committed SC_UNCHECKED_WRITERS value. */
  void observe_unchecked()
  {
    sample("unchecked", signal_unchecked.read());
  }
};

/** @brief Verify and emit the exact report identity and writer names. */
void emit_reports()
{
  require(reports.size() == 3, "expected two equal-write reports and one persistent-owner report");
  const std::string message_type = "sc_signal<T> cannot have more than one driver";
  const std::vector<std::pair<std::string, std::uint64_t>> expected_positions = {{"0 s", 0}, {"0 s", 0}, {"2 ns", 4}};
  for (std::size_t index = 0; index < reports.size(); ++index)
  {
    const auto& report = reports[index];
    require(report.id == 115 && report.severity == SC_ERROR && report.type == message_type, "E115 report identity");
    require(report.time == expected_positions[index].first && report.delta == expected_positions[index].second,
            "E115 report time/delta");
  }

  require(reports[0].message.find("signal `top.signal_one'") != std::string::npos, "first equal-write signal");
  require(reports[0].message.find("first driver `top.first'") != std::string::npos, "first equal-write owner");
  require(reports[0].message.find("second driver `top.second'") != std::string::npos, "second equal-write owner");
  require(reports[1].message.find("signal `top.signal_many'") != std::string::npos, "many equal-write signal");
  require(reports[1].message.find("conflicting write in delta cycle 0") != std::string::npos,
          "many policy delta diagnostic");
  require(reports[2].message.find("signal `top.signal_one'") != std::string::npos, "later one-writer signal");
  require(reports[2].message.find("first driver `top.second'") != std::string::npos, "later retained owner");
  require(reports[2].message.find("second driver `top.first'") != std::string::npos, "later conflicting writer");
  for (const auto& report : reports)
    require(report.message.find("signal `top.signal_unchecked'") == std::string::npos, "unchecked report");

  const char* policy_for_report[] = {"one", "many", "one"};
  const char* phase_for_report[] = {"equal_same_delta", "equal_same_delta", "later_owner"};
  for (std::size_t index = 0; index < reports.size(); ++index)
  {
    const auto& report = reports[index];
    std::cout << "WPM\tREPORT\t" << policy_for_report[index] << '\t' << phase_for_report[index] << '\t' << report.id
              << '\t' << report.severity << '\t' << report.type << '\t' << report.time << '\t' << report.delta << '\t'
              << report.message << '\n';
  }
}

/** @brief Verify observer transitions and emit their sampled times and deltas. */
void emit_samples()
{
  const std::vector<SampleRecord> expected = {
    {"one", 1, "0 s", 1},  {"many", 1, "0 s", 1},  {"unchecked", 1, "0 s", 1},
    {"one", 2, "0 s", 2},  {"many", 2, "0 s", 2},  {"unchecked", 2, "0 s", 2},
    {"one", 3, "2 ns", 5}, {"many", 3, "2 ns", 5}, {"unchecked", 3, "2 ns", 5}};
  require(samples.size() == expected.size(), "observer sample count");
  for (std::size_t index = 0; index < samples.size(); ++index)
  {
    const auto& actual = samples[index];
    const auto& wanted = expected[index];
    require(actual.policy == wanted.policy && actual.value == wanted.value && actual.time == wanted.time &&
              actual.delta == wanted.delta,
            "observer value/time/delta transition");
    std::cout << "WPM\tSAMPLE\t" << actual.policy << '\t' << actual.value << '\t' << actual.time << '\t' << actual.delta
              << '\n';
  }
}

}  // namespace

int sc_main(int, char**)
{
  const auto old_handler = sc_report_handler::set_handler(capture_report);
  try
  {
    WriterPolicyMatrix model("top");
    sc_start(3, SC_NS);
    sc_report_handler::set_handler(old_handler);
    emit_reports();
    emit_samples();
    require(model.signal_one.read() == 3 && model.signal_many.read() == 3 && model.signal_unchecked.read() == 3,
            "final signal values");
    for (const auto& item : std::vector<std::pair<const char*, int>>{{"one", model.signal_one.read()},
                                                                     {"many", model.signal_many.read()},
                                                                     {"unchecked", model.signal_unchecked.read()}})
      std::cout << "WPM\tFINAL\t" << item.first << '\t' << item.second << '\t' << sc_time_stamp() << '\t'
                << sc_delta_count() << '\n';
    require(sc_time_stamp() == sc_time(3, SC_NS), "final simulation time");
  }
  catch (...)
  {
    sc_report_handler::set_handler(old_handler);
    throw;
  }
  std::cout << "SC_CORE_WRITER_POLICY_MATRIX_OK\n";
  return 0;
}
