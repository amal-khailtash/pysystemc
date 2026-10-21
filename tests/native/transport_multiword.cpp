/** @file transport_multiword.cpp
 * @brief Independent native transport value/output copy reference.
 * @details Native channel owns response; requests/outputs are borrowed only for
 * each synchronous call. No Python callbacks, GIL operations or waits occur.
 */
#include <iostream>
#include <systemc>
#include <tlm>

/** @brief Native response owner preserving both transport overloads. */
template <class T>
struct endpoint : tlm::tlm_transport_if<int, T>
{
  T value;
  explicit endpoint(T sample) : value(sample)
  {
  }
  /** @brief Return a response copy for a verified request. */
  T transport(const int& request) override
  {
    if (request != 17) throw std::runtime_error("request");
    return value;
  }
};
/** @brief Assert exact value and output response copies. */
template <class T>
void check(T sample)
{
  endpoint<T> owner(sample);
  tlm::tlm_transport_if<int, T>& channel = owner;
  T output{};
  T value = channel.transport(17);
  channel.transport(17, output);
  if (value != sample || output != sample) throw std::runtime_error("response");
}
/** @brief Verify multiword endpoints and arbitrary-width high bit. */
int sc_main(int, char**)
{
  check(std::numeric_limits<long long>::lowest());
  check(std::numeric_limits<unsigned long long>::max());
  check(-2.5L);
  sc_dt::sc_biguint<137> wide = 17;
  wide[136] = true;
  check(wide);
  std::cout << "NATIVE_TRANSPORT_MULTIWORD_OK 4\n";
  return 0;
}
