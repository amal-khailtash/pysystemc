/** @file sc_core_time_tuple.cpp
 * @brief Public time tuple normalization/conversion native reference.
 * Tuples own scalar values; strings are copied, no callback or scheduler entry.
 */
#include <iomanip>
#include <iostream>
#include <systemc>
#include <vector>
/** @brief Compare zero and normalized units through the public tuple members. */
int sc_main(int, char**)
{
  using namespace sc_core;
  std::vector<sc_time> times{SC_ZERO_TIME, sc_time(1, SC_FS), sc_time(1500, SC_PS), sc_time(2500, SC_NS),
                             sc_time(3, SC_SEC)};
  for (unsigned index = 0; index < times.size(); ++index)
  {
    sc_time_tuple tuple = index ? sc_time_tuple(times[index]) : sc_time_tuple();
    sc_time converted = tuple;
    if (converted != times[index] || !tuple.has_value()) return 1;
    std::cout << "TIME_TUPLE_REFERENCE " << index << " " << tuple.has_value() << " " << tuple.value() << " "
              << int(tuple.unit()) << " " << tuple.unit_symbol() << " " << std::setprecision(17) << tuple.to_double()
              << " " << tuple.to_string() << "\n";
  }
  return 0;
}
