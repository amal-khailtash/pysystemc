/** @file sc_core_vpool_public.cpp
 * @brief Native reference for generic and SystemC-specialized temporary pools.
 */
#include <iostream>
#include <vector>

#include "sc_core_vpool_probe.h"

using namespace vpool_probe;

/** @brief Print native pool size, alignment, wrap indices and reset reuse. */
template <class T>
void emit_pool(const char* name, sc_core::sc_vpool<T>& pool)
{
  std::vector<std::ptrdiff_t> indices;
  T* base = pool.allocate();
  indices.push_back(slot_index(base, base));
  for (std::size_t index = 1; index < pool_capacity + 2; ++index) indices.push_back(slot_index(base, pool.allocate()));
  pool.reset();
  const auto reset_index = slot_index(base, pool.allocate());

  std::cout << "VPOOL|" << name << '|' << pool.size() << '|' << slot_aligned(base) << '|' << slot_stride<T>() << '|';
  for (std::size_t index = 0; index < indices.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << indices[index];
  }
  std::cout << '|' << reset_index << '\n';
}

/** @brief Exercise sc_vpool's internal new T[] constructor for one type. */
template <class T>
void emit_allocated_pool(const char* name)
{
  allocated_pool<T> owner;
  emit_pool<T>(name, owner.pool());
}

/** @brief Exercise sc_vpool's caller-owned T[8] storage constructor. */
template <class T>
void emit_external_pool(const char* name)
{
  external_pool<T> owner;
  emit_pool<T>(name, owner.pool());
}

/** @brief Compare every source specialization and both storage ownership paths. */
int sc_main(int, char**)
{
  emit_allocated_pool<int>("int");
  emit_allocated_pool<user_slot>("user_slot");
  emit_allocated_pool<sc_dt::sc_unsigned>("sc_unsigned");
  emit_allocated_pool<sc_dt::sc_concatref>("sc_concatref");
  emit_allocated_pool<sc_dt::sc_unsigned_bitref>("sc_unsigned_bitref");
  emit_allocated_pool<sc_dt::sc_uint_subref>("sc_uint_subref");
  emit_allocated_pool<sc_dt::sc_signed_bitref>("sc_signed_bitref");
  emit_allocated_pool<sc_dt::sc_int_bitref>("sc_int_bitref");
  emit_allocated_pool<sc_dt::sc_int_subref>("sc_int_subref");
  emit_allocated_pool<sc_dt::sc_signed_subref>("sc_signed_subref");
  emit_allocated_pool<sc_dt::sc_uint_bitref>("sc_uint_bitref");
  emit_allocated_pool<sc_dt::sc_concat_bool>("sc_concat_bool");
  emit_allocated_pool<sc_dt::sc_unsigned_subref>("sc_unsigned_subref");

  reset_lifetime_counts();
  {
    allocated_pool<lifetime_slot> owner;
    emit_pool<lifetime_slot>("lifetime_slot", owner.pool());
  }
  std::cout << "VPOOL_ALLOCATED_LIFETIME|" << lifetime_constructor_count() << '|' << lifetime_destructor_count()
            << '\n';

  emit_external_pool<int>("int");
  emit_external_pool<user_slot>("user_slot");
  reset_lifetime_counts();
  {
    external_pool<lifetime_slot> owner;
    emit_pool<lifetime_slot>("lifetime_slot", owner.pool());
  }
  std::cout << "VPOOL_EXTERNAL_LIFETIME|" << lifetime_constructor_count() << '|' << lifetime_destructor_count() << '\n';
  return 0;
}
