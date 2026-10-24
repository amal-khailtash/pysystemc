/** @file sc_core_reference_wrappers.cpp
 * @brief Native reference for arbitrary sc_ref/sc_cref targets and bound spawn.
 * @details Reference wrappers borrow their C++ objects. This model keeps each
 * target alive through every native consumer and spawned callback.
 */
#include <functional>
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

/** @brief A user-defined native value used to test forwarding templates. */
struct RefCustomState
{
  int value;
  /** @brief Add to the live state. */
  void add(int amount)
  {
    value += amount;
  }
};

using RefWide = sc_dt::sc_biguint<137>;
using RefCustomConst = decltype(sc_core::sc_cref(std::declval<RefCustomState&>()));
using RefCustomConstValue = decltype(std::declval<RefCustomConst>().get());
static_assert(std::is_const_v<std::remove_reference_t<RefCustomConstValue>>);
static_assert(!std::is_assignable_v<RefCustomConstValue, RefCustomState>);
using RefWideConst = decltype(sc_core::sc_cref(std::declval<RefWide&>()));
static_assert(std::is_const_v<std::remove_reference_t<decltype(std::declval<RefWideConst>().get())>>);

/** @brief Consume a mutable wrapper through a native reference-wrapper API. */
int consume_custom(std::reference_wrapper<RefCustomState> state, int amount)
{
  state.get().add(amount);
  return state.get().value;
}

/** @brief Assign a member-bound increment to test wrapper copies in std::bind. */
int invoke_bound_custom(std::reference_wrapper<RefCustomState> state, int amount)
{
  auto bound = sc_core::sc_bind(&RefCustomState::add, state, amount);
  bound();
  return state.get().value;
}

/** @brief Assign a 137-bit value through a mutable reference wrapper. */
void assign_wide(std::reference_wrapper<RefWide> value, unsigned int low)
{
  RefWide expected = 1;
  expected <<= 136;
  expected += low;
  value.get() = expected;
}

/** @brief Read a 137-bit value only through its const wrapper. */
bool wide_matches(std::reference_wrapper<const RefWide> value, unsigned int low)
{
  RefWide expected = 1;
  expected <<= 136;
  expected += low;
  return value.get() == expected;
}

std::vector<std::string> bound_records;

/** @brief Run a delayed native callback bound to a copied reference wrapper. */
void bound_process(std::reference_wrapper<RefCustomState> state, int amount)
{
  bound_records.push_back("start@" + std::to_string(sc_core::sc_time_stamp().value()) + "=" +
                          std::to_string(state.get().value));
  sc_core::wait(2, sc_core::SC_NS);
  state.get().add(amount);
  bound_records.push_back("end@" + std::to_string(sc_core::sc_time_stamp().value()) + "=" +
                          std::to_string(state.get().value));
}

/** @brief Build and spawn a native std::bind callback with borrowed state. */
sc_core::sc_process_handle spawn_bound(std::reference_wrapper<RefCustomState> state, int amount)
{
  auto callback = sc_core::sc_bind(&bound_process, state, amount);
  return sc_core::sc_spawn(callback, "bound_reference_callback");
}

/** @brief Verify native wrappers, arbitrary types and callback results. */
int sc_main(int, char**)
{
  RefCustomState state{10};
  auto ref = sc_core::sc_ref(state);
  auto ref_copy = ref;
  auto cref = sc_core::sc_cref(state);
  auto cref_copy = cref;
  if (&ref.get() != &state || &ref_copy.get() != &state || &cref.get() != &state || &cref_copy.get() != &state)
    return 1;

  ref.get().value = 12;
  const int consumed = consume_custom(ref_copy, 3);
  const int bound = invoke_bound_custom(ref, 2);
  if (cref.get().value != 17 || consumed != 15 || bound != 17) return 2;

  RefWide wide = 5;
  auto wide_ref = sc_core::sc_ref(wide);
  auto wide_cref = sc_core::sc_cref(wide);
  if (&wide_ref.get() != &wide || &wide_cref.get() != &wide) return 3;
  assign_wide(wide_ref, 7);
  if (!wide_matches(wide_cref, 7)) return 4;

  const auto handle = spawn_bound(ref_copy, 5);
  if (!handle.valid()) return 5;
  sc_core::sc_start();
  if (state.value != 22 || bound_records != std::vector<std::string>{"start@0=17", "end@2000=22"}) return 6;

  std::cout << "REF_CUSTOM_REFERENCE value=" << state.value << " address=1 copies=1 const=17 consumed=" << consumed
            << " bound=" << bound << "\n";
  std::cout << "REF_WIDE_REFERENCE width=137 address=1 low=7 high=1\n";
  std::cout << "REF_BOUND_REFERENCE " << bound_records[0] << " " << bound_records[1] << "\n";
  return 0;
}
