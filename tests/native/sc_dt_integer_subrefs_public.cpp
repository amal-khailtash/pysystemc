/** @file sc_dt_integer_subrefs_public.cpp
 * @brief Independent native reference for public integer range proxies.
 * @details Borrowed ranges and copied proxy shells are scoped around live
 * owners. No SystemC kernel state is reset; each executable is one-shot.
 */
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

#include "sc_dt_integer_subrefs_probe.h"

namespace
{

/** @brief Emit one tab-separated native observation for Python comparison. */
template <typename... Fields>
void emit(const char* family, int width, const char* operation, const Fields&... fields)
{
  std::cout << "ISR\t" << family << '\t' << width << '\t' << operation;
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

/** @brief Format an owner's complete value in decimal without narrowing it. */
template <typename Owner>
std::string owner_value(const Owner& owner)
{
  return owner.to_string(sc_dt::SC_DEC);
}

/** @brief Emit all read-only reductions, conversions and text/concat views. */
template <typename ReadView>
void read_signature(const char* family, int width, const char* label, const ReadView& view)
{
  sc_dt::sc_digit digits[2]{0, 0};
  sc_dt::sc_digit controls[2]{0, 0};
  const bool data_present = view.concat_get_data(digits, 0);
  const bool control_present = view.concat_get_ctrl(controls, 0);
  std::ostringstream printed;
  view.print(printed);
  const auto print_fields = [&](auto&&... numeric)
  {
    emit(family, width, "read", label, view.length(), static_cast<int>(view.and_reduce()),
         static_cast<int>(view.nand_reduce()), static_cast<int>(view.or_reduce()), static_cast<int>(view.nor_reduce()),
         static_cast<int>(view.xor_reduce()), static_cast<int>(view.xnor_reduce()), numeric...,
         view.concat_length(nullptr), static_cast<unsigned long long>(view.concat_get_uint64()),
         static_cast<int>(data_present), digits[0], digits[1], static_cast<int>(control_present), controls[0],
         controls[1], view.to_string(sc_dt::SC_DEC), view.to_string(sc_dt::SC_HEX), view.to_string(sc_dt::SC_HEX, true),
         printed.str());
  };
  if constexpr (std::is_same<ReadView, sc_dt::sc_int_subref_r>::value ||
                std::is_same<ReadView, sc_dt::sc_uint_subref_r>::value)
  {
    print_fields(view.to_int(), view.to_uint(), view.to_long(), view.to_ulong(), view.to_int64(), view.to_uint64(),
                 static_cast<int>(view.to_double()), static_cast<unsigned long long>(view.value()),
                 static_cast<unsigned long long>(view));
  }
  else
  {
    const sc_dt::sc_unsigned converted = static_cast<sc_dt::sc_unsigned>(view);
    print_fields(view.to_int(), view.to_uint(), view.to_long(), view.to_ulong(), view.to_int64(), view.to_uint64(),
                 static_cast<int>(view.to_double()), converted.to_string(sc_dt::SC_DEC), converted.length());
  }
  if constexpr (std::is_same<ReadView, sc_dt::sc_int_subref_r>::value ||
                std::is_same<ReadView, sc_dt::sc_uint_subref_r>::value)
  {
    emit(family, width, "bitwidth", label, view.length());
  }
}

/** @brief Check pool wrapping and copy durability with the owner retained. */
template <typename Owner, typename SelectConst>
void exercise_pool(const char* family, Owner& owner, int width, int left, int right, std::size_t pool_size,
                   SelectConst select_const)
{
  owner = 1 << right;
  auto& first = owner(left, right);
  const auto& const_first = select_const(owner, left, right);
  using MutableView = std::remove_reference_t<decltype(first)>;
  using ReadView = std::remove_cv_t<std::remove_reference_t<decltype(const_first)>>;
  std::unique_ptr<MutableView> copied(new MutableView(first));
  std::unique_ptr<ReadView> copied_read(new ReadView(const_first));
  const auto first_address = reinterpret_cast<std::uintptr_t>(&first);
  const auto const_first_address = reinterpret_cast<std::uintptr_t>(&const_first);
  for (std::size_t index = 0; index < pool_size - 2; ++index)
  {
    (void)owner(3, 0);
  }
  auto& wrapped = owner(3, 0);
  const auto& wrapped_read = select_const(owner, 3, 0);
  emit(family, width, "pool", pool_size, first_address == reinterpret_cast<std::uintptr_t>(&wrapped),
       const_first_address == reinterpret_cast<std::uintptr_t>(&wrapped_read), first.length(), copied->length(),
       static_cast<unsigned long long>(first.to_uint64()), static_cast<unsigned long long>(copied->to_uint64()),
       const_first.length(), copied_read->length(), static_cast<unsigned long long>(copied_read->to_uint64()));
  *copied = 0;
  emit(family, width, "copy_write", owner_value(owner), static_cast<unsigned long long>(copied_read->to_uint64()));
  copied.reset();
  copied_read.reset();
  emit(family, width, "copy_lifetime", owner_value(owner));
}

/** @brief Exercise invalid range reports while preserving process actions. */
template <typename Owner>
void exercise_errors(const char* family, Owner& owner, int width, int left, int right, bool fixed)
{
  const auto old_actions = sc_dt_integer_subrefs_probe::throw_errors();
  const auto check = [&](int bad_left, int bad_right)
  {
    owner = 0;
    bool caught = false;
    try
    {
      (void)owner(bad_left, bad_right);
    }
    catch (const sc_core::sc_report&)
    {
      caught = true;
    }
    emit(family, width, "invalid_range", bad_left, bad_right, caught, owner_value(owner));
  };
  check(width, 0);
  check(1, -1);
  if (fixed)
  {
    check(right, left);
  }
  sc_dt_integer_subrefs_probe::restore_errors(old_actions);
}

/** @brief Exercise one range's access, conversion, mutation, scanning and concat APIs. */
template <typename Owner, typename SelectConst>
void exercise_owner(const char* family, Owner& owner, int width, int left, int right, std::size_t pool_size,
                    SelectConst select_const)
{
  owner = 0;
  constexpr bool arbitrary =
    std::is_same<Owner, sc_dt::sc_signed>::value || std::is_same<Owner, sc_dt::sc_unsigned>::value;
  constexpr bool fixed = !arbitrary;
  const int high_bit = arbitrary ? width - 1 : width - 2;
  owner[high_bit] = true;
  owner(left, right) = 0xA6;
  using MutableView = std::remove_reference_t<decltype(owner(left, right))>;
  using ReadView = std::remove_cv_t<std::remove_reference_t<decltype(select_const(owner, left, right))>>;
  auto& mutable_view = owner(left, right);
  const auto& const_shell = select_const(owner, left, right);
  std::unique_ptr<MutableView> copied_mutable(new MutableView(mutable_view));
  std::unique_ptr<ReadView> copied_read(new ReadView(const_shell));
  emit(family, width, "access", left, right, owner_value(owner), mutable_view.length(),
       mutable_view.to_string(sc_dt::SC_DEC), copied_read->length(), copied_read->to_string(sc_dt::SC_DEC));
  emit(family, width, "copy_ctor", copied_mutable->to_string(sc_dt::SC_DEC), copied_read->to_string(sc_dt::SC_DEC));
  read_signature(family, width, "forward", *copied_read);
  if constexpr (std::is_same<Owner, sc_dt::sc_signed>::value)
  {
    const auto initialized = sc_dt_integer_subrefs_probe::initialized_signed_range(owner, left, right);
    read_signature(family, width, "protected_initialize", initialized);
  }
  else if constexpr (std::is_same<Owner, sc_dt::sc_unsigned>::value)
  {
    const auto initialized = sc_dt_integer_subrefs_probe::initialized_unsigned_range(owner, left, right);
    read_signature(family, width, "protected_initialize", initialized);
  }
  copied_mutable.reset();
  copied_read.reset();

  if constexpr (arbitrary)
  {
    auto& reverse_mutable = owner(right, left);
    const auto& reverse_const_shell = select_const(owner, right, left);
    std::unique_ptr<ReadView> reverse_read(new ReadView(reverse_const_shell));
    emit(family, width, "reverse", reverse_mutable.length(),
         static_cast<unsigned long long>(reverse_mutable.to_uint64()), reverse_read->length(),
         reverse_read->to_string(sc_dt::SC_DEC), static_cast<unsigned long long>(reverse_read->concat_get_uint64()));
    reverse_read.reset();
  }

  Owner source_owner(owner);
  source_owner = 0;
  source_owner(left, right) = 0x5A;
  auto& source_mutable = source_owner(left, right);
  const auto& source_const_shell = select_const(source_owner, left, right);
  std::unique_ptr<MutableView> source_mutable_copy(new MutableView(source_mutable));
  std::unique_ptr<ReadView> source_read_copy(new ReadView(source_const_shell));

  const auto assign = [&](const char* operation, const auto& value)
  {
    owner = 0;
    owner(left, right) = value;
    emit(family, width, operation, owner_value(owner));
  };
  assign("assign_mutable", *source_mutable_copy);
  assign("assign_const", *source_read_copy);
  assign("assign_cstr", "173");
  assign("assign_int", static_cast<int>(37));
  assign("assign_uint", static_cast<unsigned int>(37));
  assign("assign_long", static_cast<long>(-13));
  assign("assign_ulong", static_cast<unsigned long>(221));
  assign("assign_int64", sc_dt_integer_subrefs_probe::Int64{-13});
  assign("assign_uint64", sc_dt_integer_subrefs_probe::UInt64{221});
  assign("assign_double", 37.75);

  sc_dt::sc_signed signed_source(16);
  signed_source = 0x35;
  sc_dt::sc_unsigned unsigned_source(16);
  unsigned_source = 0x69;
  sc_dt::sc_int<16> fixed_signed_source(0x47);
  sc_dt::sc_uint<16> fixed_unsigned_source(0x83);
  assign("assign_signed", signed_source);
  assign("assign_unsigned", unsigned_source);
  if constexpr (std::is_same<MutableView, sc_dt::sc_int_subref>::value)
  {
    assign("assign_int_base", static_cast<const sc_dt::sc_int_base&>(fixed_signed_source));
    assign("assign_sc_uint_generic", fixed_unsigned_source);
  }
  else if constexpr (std::is_same<MutableView, sc_dt::sc_uint_subref>::value)
  {
    assign("assign_sc_int_generic", fixed_signed_source);
    assign("assign_uint_base", static_cast<const sc_dt::sc_uint_base&>(fixed_unsigned_source));
  }
  else
  {
    assign("assign_int_base", static_cast<const sc_dt::sc_int_base&>(fixed_signed_source));
    assign("assign_uint_base", static_cast<const sc_dt::sc_uint_base&>(fixed_unsigned_source));
  }
  const auto& generic_source = sc_dt_integer_subrefs_probe::generic_concat_source(signed_source, unsigned_source);
  assign("assign_generic_concat", generic_source);

  if constexpr (std::is_same<MutableView, sc_dt::sc_int_subref>::value ||
                std::is_same<MutableView, sc_dt::sc_uint_subref>::value)
  {
    sc_dt::sc_bv_base bit_source("0000000000101010");
    sc_dt::sc_lv_base logic_source("0000000000011011");
    assign("assign_bv", bit_source);
    assign("assign_lv", logic_source);
  }
  if constexpr (arbitrary)
  {
    using OtherOwner =
      std::conditional_t<std::is_same<Owner, sc_dt::sc_signed>::value, sc_dt::sc_unsigned, sc_dt::sc_signed>;
    OtherOwner other_owner(width);
    other_owner = 0;
    other_owner(left, right) = 0x37;
    const auto& other_shell = [&]() -> const auto&
    {
      if constexpr (std::is_same<Owner, sc_dt::sc_signed>::value)
      {
        return sc_dt_integer_subrefs_probe::const_unsigned_range(other_owner, left, right);
      }
      else
      {
        return sc_dt_integer_subrefs_probe::const_signed_range(other_owner, left, right);
      }
    }
    ();
    assign("assign_cross_const", other_shell);
  }

  for (const char* text : {"0", "1", "173"})
  {
    owner = 0;
    std::istringstream input(text);
    owner(left, right).scan(input);
    emit(family, width, "scan", text, owner_value(owner));
  }
  owner = 0;
  owner(left, right).concat_set(sc_dt_integer_subrefs_probe::Int64{0x35}, 0);
  emit(family, width, "concat_int64", owner_value(owner));
  owner = 0;
  owner(left, right).concat_set(sc_dt_integer_subrefs_probe::UInt64{0x69}, 0);
  emit(family, width, "concat_uint64", owner_value(owner));
  owner = 0;
  owner(left, right).concat_set(signed_source, 0);
  emit(family, width, "concat_signed", owner_value(owner));
  owner = 0;
  owner(left, right).concat_set(unsigned_source, 0);
  emit(family, width, "concat_unsigned", owner_value(owner));

  source_mutable_copy.reset();
  source_read_copy.reset();
  exercise_pool(family, owner, width, left, right, pool_size, select_const);
  exercise_errors(family, owner, width, left, right, fixed);
}

/** @brief Run the bounded signed and unsigned matrix at one template width. */
template <int Width>
void exercise_fixed()
{
  sc_dt::sc_int<Width> signed_owner(0);
  exercise_owner("sc_dt::sc_int_subref", signed_owner, Width, 14, 5, sc_dt_integer_subrefs_probe::int_pool_size(),
                 sc_dt_integer_subrefs_probe::const_int_range);
  sc_dt::sc_uint<Width> unsigned_owner(0);
  exercise_owner("sc_dt::sc_uint_subref", unsigned_owner, Width, 14, 5, sc_dt_integer_subrefs_probe::uint_pool_size(),
                 sc_dt_integer_subrefs_probe::const_uint_range);
}

/** @brief Run the arbitrary-width signed and unsigned matrix at one width. */
template <int Width>
void exercise_arbitrary()
{
  sc_dt::sc_signed signed_owner(Width);
  signed_owner = 0;
  exercise_owner("sc_dt::sc_signed_subref", signed_owner, Width, 14, 5, sc_dt_integer_subrefs_probe::signed_pool_size(),
                 sc_dt_integer_subrefs_probe::const_signed_range);
  sc_dt::sc_unsigned unsigned_owner(Width);
  unsigned_owner = 0;
  exercise_owner("sc_dt::sc_unsigned_subref", unsigned_owner, Width, 14, 5,
                 sc_dt_integer_subrefs_probe::unsigned_pool_size(), sc_dt_integer_subrefs_probe::const_unsigned_range);
}

}  // namespace

/** @brief Emit native references for all bounded and arbitrary range views. */
int sc_main(int, char**)
{
  exercise_fixed<19>();
  exercise_fixed<63>();
  exercise_arbitrary<137>();
  exercise_arbitrary<191>();
  return 0;
}
