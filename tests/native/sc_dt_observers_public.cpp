/** @file Independent native observer callback and generic-value reference. */
#include <iostream>

#include "sc_dt_observers_probe.h"
using namespace datatype_observer_probe;
/** @brief Assert checks remain active when native sc_assert is disabled. */
void verify(bool condition)
{
  if (!condition) throw std::runtime_error("datatype observer comparison");
}
/** @brief Dispatch and verify native observer storage and virtual destruction. */
template <class Probe, class Native, class Value>
void run(const char* label)
{
  const int initial = Probe::destroyed;
  {
    Value value(1.25);
    Native observer;
    observer.defaults(value);
    verify(observer.values.empty());
    Probe::dispatch(observer, value);
    verify(observer.values == std::vector<double>({1.25, 2.25, 3.25, 4.25}));
    verify(Probe::factory_identity(observer));
    std::cout << "DT_OBSERVER " << label;
    for (double number : observer.values) std::cout << ' ' << number;
    std::cout << '\n';
  }
  verify(Probe::destroyed == initial + 1);
  std::cout << "DT_OBSERVER_DTOR " << label << " 1\n";
}
int sc_main(int, char**)
{
  run<Num, NativeNum, sc_dt::sc_fixed<23, 11>>("Num");
  run<FastNum, NativeFastNum, sc_dt::sc_fixed_fast<23, 11>>("FastNum");
  run<Val, NativeVal, sc_dt::sc_fxval>("Val");
  run<FastVal, NativeFastVal, sc_dt::sc_fxval_fast>("FastVal");
  {
    ValueBase value;
  }
  verify(ValueBase::destroyed == 1);
  std::cout << "DT_VALUE_BASE 1\n";
  Generic generic(-37);
  verify(generic_arrow(generic) == reinterpret_cast<std::uintptr_t>(&generic));
  verify(generic_const_arrow(generic) == reinterpret_cast<std::uintptr_t>(&generic));
  sc_dt::sc_signed signed_value(137);
  sc_dt::sc_unsigned unsigned_value(137);
  generic.to_sc_signed(signed_value);
  generic.to_sc_unsigned(unsigned_value);
  sc_dt::sc_uint<19> left(12);
  sc_dt::sc_uint<7> right(3);
  auto& concat = sc_dt::concat(left, right);
  verify(concat_arrow(concat) == reinterpret_cast<std::uintptr_t>(&concat));
  verify(concat_const_arrow(concat) == reinterpret_cast<std::uintptr_t>(&concat));
  verify(signed_value.to_int64() == -37 && unsigned_value.to_uint64() == static_cast<sc_dt::uint64>(-37));
  verify(concat.to_uint64() == ((12 << 7) | 3));
  std::cout << "DT_GENERIC " << signed_value.to_int64() << ' ' << unsigned_value.to_uint64() << ' '
            << concat.to_uint64() << '\n';
  std::cout << "SC_DT_OBSERVERS_PUBLIC_OK\n";
  return 0;
}
