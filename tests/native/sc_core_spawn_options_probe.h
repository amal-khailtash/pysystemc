/** @file sc_core_spawn_options_probe.h
 * @brief Test-only access to protected process options for public API checks.
 */
#ifndef PYSYSTEMC_SC_CORE_SPAWN_OPTIONS_PROBE_H
#define PYSYSTEMC_SC_CORE_SPAWN_OPTIONS_PROBE_H

#include <systemc>

namespace spawn_options_probe
{
/** @brief Expose option storage while retaining the native public API.
 * @details Events, ports, finders, interfaces, and reset sources are borrowed
 * by the base options. The reset wrappers are owned by the options object.
 * Spawned processes copy their reset specification during registration, so
 * the options must remain alive through that call. No Python callback is
 * stored here; callers must retain callbacks for each process lifetime and
 * obey the native callback's GIL requirements.
 */
struct Options : sc_core::sc_spawn_options
{
  inline static int destroyed = 0;

  /** @brief Construct empty native spawn options. */
  Options() = default;

  /** @brief Count derived destruction before the base releases reset specs. */
  ~Options()
  {
    ++destroyed;
  }

  /** @brief Return the number of owned synchronous/asynchronous reset specs. */
  std::size_t reset_count() const
  {
    return m_resets.size();
  }

  /** @brief Return the number of borrowed event sensitivities. */
  std::size_t event_count() const
  {
    return m_sensitive_events.size();
  }

  /** @brief Return the number of borrowed event-finder sensitivities. */
  std::size_t finder_count() const
  {
    return m_sensitive_event_finders.size();
  }

  /** @brief Return the number of borrowed interface sensitivities. */
  std::size_t interface_count() const
  {
    return m_sensitive_interfaces.size();
  }

  /** @brief Return the number of borrowed port sensitivities. */
  std::size_t port_count() const
  {
    return m_sensitive_port_bases.size();
  }

  /** @brief Read the native no-initialization policy. */
  bool no_initialization() const
  {
    return m_dont_initialize;
  }

  /** @brief Read the native configured coroutine stack size. */
  int stack_size() const
  {
    return m_stack_size;
  }
};
}  // namespace spawn_options_probe

#endif  // PYSYSTEMC_SC_CORE_SPAWN_OPTIONS_PROBE_H
