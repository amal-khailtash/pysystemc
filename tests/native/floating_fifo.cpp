/** @file floating_fifo.cpp
 * @brief Native FIFO reference for exceptional floating samples.
 * @details Each FIFO owns copied samples. All accesses occur in sc_main;
 * no Python callbacks, GIL operations, or borrowed values survive the call.
 */
#include <cmath>
#include <iostream>
#include <limits>
#include <systemc>

/** @brief Check blocking and nonblocking reads for each floating sample. */
template <class T>
void check(const char* name)
{
  sc_core::sc_fifo<T> fifo(name, 1);
  T values[] = {std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(),
                std::numeric_limits<T>::quiet_NaN(), T(-0.0), T(1.25)};
  for (T value : values)
  {
    fifo.write(value);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    T result = fifo.read();
    bool blocking =
      std::isnan(value) ? std::isnan(result) : result == value && std::signbit(result) == std::signbit(value);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    fifo.write(value);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    bool success = fifo.nb_read(result);
    bool nonblocking = success && (std::isnan(value) ? std::isnan(result)
                                                     : result == value && std::signbit(result) == std::signbit(value));
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    fifo.write(value);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    fifo.read(result);
    bool assigned =
      std::isnan(value) ? std::isnan(result) : result == value && std::signbit(result) == std::signbit(value);
    if (!blocking || !nonblocking || !assigned) throw std::runtime_error("floating FIFO mismatch");
    std::cout << "FLOAT_FIFO " << name << " " << blocking << " " << nonblocking << "\n";
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
  }
}

/** @brief Run each type in an independent kernel via the process argument. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) return 2;
  std::string type = argv[1];
  if (type == "float")
    check<float>("float");
  else if (type == "double")
    check<double>("double");
  else
    check<long double>("long_double");
  return 0;
}
