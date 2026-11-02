/** @file hash_constructor_defaults.cpp
 * @brief Native constructor-default oracle for sc_phash_base.
 */
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "hash_constructor_defaults_probe.h"

static_assert(std::is_same_v<decltype(sc_core::PHASH_DEFAULT_INIT_TABLE_SIZE), const int>);
static_assert(std::is_same_v<decltype(sc_core::PHASH_DEFAULT_MAX_DENSITY), const int>);
static_assert(std::is_same_v<decltype(sc_core::PHASH_DEFAULT_GROW_FACTOR), const double>);
static_assert(std::is_same_v<decltype(sc_core::PHASH_DEFAULT_REORDER_FLAG), const bool>);

namespace
{
/** @brief Require matching constructor configuration and keep release builds checked. */
void require_config(const hash_constructor_defaults_probe::Probe& table, int bins, int density, double growth,
                    bool reorder)
{
  if (table.bins() != bins || table.density() != density || table.growth() != growth || table.reorder() != reorder)
    throw std::runtime_error("sc_phash_base constructor configuration mismatch");
}

/** @brief Emit the stable four-field record consumed by the Python comparison. */
void emit_config(const char* label, const hash_constructor_defaults_probe::Probe& table)
{
  std::cout << "HASH_DEFAULTS " << label << ' ' << table.bins() << ' ' << table.density() << ' ' << table.growth()
            << ' ' << table.reorder() << '\n';
}
}  // namespace

/** @brief Compare inherited constructor defaults with explicit and custom values. */
int sc_main(int, char**)
{
  using hash_constructor_defaults_probe::Probe;
  Probe omitted;
  Probe explicit_defaults(nullptr, 11, 5, 2.0, true);
  Probe custom(nullptr, 17, 3, 1.5, false);

  require_config(omitted, 11, 5, 2.0, true);
  require_config(explicit_defaults, 11, 5, 2.0, true);
  require_config(custom, 17, 3, 1.5, false);
  if (sc_core::PHASH_DEFAULT_INIT_TABLE_SIZE != 11 || sc_core::PHASH_DEFAULT_MAX_DENSITY != 5 ||
      sc_core::PHASH_DEFAULT_GROW_FACTOR != 2.0 || !sc_core::PHASH_DEFAULT_REORDER_FLAG)
    throw std::runtime_error("sc_phash_base declaration constants changed");

  emit_config("omitted", omitted);
  emit_config("explicit", explicit_defaults);
  emit_config("custom", custom);
  std::cout << "HASH_CONSTANTS " << sc_core::PHASH_DEFAULT_INIT_TABLE_SIZE << ' ' << sc_core::PHASH_DEFAULT_MAX_DENSITY
            << ' ' << sc_core::PHASH_DEFAULT_GROW_FACTOR << ' ' << sc_core::PHASH_DEFAULT_REORDER_FLAG << '\n';
  return 0;
}
