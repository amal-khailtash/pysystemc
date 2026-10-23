/** @file sc_core_time_free_operators.cpp
 * @brief Native references for the seven free sc_time arithmetic overloads.
 */
#include <iomanip>
#include <iostream>
#include <systemc>
#include <type_traits>
#include <utility>

namespace
{
/** @brief Detect whether two time values can be multiplied by native overload.
 * @tparam T Native duration type.
 * @tparam Enable Substitution result for the attempted product expression.
 */
template <typename T, typename Enable = void>
struct is_time_multipliable : std::false_type
{
};

/** @brief Mark a type when multiplying two instances is a valid expression. */
template <typename T>
struct is_time_multipliable<T, std::void_t<decltype(std::declval<T>() * std::declval<T>())>> : std::true_type
{
};

/** @brief Print all free-operator results and verify native operands stay intact.
 * @param label Name shared with the Python comparison case.
 * @param left First duration, passed by value to preserve caller state.
 * @param right Second duration, passed by value to preserve caller state.
 * @param factor Scalar multiplier and divisor.
 * @details Calls namespace free functions explicitly, including both scalar
 *          multiplication orders and both time/double and time/time division.
 */
void emit_case(const char* label, sc_core::sc_time left, sc_core::sc_time right, double factor)
{
  const auto left_ticks = left.value();
  const auto right_ticks = right.value();
  const auto unchanged = [&] { return left.value() == left_ticks && right.value() == right_ticks; };

  std::cout << "TIME_ADD " << label << ' ' << left_ticks << ' ' << right_ticks << ' '
            << sc_core::operator+(left, right).value() << ' ' << int(unchanged()) << '\n';
  std::cout << "TIME_SUB " << label << ' ' << left_ticks << ' ' << right_ticks << ' '
            << sc_core::operator-(left, right).value() << ' ' << int(unchanged()) << '\n';
  std::cout << "TIME_MUL_RIGHT " << label << ' ' << left_ticks << ' ' << std::setprecision(17) << factor << ' '
            << sc_core::operator*(left, factor).value() << ' ' << int(unchanged()) << '\n';
  std::cout << "TIME_MUL_LEFT " << label << ' ' << std::setprecision(17) << factor << ' ' << left_ticks << ' '
            << sc_core::operator*(factor, left).value() << ' ' << int(unchanged()) << '\n';
  std::cout << "TIME_DIV_DOUBLE " << label << ' ' << left_ticks << ' ' << std::setprecision(17) << factor << ' '
            << sc_core::operator/(left, factor).value() << ' ' << int(unchanged()) << '\n';
  std::cout << "TIME_RATIO " << label << ' ' << left_ticks << ' ' << right_ticks << ' ' << std::setprecision(17)
            << sc_core::operator/(left, right) << ' ' << int(unchanged()) << '\n';
  std::cout << "TIME_MOD " << label << ' ' << left_ticks << ' ' << right_ticks << ' '
            << sc_core::operator%(left, right).value() << ' ' << int(unchanged()) << '\n';
}
}  // namespace

/** @brief Compare the seven native arithmetic declarations with Python operators.
 * @details All durations and divisors are positive; zero modulo and scalar
 *          division are omitted because native integer arithmetic cannot safely
 *          evaluate those invalid cases.
 */
int sc_main(int, char**)
{
  static_assert(!is_time_multipliable<sc_core::sc_time>::value, "time * time must remain invalid");

  emit_case("basic", sc_core::sc_time::from_value(1001), sc_core::sc_time::from_value(250), 1.5);
  emit_case("fractional", sc_core::sc_time(0.625, sc_core::SC_NS), sc_core::sc_time(0.125, sc_core::SC_NS), 1.25);
  emit_case("large", sc_core::sc_time::from_value((sc_dt::uint64(1) << 52) + 128),
            sc_core::sc_time::from_value((sc_dt::uint64(1) << 50) + 64), 1.5);
  std::cout << "TIME_PRODUCT_INVALID 1\n";
  return 0;
}
