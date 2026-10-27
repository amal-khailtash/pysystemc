/** @file sc_core_report_verbosity_matrix.cpp
 * @brief Native verbosity boundary and severity filtering reference.
 * @details The callback borrows reports for one synchronous call and retains
 * only scalar evidence. No simulation or Python/GIL interaction occurs.
 */
#include <array>
#include <limits>
#include <stdexcept>
#include <systemc>

namespace
{
int calls = 0;
int last_severity = 0;
int last_verbosity = 0;
unsigned last_actions = 0;
/** @brief Record scalar metadata while leaving report actions unexecuted. */
void capture(const sc_core::sc_report& report, const sc_core::sc_actions& actions)
{
  ++calls;
  last_severity = report.get_severity();
  last_verbosity = report.get_verbosity();
  last_actions = actions;
}
}  // namespace

/** @brief Compare all accepted integer levels and default/explicit report forms. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using h = sc_report_handler;
  const std::array<int, 9> levels = {std::numeric_limits<int>::min(), -1, 0, 1, SC_LOW, SC_MEDIUM, SC_HIGH, SC_FULL,
                                     std::numeric_limits<int>::max()};
  const auto previous_handler = h::set_handler(capture);
  const int previous_level = h::get_verbosity_level();
  const char* message = "verbosity/matrix";
  h::set_actions(message, SC_DO_NOTHING);
  for (int level_index = 0; level_index < int(levels.size()); ++level_index)
  {
    const int level = levels[level_index];
    const int old = h::set_verbosity_level(level);
    if (old != (level_index == 0 ? previous_level : levels[level_index - 1]) || h::get_verbosity_level() != level)
      throw std::runtime_error("verbosity level did not preserve native integer");
    for (const auto severity : {SC_INFO, SC_WARNING, SC_ERROR, SC_FATAL})
    {
      for (int verbosity_index = 0; verbosity_index <= int(levels.size()); ++verbosity_index)
      {
        const int effective = verbosity_index == 0 ? SC_MEDIUM : levels[verbosity_index - 1];
        const std::array<int, 4> before = {h::get_count(message), h::get_count(message, severity),
                                           h::get_count(severity), calls};
        if (verbosity_index == 0)
          h::report(severity, message, "boundary", "verbosity_matrix.cpp", 21);
        else
          h::report(severity, message, "boundary", effective, "verbosity_matrix.cpp", 21);
        const std::array<int, 4> after = {h::get_count(message), h::get_count(message, severity),
                                          h::get_count(severity), calls};
        const int expected = severity != SC_INFO || effective <= level;
        for (int index = 0; index < 4; ++index)
          if (after[index] - before[index] != expected) throw std::runtime_error("verbosity filtering count mismatch");
        if (expected && (last_severity != severity || last_verbosity != effective || last_actions != SC_DO_NOTHING))
          throw std::runtime_error("verbosity dispatch metadata mismatch");
        std::cout << "REPORT_VERBOSITY " << level_index << " " << level << " " << int(severity) << " "
                  << verbosity_index << " " << effective;
        for (int index = 0; index < 4; ++index) std::cout << " " << after[index] - before[index];
        std::cout << "\n";
      }
    }
  }
  if (h::set_verbosity_level(previous_level) != levels.back()) return 2;
  h::set_handler(previous_handler);
  return 0;
}
