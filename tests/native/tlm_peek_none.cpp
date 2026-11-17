/** @file tlm_peek_none.cpp
 *  @brief Compare native TLM None-equivalent tag behavior for int and wide data.
 *  @details Native probes own all stored values and counters. Calls are
 *  synchronous, have no Python callbacks or GIL transitions, and do not run
 *  the SystemC scheduler.
 */
#include <iostream>
#include <stdexcept>

#include "tlm_peek_none_probe.h"

namespace
{

/** @brief Print the low word and high bit of a copied wide sample. */
void print_wide(const sc_dt::sc_biguint<257>& value)
{
  std::cout << value.to_uint64() << ' ' << value[256].to_bool();
}

/** @brief Check native no-argument, null-tag, tag and output forms for T. */
template <class T>
void check_scalar()
{
  tlm_peek_none::PeekNoneProbe<T> probe(T(23));
  auto& combined = static_cast<tlm::tlm_blocking_get_peek_if<T>&>(probe);
  auto& peek = static_cast<tlm::tlm_blocking_peek_if<T>&>(combined);
  tlm::tlm_tag<T> tag;

  const T native_reference = tlm_peek_none::typed_nullptr_peek<T>(peek);
  const T no_argument = combined.peek();
  const T none_nullptr = combined.peek(static_cast<tlm::tlm_tag<T>*>(nullptr));
  const T explicit_nullptr = combined.peek(static_cast<tlm::tlm_tag<T>*>(nullptr));
  const T tagged = combined.peek(&tag);

  if (native_reference != T(23) || no_argument != T(23) || none_nullptr != T(23) || explicit_nullptr != T(23) ||
      tagged != T(23) || probe.value() != T(23) || probe.peek_calls() != 5 || probe.get_calls() != 0 ||
      sc_core::sc_time_stamp().value() != 0)
    throw std::runtime_error("scalar blocking peek parity failed");

  std::cout << "TLM_PEEK_NONE_INT " << native_reference << ' ' << no_argument << ' ' << none_nullptr << ' '
            << explicit_nullptr << ' ' << tagged << ' ' << probe.value() << ' ' << probe.peek_calls() << ' '
            << probe.get_calls() << ' ' << sc_core::sc_time_stamp().value() << '\n';
}

/** @brief Check native forms and full-width output storage for 257-bit values. */
void check_wide()
{
  using Wide = sc_dt::sc_biguint<257>;
  Wide initial = 1;
  initial <<= 256;
  initial += 7;
  tlm_peek_none::PeekNoneProbe<Wide> probe(initial);
  auto& combined = static_cast<tlm::tlm_blocking_get_peek_if<Wide>&>(probe);
  auto& peek = static_cast<tlm::tlm_blocking_peek_if<Wide>&>(combined);
  tlm::tlm_tag<Wide> tag;

  const Wide native_reference = tlm_peek_none::typed_nullptr_peek<Wide>(peek);
  const Wide no_argument = combined.peek();
  const Wide none_nullptr = combined.peek(static_cast<tlm::tlm_tag<Wide>*>(nullptr));
  const Wide explicit_nullptr = combined.peek(static_cast<tlm::tlm_tag<Wide>*>(nullptr));
  const Wide tagged = combined.peek(&tag);
  Wide output = 0;
  combined.peek(output);

  const auto valid = [&](const Wide& value)
  { return value == initial && value.to_uint64() == 7 && value[256].to_bool(); };
  if (!valid(native_reference) || !valid(no_argument) || !valid(none_nullptr) || !valid(explicit_nullptr) ||
      !valid(tagged) || !valid(output) || !valid(probe.value()) || probe.peek_calls() != 6 || probe.get_calls() != 0 ||
      sc_core::sc_time_stamp().value() != 0)
    throw std::runtime_error("wide blocking peek parity failed");

  std::cout << "TLM_PEEK_NONE_WIDE ";
  print_wide(native_reference);
  std::cout << ' ';
  print_wide(no_argument);
  std::cout << ' ';
  print_wide(none_nullptr);
  std::cout << ' ';
  print_wide(explicit_nullptr);
  std::cout << ' ';
  print_wide(tagged);
  std::cout << ' ';
  print_wide(output);
  std::cout << ' ';
  print_wide(probe.value());
  std::cout << ' ' << probe.peek_calls() << ' ' << probe.get_calls() << ' ' << sc_core::sc_time_stamp().value() << '\n';
}

}  // namespace

/** @brief Run synchronous native references without starting simulation. */
int sc_main(int, char**)
{
  check_scalar<int>();
  check_wide();
  return 0;
}
