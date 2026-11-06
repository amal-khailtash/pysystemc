/** @file hash_constructor_defaults_probe.h
 * @brief Read inherited sc_phash_base constructor configuration for an oracle.
 */
#ifndef HASH_CONSTRUCTOR_DEFAULTS_PROBE_H
#define HASH_CONSTRUCTOR_DEFAULTS_PROBE_H

#include <systemc>

namespace hash_constructor_defaults_probe
{
/** @brief Expose protected hash configuration while inheriting native constructors.
 * @details The base owns its bucket storage and borrows its default-value pointer.
 * This probe installs no callbacks, so it has no callback lifetime or GIL
 * requirements. Getter calls are synchronous; Python callers hold the GIL
 * during these value reads.
 */
class Probe : public sc_core::sc_phash_base
{
 public:
  /** @brief Inherit the complete native constructor overload set unchanged. */
  using sc_core::sc_phash_base::sc_phash_base;

  /** @brief Return the normalized allocated bucket count. */
  int bins() const
  {
    return num_bins;
  }

  /** @brief Return the configured maximum entries per bucket. */
  int density() const
  {
    return max_density;
  }

  /** @brief Return the configured table growth multiplier. */
  double growth() const
  {
    return grow_factor;
  }

  /** @brief Return whether the table reorders entries after access. */
  bool reorder() const
  {
    return reorder_flag != 0;
  }
};

/** @brief Add one ordinary native constructor beside inherited hash constructors.
 * @details The inherited base owns bucket storage and default configuration.
 * The marker is a value member owned by this object. Construction is
 * synchronous, invokes no Python callback and has no GIL requirement.
 */
class ProbeWithTag : public Probe
{
 public:
  /** @brief Inherit every constructor exposed by Probe. */
  using Probe::Probe;

  /** @brief Construct with a native-only marker overload.
   * @param marker Value returned by tag().
   */
  explicit ProbeWithTag(int marker) : Probe(), marker_(marker)
  {
  }

  /** @brief Return the marker supplied to the native-only constructor. */
  int tag() const
  {
    return marker_;
  }

 private:
  int marker_ = 0;
};
}  // namespace hash_constructor_defaults_probe

#endif  // HASH_CONSTRUCTOR_DEFAULTS_PROBE_H
