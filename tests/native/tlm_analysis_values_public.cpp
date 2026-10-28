/** @file tlm_analysis_values_public.cpp
 *  @brief Native reference for arbitrary analysis FIFO payload values.
 *  @details Pointer queue entries retain borrowed owner storage while wide
 *  value entries are copied into the native FIFO.
 */
#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_fifo.h>
#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_triple.h>

#include <iostream>
#include <stdexcept>

// The bundled FIFO methods call a dependent-base nb_put without `this->`.
// Define the exact native overloads before the probe instantiates these FIFOs.
template <>
inline void tlm::tlm_analysis_fifo<int*>::write(int* const& value)
{
  this->nb_put(value);
}

template <>
inline void tlm::tlm_analysis_fifo<int*>::write(const tlm::tlm_analysis_triple<int*>& triple)
{
  this->nb_put(triple.transaction);
}

template <>
inline void tlm::tlm_analysis_fifo<sc_dt::sc_biguint<191>>::write(const sc_dt::sc_biguint<191>& value)
{
  this->nb_put(value);
}

template <>
inline void tlm::tlm_analysis_fifo<sc_dt::sc_biguint<191>>::write(
  const tlm::tlm_analysis_triple<sc_dt::sc_biguint<191>>& triple)
{
  this->nb_put(triple.transaction);
}

#include "tlm_analysis_values_probe.h"

namespace
{
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}
}  // namespace

int sc_main(int, char**)
{
  using namespace tlm_analysis_values_probe;
  Owner owner("analysis_values_owner");

  int* first_pointer = owner.pointer_at(0);
  int* second_pointer = owner.pointer_at(2);
  owner.pointer_fifo.write(first_pointer);
  owner.pointer_fifo.write(tlm::tlm_analysis_triple<int*>(second_pointer));
  owner.pointer_values[0] = 111;
  owner.pointer_values[1] = 222;
  owner.pointer_values[2] = 333;
  owner.pointer_values[3] = 444;

  WideValue first_wide = 165;
  first_wide[190] = true;
  WideValue second_wide = 90;
  second_wide[190] = true;
  owner.wide_fifo.write(first_wide);
  owner.wide_fifo.write(tlm::tlm_analysis_triple<WideValue>(second_wide));
  first_wide = 101;
  second_wide = 202;

  sc_core::sc_start(sc_core::SC_ZERO_TIME);

  const PointerObservation first_pointer_read = pop_pointer(owner.pointer_fifo);
  const PointerObservation second_pointer_read = pop_pointer(owner.pointer_fifo);
  const bool first_pointer_matches = first_pointer_read.address == Owner::pointer_address(first_pointer);
  const bool second_pointer_matches = second_pointer_read.address == Owner::pointer_address(second_pointer);
  verify(first_pointer_matches && first_pointer_read.first == 111 && first_pointer_read.second == 222,
         "direct pointer analysis FIFO write mismatch");
  verify(second_pointer_matches && second_pointer_read.first == 333 && second_pointer_read.second == 444,
         "triple pointer analysis FIFO write mismatch");
  std::cout << "ANALYSIS_VALUES_POINTER " << first_pointer_matches << ' ' << first_pointer_read.first << ' '
            << first_pointer_read.second << ' ' << second_pointer_matches << ' ' << second_pointer_read.first << ' '
            << second_pointer_read.second << '\n';

  const WideValue first_wide_read = pop_value(owner.wide_fifo);
  const WideValue second_wide_read = pop_value(owner.wide_fifo);
  verify(first_wide_read[190] && first_wide_read.to_uint64() == 165, "direct wide analysis FIFO write mismatch");
  verify(second_wide_read[190] && second_wide_read.to_uint64() == 90, "triple wide analysis FIFO write mismatch");
  std::cout << "ANALYSIS_VALUES_WIDE " << first_wide_read[190] << ' ' << first_wide_read.to_uint64() << ' '
            << second_wide_read[190] << ' ' << second_wide_read.to_uint64() << '\n';
  std::cout << "TLM_ANALYSIS_VALUES_PUBLIC_OK\n";
  sc_core::sc_stop();
  return 0;
}
