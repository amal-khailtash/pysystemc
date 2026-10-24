/** @file sc_core_time_public.cpp
 * @brief Independent native time constructor, conversion and mutation references.
 */
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <systemc>
#include <type_traits>

/** @brief Enforce reference-model checks in release and debug native builds. */
void require(bool condition)
{
  if (!condition) throw std::runtime_error("native time reference mismatch");
}

/** @brief Record all public conversions without changing a borrowed duration. */
void emit(const std::string& label, const sc_core::sc_time& value)
{
  std::ostringstream stream;
  value.print(stream);
  require(stream.str() == value.to_string());
  std::cout << "TIME_PUBLIC " << label << ' ' << value.value() << ' ' << std::fixed << std::setprecision(1)
            << value.to_double() << ' ' << std::defaultfloat << std::setprecision(17) << value.to_seconds() << ' '
            << value.to_default_time_units() << ' ' << stream.str() << '\n';
}

/** @brief Compare native public declarations, diagnostics and recovery. */
int sc_main(int, char**)
{
  using namespace sc_core;
  static_assert(std::is_same_v<sc_time::value_type, sc_dt::uint64>);
  sc_set_time_resolution(1, SC_PS);
  sc_set_default_time_unit(1, SC_NS);
  auto* context = sc_get_curr_simcontext();
  emit("default", sc_time());
  sc_time base(1.25, SC_NS);
  emit("copy", sc_time(base));
  const char* units[] = {"fs", "ps", "ns", "us", "ms", "s"};
  const char* full[] = {"SC_FS", "SC_PS", "SC_NS", "SC_US", "SC_MS", "SC_SEC"};
  for (int index = 0; index < 6; ++index)
  {
    auto unit = static_cast<sc_time_unit>(index);
    sc_time expected(2.5, unit);
    require(sc_time(2.5, unit, context) == expected);
    require(sc_time(2.5, units[index]) == expected);
    require(sc_time(2.5, units[index], context) == expected);
    require(sc_time(2.5, full[index]) == expected);
    emit(units[index], expected);
  }
  emit("seconds", sc_time::from_seconds(1.25e-9));
  emit("string", sc_time::from_string(" 1.25 ns"));
  emit("ticks", sc_time::from_value((sc_dt::uint64(1) << 63) + 17));
  for (bool scale : {false, true}) emit(std::string("double") + char('0' + scale), sc_time(2.5, scale));
  for (bool scale : {false, true}) emit(std::string("uint64") + char('0' + scale), sc_time(sc_dt::uint64(3), scale));
  sc_time copy;
  auto* assigned = &(copy = base);
  require(assigned == &copy);
  assigned = &(copy = copy);
  require(assigned == &copy);
  require(copy.value() == 1250 && base.value() == 1250);
  std::cout << "TIME_RELATIONS " << (copy == base) << ' ' << (copy != sc_time()) << ' ' << (sc_time() < base) << ' '
            << (copy <= base) << ' ' << (base > sc_time()) << ' ' << (copy >= base) << '\n';
  sc_time value(base);
  auto* result_43 = &(value += sc_time::from_value(251));
  require(result_43 == &value);
  emit("add", value);
  value = base;
  auto* result_45 = &(value -= sc_time::from_value(251));
  require(result_45 == &value);
  emit("sub", value);
  value = base;
  auto* result_42 = &(value *= 1.5);
  require(result_42 == &value);
  emit("mul", value);
  value = base;
  auto* result_47 = &(value /= 1.5);
  require(result_47 == &value);
  emit("div", value);
  value = base;
  auto* result_37 = &(value %= sc_time::from_value(251));
  require(result_37 == &value);
  emit("mod", value);
  require(base.value() == 1250);
  for (const char* text : {"", "-1 ns", "abc ns", "1", "1 bad"})
  {
    try
    {
      sc_time::from_string(text);
      require(false);
    }
    catch (const sc_report& error)
    {
      std::cout << "TIME_ERROR string " << error.get_id() << '\n';
    }
  }
  for (const char* unit : {"", "invalid"})
  {
    for (bool explicit_context : {false, true})
    {
      try
      {
        if (explicit_context)
          sc_time(1., unit, context);
        else
          sc_time(1., unit);
        require(false);
      }
      catch (const sc_report& error)
      {
        std::cout << "TIME_ERROR unit " << error.get_id() << '\n';
      }
    }
  }
  require(sc_time::from_string("3 ns").value() == 3000);
  std::cout << "TIME_RECOVERY 3000\n";
  return 0;
}
