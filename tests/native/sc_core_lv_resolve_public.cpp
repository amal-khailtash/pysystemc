/** @file sc_core_lv_resolve_public.cpp
 * @brief Native reference for the public sc_lv_resolve declaration family.
 */
#include <array>
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

namespace
{
/** @brief Resolve live driver values and report result/input identity checks.
 * @tparam W Width of every logic vector in the call.
 * @param label Record prefix identifying this case.
 * @param driver_strings Nonempty vector of same-width four-state values.
 * @details sc_lv_resolve asserts for an empty vector. The local driver array
 *          owns every pointer for the duration of the call, and the helper
 *          writes into caller-owned result storage without retaining pointers.
 */
template <int W, std::size_t N>
void emit_resolution(const char* label, const std::array<std::string, N>& driver_strings)
{
  std::array<sc_dt::sc_lv<W>, N> drivers;
  std::vector<sc_dt::sc_lv<W>*> pointers;
  std::array<std::string, N> original_values;
  std::array<const sc_dt::sc_lv<W>*, N> original_addresses;
  for (std::size_t i = 0; i < N; ++i)
  {
    drivers[i] = driver_strings[i].c_str();
    original_values[i] = drivers[i].to_string();
    original_addresses[i] = &drivers[i];
    pointers.push_back(&drivers[i]);
  }

  sc_dt::sc_lv<W> result("X");
  const sc_dt::sc_lv<W>* const original_result_address = &result;
  sc_core::sc_lv_resolve<W>::resolve(result, pointers);
  bool addresses_preserved = &result == original_result_address;
  bool pointers_preserved = pointers.size() == N;
  bool inputs_unchanged = true;
  for (std::size_t i = 0; i < N; ++i)
  {
    pointers_preserved = pointers_preserved && pointers[i] == original_addresses[i];
    inputs_unchanged = inputs_unchanged && drivers[i].to_string() == original_values[i];
  }
  std::cout << label << ' ' << result.to_string() << ' ' << int(addresses_preserved && pointers_preserved) << ' '
            << int(inputs_unchanged) << '\n';
}
}  // namespace

/** @brief Emit all public resolver cases used by the Python comparison.
 * @details Each case uses a nonempty vector of live same-width driver values.
 */
int sc_main(int, char**)
{
  const std::array<char, 4> symbols = {'0', '1', 'X', 'Z'};
  for (char left : symbols)
  {
    for (char right : symbols)
    {
      const std::array<std::string, 2> values = {std::string(1, left), std::string(1, right)};
      std::array<sc_dt::sc_lv<1>, 2> drivers = {sc_dt::sc_lv<1>(values[0].c_str()), sc_dt::sc_lv<1>(values[1].c_str())};
      std::vector<sc_dt::sc_lv<1>*> pointers = {&drivers[0], &drivers[1]};
      sc_dt::sc_lv<1> result("X");
      sc_core::sc_lv_resolve<1>::resolve(result, pointers);
      std::cout << "RESOLVE_PAIR " << left << ' ' << right << ' ' << result.to_string() << '\n';
    }
  }

  std::string singleton;
  while (singleton.size() < 137) singleton += "01XZ";
  singleton.resize(137);
  emit_resolution<137>("RESOLVE_SINGLETON", std::array<std::string, 1>{singleton});
  emit_resolution<4>("RESOLVE_MULTI", std::array<std::string, 3>{"01XZ", "10Z1", "ZZ0Z"});

  std::array<std::string, 3> wide_values;
  const std::array<std::string, 3> patterns = {"01XZ", "X0Z1", "ZZ101"};
  for (std::size_t i = 0; i < patterns.size(); ++i)
  {
    while (wide_values[i].size() < 137) wide_values[i] += patterns[i];
    wide_values[i].resize(137);
  }
  emit_resolution<137>("RESOLVE_WIDE", wide_values);
  return 0;
}
