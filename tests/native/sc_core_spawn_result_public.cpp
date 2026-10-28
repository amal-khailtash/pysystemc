/** @file sc_core_spawn_result_public.cpp
 * @brief Native reference for arbitrary typed dynamic-process results.
 * @details Result lvalues and functors outlive their processes. Native stack
 * guards record reset/kill cleanup; no Python or GIL is involved in this model.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

/** @brief Custom owning native result matching the Python-defined test type. */
struct SpawnPayload
{
  int value = -7;
};

/** @brief Record callback cleanup under normal return and native unwind. */
struct Cleanup
{
  std::vector<std::string>& records;
  std::string name;
  ~Cleanup()
  {
    records.push_back(name);
  }
};

/** @brief Run one bounded native result-assignment or callback-error scenario. */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  if (argc > 1 && std::string(argv[1]) == "error")
  {
    int result = -9;
    sc_spawn(&result, std::function<int()>([]() -> int { throw std::runtime_error("typed result failure"); }),
             "failed_result");
    try
    {
      sc_start(1, SC_NS);
    }
    catch (const sc_report& error)
    {
      if (error.get_id() != 549 || result != -9) return 3;
      std::cout << "SPAWN_RESULT_ERROR " << error.get_id() << ' ' << result << '\n';
      return 0;
    }
    return 2;
  }
  int integer = -1, method_result = 0, reset_result = -1, kill_result = -1;
  double real = -1;
  sc_dt::sc_biguint<137> wide;
  SpawnPayload payload;
  sc_time time_result;
  std::vector<sc_dt::uint64> entries, method_visits;
  std::vector<std::string> cleanup;
  sc_spawn(&integer,
           std::function<int()>(
             []
             {
               wait(1, SC_NS);
               return 23;
             }),
           "integer");
  sc_spawn(&real, std::function<double()>([] { return 1.25; }), "real");
  sc_spawn(&wide,
           std::function<sc_dt::sc_biguint<137>()>(
             [] { return sc_dt::sc_biguint<137>("0x10000000000000000000000000000000011"); }),
           "wide");
  sc_spawn(&payload, std::function<SpawnPayload()>([] { return SpawnPayload{71}; }), "payload");
  SpawnPayload native_result;
  sc_spawn(
    [&]
    {
      wait(0.75, SC_NS);
      native_result.value = 89;
    },
    "native_functor");
  sc_spawn(&time_result, std::function<sc_time()>([] { return sc_time(3, SC_NS); }), "time");
  sc_event tick("method_tick");
  sc_spawn_options options;
  options.spawn_method();
  options.dont_initialize();
  options.set_sensitivity(&tick);
  sc_spawn(&method_result,
           std::function<int()>(
             [&]
             {
               method_visits.push_back(sc_time_stamp().value());
               return static_cast<int>(method_visits.size()) * 10;
             }),
           "method", &options);
  auto reset_handle = sc_spawn(&reset_result,
                               std::function<int()>(
                                 [&]
                                 {
                                   entries.push_back(sc_time_stamp().value());
                                   Cleanup guard{cleanup, "reset"};
                                   wait(1, SC_NS);
                                   return 37;
                                 }),
                               "reset");
  auto kill_handle = sc_spawn(&kill_result,
                              std::function<int()>(
                                [&]
                                {
                                  Cleanup guard{cleanup, "kill"};
                                  wait(10, SC_NS);
                                  return 99;
                                }),
                              "kill");
  sc_spawn(
    [&]
    {
      wait(0.25, SC_NS);
      reset_handle.reset();
      kill_handle.kill();
      wait(0.25, SC_NS);
      tick.notify(SC_ZERO_TIME);
      wait(1, SC_NS);
      tick.notify(SC_ZERO_TIME);
    },
    "controller");
  sc_start(2, SC_NS);
  std::cout << "SPAWN_RESULTS " << integer << ' ' << real << ' ' << wide[136].to_bool() << ' ' << wide.to_uint64()
            << ' ' << payload.value << ' ' << time_result << '\n';
  std::cout << "SPAWN_RESULT_NATIVE " << native_result.value << '\n';
  std::cout << "SPAWN_RESULT_METHOD " << method_result;
  for (auto value : method_visits) std::cout << ' ' << value;
  std::cout << '\n' << "SPAWN_RESULT_UNWIND " << reset_result << ' ' << kill_result;
  for (auto value : entries) std::cout << ' ' << value;
  std::cout << ' ';
  for (std::size_t index = 0; index < cleanup.size(); ++index) std::cout << (index ? "|" : "") << cleanup[index];
  std::cout << '\n';
  sc_stop();
  return 0;
}
