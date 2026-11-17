/** @file public_adapter_coverage.cpp
 * @brief Native reference for public report assignment and resolved reads.
 * @details Uses independent native writer processes and compares value/time
 *          samples after resolved-channel updates with the Python binding.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

namespace
{
using namespace sc_core;

std::vector<sc_report> reports;

/** @brief Copy the borrowed report while the native handler dispatches it. */
void capture_report(const sc_report& report, const sc_actions&)
{
  reports.push_back(report);
}

/** @brief Drive zero and release the first resolved signal driver. */
struct ZeroWriter : sc_module
{
  sc_signal_resolved& signal;

  SC_HAS_PROCESS(ZeroWriter);

  /** @brief Bind the signal and register this independent writer process. */
  ZeroWriter(sc_module_name name, sc_signal_resolved& target) : sc_module(name), signal(target)
  {
    SC_THREAD(run);
  }

  /** @brief Write zero at 1 ns, then Z at 3 ns. */
  void run()
  {
    wait(1, SC_NS);
    signal.write(sc_dt::SC_LOGIC_0);
    wait(2, SC_NS);
    signal.write(sc_dt::SC_LOGIC_Z);
  }
};

/** @brief Drive a separate sequence through the second resolved driver. */
struct OneWriter : sc_module
{
  sc_signal_resolved& signal;

  SC_HAS_PROCESS(OneWriter);

  /** @brief Bind the signal and register the second independent writer. */
  OneWriter(sc_module_name name, sc_signal_resolved& target) : sc_module(name), signal(target)
  {
    SC_THREAD(run);
  }

  /** @brief Write Z, one, and zero at 1 ns, 2 ns, and 4 ns. */
  void run()
  {
    wait(1, SC_NS);
    signal.write(sc_dt::SC_LOGIC_Z);
    wait(1, SC_NS);
    signal.write(sc_dt::SC_LOGIC_1);
    wait(2, SC_NS);
    signal.write(sc_dt::SC_LOGIC_0);
  }
};

/** @brief Read and record resolved channel changes after native updates. */
struct Observer : sc_module
{
  sc_signal_resolved& signal;
  std::vector<std::pair<std::string, char>> samples;

  SC_HAS_PROCESS(Observer);

  /** @brief Bind the channel and observe updates without initialization. */
  Observer(sc_module_name name, sc_signal_resolved& target) : sc_module(name), signal(target)
  {
    SC_METHOD(observe);
    sensitive << signal;
    dont_initialize();
  }

  /** @brief Copy the resolved read value and simulation timestamp. */
  void observe()
  {
    const auto sample = std::make_pair(sc_time_stamp().to_string(), signal.read().to_char());
    samples.push_back(sample);
    std::cout << "RESOLVED_READ " << sample.first << ' ' << sample.second << '\n';
  }
};

/** @brief Encode arbitrary text bytes as lowercase hexadecimal. */
std::string hex_text(const std::string& value)
{
  static const char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(value.size() * 2);
  for (unsigned char ch : value)
  {
    result.push_back(digits[ch >> 4]);
    result.push_back(digits[ch & 0x0f]);
  }
  return result;
}
}  // namespace

/** @brief Emit native report-assignment and resolved-channel reference records. */
int sc_main(int, char**)
{
  using namespace sc_core;

  const auto previous_handler = sc_report_handler::set_handler(capture_report);
  sc_report_handler::report(SC_INFO, "assignment/destination", "old", "dest.cpp", 11);
  sc_report_handler::report(SC_WARNING, "assignment/source", "copied", "source.cpp", 73);
  sc_report_handler::report(SC_ERROR, "assignment/alias-target", "alias", "alias.cpp", 97);
  sc_report_handler::set_handler(previous_handler);
  if (reports.size() != 3) return 2;

  sc_report& destination = reports[0];
  const sc_report& source = reports[1];
  const sc_report& alias_target = reports[2];
  sc_report& returned = destination.operator=(source);
  if (&returned != &destination || destination.get_id() != source.get_id() ||
      destination.get_severity() != source.get_severity() ||
      std::string(destination.get_msg_type()) != source.get_msg_type() ||
      std::string(destination.get_msg()) != source.get_msg() ||
      std::string(destination.get_file_name()) != source.get_file_name() ||
      destination.get_line_number() != source.get_line_number() ||
      destination.get_verbosity() != source.get_verbosity() ||
      destination.get_time().value() != source.get_time().value() || std::string(destination.what()) != source.what())
    return 3;

  returned.operator=(alias_target);
  if (std::string(destination.get_msg_type()) != alias_target.get_msg_type() ||
      std::string(destination.get_msg()) != alias_target.get_msg() ||
      std::string(destination.get_file_name()) != alias_target.get_file_name() ||
      destination.get_line_number() != alias_target.get_line_number())
    return 4;
  destination.operator=(source);

  std::cout << "REPORT_ASSIGN 1 " << destination.get_id() << ' ' << int(destination.get_severity()) << ' '
            << destination.get_msg_type() << ' ' << destination.get_msg() << ' ' << destination.get_file_name() << ' '
            << destination.get_line_number() << ' ' << destination.get_time().value() << ' '
            << destination.get_verbosity() << ' ' << hex_text(destination.what()) << '\n';
  const std::string before_self_assignment = destination.what();
  sc_report& self_result = destination.operator=(destination);
  const bool self_preserved = std::string(destination.what()) == before_self_assignment;
  if (&self_result != &destination || !self_preserved) return 5;
  self_result.operator=(alias_target);
  if (std::string(destination.what()) != alias_target.what()) return 6;
  destination.operator=(source);
  if (std::string(destination.what()) != before_self_assignment) return 7;
  std::cout << "REPORT_SELF 1 " << int(self_preserved) << '\n';

  sc_signal_resolved signal("resolved");
  ZeroWriter zero_writer("zero_writer", signal);
  OneWriter one_writer("one_writer", signal);
  Observer observer("observer", signal);
  sc_start(5, SC_NS);
  const std::vector<std::pair<std::string, char>> expected = {
    {"1 ns", '0'}, {"2 ns", 'X'}, {"3 ns", '1'}, {"4 ns", '0'}};
  if (observer.samples != expected || signal.read().to_char() != '0') return 8;
  std::cout << "RESOLVED_FINAL " << sc_time_stamp() << ' ' << signal.read().to_char() << '\n';
  sc_stop();
  return 0;
}
