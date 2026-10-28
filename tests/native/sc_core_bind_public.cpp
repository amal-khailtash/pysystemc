/** @file sc_core_bind_public.cpp
 * @brief Native reference for binding value/reference and process semantics.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <functional>
#include <iostream>
#include <systemc>
#include <utility>
#include <vector>

/** @brief Native state copied or borrowed by a binding. */
struct BindState
{
  int value = 10;
};
/** @brief Native functor with mutable copy-owned state. */
struct BindAdder
{
  int offset = 5;
  int operator()(int a, int b)
  {
    return offset + a + b;
  }
};
/** @brief Sum two bound or supplied native arguments. */
int bind_sum(int a, int b)
{
  return a + b;
}
/** @brief Read copied or reference-wrapped state. */
int bind_state(const BindState& state, int add)
{
  return state.value + add;
}
/** @brief Mutate reference-wrapped state through a void native callable. */
void bind_store(BindState& state, int value)
{
  state.value = value;
}
/** @brief Return borrowed state from a binding. */
BindState& bind_reference(BindState& state)
{
  return state;
}

/** @brief Count cleanup for ordinary return and native reset unwind. */
struct BindCleanup
{
  std::vector<sc_dt::uint64>& times;
  ~BindCleanup()
  {
    times.push_back(sc_core::sc_time_stamp().value());
  }
};

/** @brief Run the native binding and reset timeline reference. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace std::placeholders;
  auto simple = sc_bind(bind_sum, 3, 4);
  const auto const_simple = sc_bind(bind_sum, 3, 4);
  if (const_simple() != 7) return 4;
  auto reordered = sc_bind(bind_sum, _2, _1);
  auto duplicated = sc_bind(bind_sum, _1, _1);
  auto nested = sc_bind(bind_sum, sc_bind(bind_sum, _1, 2), 3);
  BindAdder functor;
  auto copied_functor = sc_bind(functor, 1, 2);
  functor.offset = 100;
  BindState state;
  auto copied = sc_bind(bind_state, state, 1);
  auto borrowed = sc_bind(bind_state, sc_ref(state), 1);
  auto const_borrowed = sc_bind(bind_state, sc_cref(state), 1);
  auto reference = sc_bind(bind_reference, sc_ref(state));
  state.value = 20;
  if (const_borrowed() != 21 || &reference() != &state) return 2;
  std::cout << "BIND_VALUES " << simple() << ' ' << reordered(2, 9) << ' ' << duplicated(6) << ' ' << nested(8) << ' '
            << copied_functor() << ' ' << copied() << ' ' << borrowed() << '\n';
  auto store = sc_bind(bind_store, sc_ref(state), _1);
  store(29);
  if (state.value != 29) return 5;
  std::vector<std::pair<int, sc_dt::uint64>> visits;
  std::vector<sc_dt::uint64> cleanup;
  std::function<int(int)> callback = [&](int value)
  {
    visits.emplace_back(value, sc_time_stamp().value());
    BindCleanup scope{cleanup};
    wait(1, SC_NS);
    return value + 1;
  };
  auto bound = sc_bind(callback, 17);
  auto handle = sc_spawn(bound, "bound_worker");
  sc_spawn(
    [&]
    {
      wait(0.25, SC_NS);
      handle.reset();
    },
    "bound_controller");
  sc_start(2, SC_NS);
  if (!handle.terminated() || visits.size() != 2 || cleanup.size() != 2) return 3;
  std::cout << "BIND_PROCESS";
  for (const auto& [value, time] : visits) std::cout << ' ' << value << ':' << time;
  for (auto time : cleanup) std::cout << ' ' << time;
  std::cout << "\nBIND_OK\n";
  sc_stop();
  return 0;
}
