/** @file sc_dt_integer_bitrefs_public.cpp
 * @brief Independent native reference for public integer bit-reference APIs.
 * @details Views borrow their numeric owners and native pool slots; all owners,
 * streams, and temporary copied proxy shells are scoped within this process.
 */
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "sc_dt_integer_bitrefs_probe.h"

namespace
{

/** @brief Emit one tab-separated native observation for Python comparison. */
template <typename... Fields>
void emit(const char* family, int width, const char* operation, const Fields&... fields)
{
  std::cout << "IBR\t" << family << '\t' << width << '\t' << operation;
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

/** @brief Format an owner's complete value in decimal without narrowing it. */
template <typename Owner>
std::string owner_value(const Owner& owner)
{
  return owner.to_string(sc_dt::SC_DEC);
}

/** @brief Exercise one lvalue and const bit-reference pair from a live owner.
 * @details The owner remains alive until every borrowed view and copied shell
 * is destroyed. The mutable view is always obtained through owner selection;
 * this deliberately avoids constructing a pooled proxy directly.
 */
template <typename Owner, typename ConstView>
void exercise_owner(const char* family, Owner& owner, int width, int index, const ConstView& read_view)
{
  using MutableView = std::remove_reference_t<decltype(owner[index])>;
  MutableView& view = owner[index];
  std::unique_ptr<MutableView> mutable_copy(new MutableView(view));
  using ReadView = std::remove_cv_t<std::remove_reference_t<ConstView>>;
  std::unique_ptr<ReadView> read_copy(new ReadView(read_view));

  emit(family, width, "access", view.length(), read_view.length(), static_cast<int>(view.value()),
       static_cast<int>(read_view.value()));
  emit(family, width, "copy_ctor", static_cast<int>(mutable_copy->value()), static_cast<int>(read_copy->value()));
  mutable_copy.reset();
  read_copy.reset();

  sc_dt::sc_digit data[2]{0, 0};
  sc_dt::sc_digit control[2]{0, 0};
  const bool data_present = read_view.concat_get_data(data, 1);
  const bool control_present = read_view.concat_get_ctrl(control, 1);
  std::ostringstream printed;
  read_view.print(printed);
  emit(family, width, "read", static_cast<int>(read_view.value()), static_cast<int>(read_view.to_bool()),
       static_cast<unsigned long long>(read_view), static_cast<int>(!read_view), static_cast<int>(~read_view),
       read_view.concat_length(nullptr), static_cast<unsigned long long>(read_view.concat_get_uint64()),
       static_cast<int>(data_present), data[0], data[1], static_cast<int>(control_present), control[0], control[1],
       printed.str());

  for (const bool bit_value : {false, true})
  {
    owner = bit_value ? (sc_dt::uint64{1} << index) : 0;
    MutableView& truth_view = owner[index];
    emit(family, width, "truth", "mutable", bit_value, static_cast<int>(truth_view.to_bool()),
         static_cast<int>(!truth_view));

    const auto& const_truth_view = sc_dt_integer_bitrefs_probe::const_view_for(owner, index);
    std::unique_ptr<ReadView> const_truth_copy(new ReadView(const_truth_view));
    emit(family, width, "truth", "const", bit_value, static_cast<int>(const_truth_copy->to_bool()),
         static_cast<int>(!*const_truth_copy));
  }
  owner = 0;

#ifdef SC_DT_DEPRECATED
  emit(family, width, "bitwidth", read_view.bitwidth());
#else
  emit(family, width, "bitwidth", "feature-disabled");
#endif

  owner = 0;
  owner[index] = true;
  emit(family, width, "assign_bool", owner_value(owner));
  owner = 0;
  owner[index] = true;
  owner[index - 1] = owner[index];
  emit(family, width, "assign_lvalue", owner_value(owner));
  owner = 0;
  owner[index] = true;
  owner[index - 1] = read_view;
  emit(family, width, "assign_rvalue", owner_value(owner));

  for (const bool operand : {false, true})
  {
    owner = 0;
    owner[index] = true;
    owner[index] &= operand;
    emit(family, width, "and", operand, owner_value(owner));
    owner = 0;
    owner[index] = false;
    owner[index] |= operand;
    emit(family, width, "or", operand, owner_value(owner));
    owner = 0;
    owner[index] = true;
    owner[index] ^= operand;
    emit(family, width, "xor", operand, owner_value(owner));
  }

  for (const char* text : {"0", "1"})
  {
    owner = 0;
    std::istringstream input(text);
    owner[index].scan(input);
    emit(family, width, "scan", text, owner_value(owner));
  }

  owner = 0;
  owner[index].concat_set(sc_dt::int64{8}, 3);
  emit(family, width, "concat_int64", owner_value(owner));
  owner = 0;
  owner[index].concat_set(sc_dt::uint64{8}, 3);
  emit(family, width, "concat_uint64", owner_value(owner));
  owner = 0;
  sc_dt::sc_signed signed_source(16);
  signed_source = 32;
  owner[index].concat_set(signed_source, 5);
  emit(family, width, "concat_signed", owner_value(owner));
  owner = 0;
  sc_dt::sc_unsigned unsigned_source(16);
  unsigned_source = 32;
  owner[index].concat_set(unsigned_source, 5);
  emit(family, width, "concat_unsigned", owner_value(owner));
}

/** @brief Exercise reusable slot and copied-view lifetime semantics. */
template <typename Owner>
void exercise_pool(const char* family, Owner& owner, int width, std::size_t pool_size)
{
  owner = 1;
  auto& first = owner[0];
  using MutableView = std::remove_reference_t<decltype(first)>;
  const auto& first_read = sc_dt_integer_bitrefs_probe::const_view_for(owner, 0);
  using ReadView = std::remove_cv_t<std::remove_reference_t<decltype(first_read)>>;
  {
    std::unique_ptr<MutableView> copied(new MutableView(first));
    std::unique_ptr<ReadView> copied_read(new ReadView(first_read));
    const auto first_address = reinterpret_cast<std::uintptr_t>(&first);
    const auto first_read_address = reinterpret_cast<std::uintptr_t>(&first_read);
    for (std::size_t index = 0; index + 2 < pool_size; ++index)
    {
      (void)owner[1];
    }
    auto& wrapped = owner[1];
    const auto& wrapped_read = sc_dt_integer_bitrefs_probe::const_view_for(owner, 1);
    const auto wrapped_address = reinterpret_cast<std::uintptr_t>(&wrapped);
    const auto wrapped_read_address = reinterpret_cast<std::uintptr_t>(&wrapped_read);
    emit(family, width, "pool", pool_size, first_address == wrapped_address, first_read_address == wrapped_read_address,
         static_cast<int>(first.value()), static_cast<int>(copied->value()), static_cast<int>(first_read.value()),
         static_cast<int>(copied_read->value()));
    *copied = false;
    emit(family, width, "copied_view_write", owner_value(owner), static_cast<int>(copied_read->value()));
    copied.reset();
    copied_read.reset();
  }
  emit(family, width, "copy_lifetime", owner_value(owner));
}

/** @brief Catch invalid owner indexes while SystemC reports are configured to throw. */
template <typename Owner>
void exercise_errors(const char* family, Owner& owner, int width)
{
  const auto old_actions = sc_dt_integer_bitrefs_probe::throw_errors();
  for (int index : {-1, width})
  {
    owner = 0;
    bool caught = false;
    try
    {
      (void)owner[index];
    }
    catch (const sc_core::sc_report&)
    {
      caught = true;
    }
    emit(family, width, "invalid_index", index, caught, owner_value(owner));
  }
  sc_dt_integer_bitrefs_probe::restore_errors(old_actions);
}

/** @brief Execute selected APIs for a bounded signed and unsigned owner. */
template <int Width>
void exercise_fixed()
{
  sc_dt::sc_int<Width> signed_owner(0);
  const auto& signed_read = sc_dt_integer_bitrefs_probe::const_int_view(signed_owner, 3);
  exercise_owner("sc_dt::sc_int_bitref", signed_owner, Width, 3, signed_read);
  exercise_pool("sc_dt::sc_int_bitref", signed_owner, Width, sc_dt_integer_bitrefs_probe::int_pool_size());
  exercise_errors("sc_dt::sc_int_bitref", signed_owner, Width);

  sc_dt::sc_uint<Width> unsigned_owner(0);
  const auto& unsigned_read = sc_dt_integer_bitrefs_probe::const_uint_view(unsigned_owner, 3);
  exercise_owner("sc_dt::sc_uint_bitref", unsigned_owner, Width, 3, unsigned_read);
  exercise_pool("sc_dt::sc_uint_bitref", unsigned_owner, Width, sc_dt_integer_bitrefs_probe::uint_pool_size());
  exercise_errors("sc_dt::sc_uint_bitref", unsigned_owner, Width);
}

/** @brief Execute selected APIs for arbitrary-width signed and unsigned owners. */
template <int Width>
void exercise_arbitrary()
{
  sc_dt::sc_signed signed_owner(Width);
  signed_owner = 0;
  const auto& signed_read = sc_dt_integer_bitrefs_probe::const_signed_view(signed_owner, 3);
  exercise_owner("sc_dt::sc_signed_bitref", signed_owner, Width, 3, signed_read);
  exercise_pool("sc_dt::sc_signed_bitref", signed_owner, Width, sc_dt_integer_bitrefs_probe::signed_pool_size());
  exercise_errors("sc_dt::sc_signed_bitref", signed_owner, Width);

  sc_dt::sc_unsigned unsigned_owner(Width);
  unsigned_owner = 0;
  const auto& unsigned_read = sc_dt_integer_bitrefs_probe::const_unsigned_view(unsigned_owner, 3);
  exercise_owner("sc_dt::sc_unsigned_bitref", unsigned_owner, Width, 3, unsigned_read);
  exercise_pool("sc_dt::sc_unsigned_bitref", unsigned_owner, Width, sc_dt_integer_bitrefs_probe::unsigned_pool_size());
  exercise_errors("sc_dt::sc_unsigned_bitref", unsigned_owner, Width);
}

}  // namespace

/** @brief Emit native references for bounded and arbitrary integer proxies. */
int sc_main(int, char**)
{
  exercise_fixed<19>();
  exercise_fixed<63>();
  exercise_arbitrary<137>();
  exercise_arbitrary<191>();
  return 0;
}
