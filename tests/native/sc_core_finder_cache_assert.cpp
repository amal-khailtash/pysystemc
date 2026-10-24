/** @file sc_core_finder_cache_assert.cpp
 * @brief Compare same-port cache assertion and release behavior.
 *
 * This source is run only in an isolated subprocess with core dumps disabled.
 */
#include <sys/resource.h>

#include <csignal>
#include <iostream>
#include <systemc>

namespace
{
using wide_value = sc_dt::sc_biguint<137>;
using wide_if = sc_core::sc_signal_in_if<wide_value>;

#if defined(NDEBUG) && !defined(SC_ENABLE_ASSERTIONS)
constexpr bool finder_assertions_enabled = false;
#else
constexpr bool finder_assertions_enabled = true;
#endif

/** @brief Own two bound ports that must never share an event-finder cache. */
struct finder_cache_model : sc_core::sc_module
{
  sc_core::sc_in<wide_value> first_port;
  sc_core::sc_in<wide_value> second_port;
  sc_core::sc_signal<wide_value> first_source;
  sc_core::sc_signal<wide_value> second_source;

  /** @brief Bind each port to a distinct interface. */
  explicit finder_cache_model(sc_core::sc_module_name name)
    : sc_module(name)
    , first_port("first_port")
    , second_port("second_port")
    , first_source("first_source")
    , second_source("second_source")
  {
    first_port.bind(first_source);
    second_port.bind(second_source);
  }
};
}  // namespace

/** @brief Hit the cached_create same-port assertion after one successful call. */
int sc_main(int, char**)
{
  rlimit core_limit{0, 0};
  setrlimit(RLIMIT_CORE, &core_limit);
  std::signal(SIGABRT, SIG_DFL);

  finder_cache_model model("finder_cache_assert");
  sc_core::sc_event_finder* cache = nullptr;
  sc_core::sc_event_finder& first =
    sc_core::sc_event_finder::cached_create<wide_if>(cache, model.first_port, &wide_if::value_changed_event);
  const bool first_lookup =
    &first.port() == &model.first_port && &first.find_event() == &model.first_source.value_changed_event();
  std::cout << "FINDER_CACHE_ASSERT_CONFIG " << (finder_assertions_enabled ? "enabled" : "disabled") << std::endl;
  std::cout << "FINDER_CACHE_ASSERT_ENTER" << std::endl;
  sc_core::sc_event_finder& second =
    sc_core::sc_event_finder::cached_create<wide_if>(cache, model.second_port, &wide_if::value_changed_event);
  if (finder_assertions_enabled)
  {
    std::cout << "FINDER_CACHE_ASSERT_RETURNED" << std::endl;
    return 2;
  }
  const bool same_finder = &first == &second;
  const bool retains_first_port = &second.port() == &model.first_port;
  const bool retains_first_interface = &second.find_event() == &model.first_source.value_changed_event();
  std::cout << "FINDER_CACHE_ASSERT_RELEASE\t" << same_finder << '\t' << retains_first_port << '\t'
            << retains_first_interface << std::endl;
  return first_lookup && same_finder && retains_first_port && retains_first_interface ? 0 : 1;
}
