/** @file sc_dt_vectors_public.cpp
 * @brief Independent native reference for arbitrary-width vector owners.
 * @details Every constructed BV/LV owns its storage; copies remain valid when
 *          source owners are destroyed. Pointer fixtures have static lifetime,
 *          and no Python references or callbacks are retained. Each test runs
 *          in a fresh process, holds the GIL, and does not schedule processes.
 *          SystemC reports are caught only by the explicit error probe.
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_vectors_probe.h"

namespace
{

using namespace sc_dt_vectors_probe;

/** @brief Emit one exact native-comparable vector record. */
template <typename... Values>
void emit(const char* family, const char* label, const Values&... values)
{
  std::cout << "SDV\t" << family << '\t' << label;
  using swallow = int[];
  (void)swallow{0, ((std::cout << '\t' << values), 0)...};
  std::cout << '\n';
}

/** @brief Emit a constructor result while its owning value is live. */
template <typename Vector>
void emit_construct(const char* type_name, const char* overload, const Vector& value)
{
  emit("ctor", type_name, overload, render(value));
}

/** @brief Construct every public sc_bv<W> overload with a width-37 type. */
void emit_bv_constructors()
{
  using Vector = sc_dt::sc_bv<37>;
  emit_construct("sc_bv37", "default", Vector());
  emit_construct("sc_bv37", "bool", Vector(true));
  emit_construct("sc_bv37", "char", Vector('1'));
  emit_construct("sc_bv37", "char_pointer", Vector("101101"));
  emit_construct("sc_bv37", "bool_pointer", Vector(bool_bits()));
  emit_construct("sc_bv37", "logic_pointer", Vector(logic_bits()));
  emit_construct("sc_bv37", "sc_unsigned", Vector(unsigned_value()));
  emit_construct("sc_bv37", "sc_signed", Vector(signed_value()));
  emit_construct("sc_bv37", "sc_uint_base", Vector(uint_value()));
  emit_construct("sc_bv37", "sc_int_base", Vector(int_value()));
  emit_construct("sc_bv37", "unsigned_long", Vector(static_cast<unsigned long>(0x12345678UL)));
  emit_construct("sc_bv37", "long", Vector(static_cast<long>(-0x13579)));
  emit_construct("sc_bv37", "unsigned_int", Vector(static_cast<unsigned int>(0xA53C)));
  emit_construct("sc_bv37", "int", Vector(0x5A35));
  emit_construct("sc_bv37", "uint64", Vector(static_cast<sc_dt::uint64>(0x123456789ABCDEF0ULL)));
  emit_construct("sc_bv37", "int64", Vector(static_cast<sc_dt::int64>(-0x123456789LL)));
  emit_construct("sc_bv37", "proxy_bv", Vector(bv_proxy()));
  emit_construct("sc_bv37", "proxy_lv", Vector(lv_proxy()));
  const Vector copy_source("101001011010010111001");
  emit_construct("sc_bv37", "copy", Vector(copy_source));
}

/** @brief Construct every public sc_lv<W> overload with a width-38 type. */
void emit_lv_constructors()
{
  using Vector = sc_dt::sc_lv<38>;
  emit_construct("sc_lv38", "default", Vector());
  emit_construct("sc_lv38", "logic", Vector(sc_dt::sc_logic('Z')));
  emit_construct("sc_lv38", "bool", Vector(true));
  emit_construct("sc_lv38", "char", Vector('X'));
  emit_construct("sc_lv38", "char_pointer", Vector("01ZX10"));
  emit_construct("sc_lv38", "bool_pointer", Vector(bool_bits()));
  emit_construct("sc_lv38", "logic_pointer", Vector(logic_bits()));
  emit_construct("sc_lv38", "sc_unsigned", Vector(unsigned_value()));
  emit_construct("sc_lv38", "sc_signed", Vector(signed_value()));
  emit_construct("sc_lv38", "sc_uint_base", Vector(uint_value()));
  emit_construct("sc_lv38", "sc_int_base", Vector(int_value()));
  emit_construct("sc_lv38", "unsigned_long", Vector(static_cast<unsigned long>(0x12345678UL)));
  emit_construct("sc_lv38", "long", Vector(static_cast<long>(-0x13579)));
  emit_construct("sc_lv38", "unsigned_int", Vector(static_cast<unsigned int>(0xA53C)));
  emit_construct("sc_lv38", "int", Vector(0x5A35));
  emit_construct("sc_lv38", "uint64", Vector(static_cast<sc_dt::uint64>(0x123456789ABCDEF0ULL)));
  emit_construct("sc_lv38", "int64", Vector(static_cast<sc_dt::int64>(-0x123456789LL)));
  emit_construct("sc_lv38", "proxy_bv", Vector(bv_proxy()));
  emit_construct("sc_lv38", "proxy_lv", Vector(lv_proxy()));
  const Vector copy_source("01ZX001101");
  emit_construct("sc_lv38", "copy", Vector(copy_source));
}

/** @brief Construct every inventoried sc_bv_base constructor overload. */
void emit_bv_base_constructors()
{
  using Vector = sc_dt::sc_bv_base;
  emit_construct("sc_bv_base55", "int_length", Vector(55));
  emit_construct("sc_bv_base55", "bool_length", Vector(true, 55));
  emit_construct("sc_bv_base55", "char_pointer", Vector("101101"));
  emit_construct("sc_bv_base55", "char_pointer_length", Vector("101101", 55));
  emit_construct("sc_bv_base55", "copy", Vector(Vector("10100101", 55)));
  emit_construct("sc_bv_base55", "proxy_bv",
                 Vector(static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(bv_proxy())));
  emit_construct("sc_bv_base55", "proxy_lv",
                 Vector(static_cast<const sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(lv_proxy())));
  const auto proxy = bv_proxy().range(54, 0);
  emit_construct("sc_bv_base55", "proxy_template", Vector(proxy));
}

/** @brief Construct every inventoried sc_lv_base constructor overload. */
void emit_lv_base_constructors()
{
  using Vector = sc_dt::sc_lv_base;
  emit_construct("sc_lv_base54", "int_length", Vector(54));
  emit_construct("sc_lv_base54", "logic_length", Vector(sc_dt::SC_LOGIC_Z, 54));
  emit_construct("sc_lv_base54", "char_pointer", Vector("01ZX10"));
  emit_construct("sc_lv_base54", "char_pointer_length", Vector("01ZX10", 54));
  emit_construct("sc_lv_base54", "copy", Vector(Vector("01ZX01", 54)));
  emit_construct("sc_lv_base54", "proxy_bv",
                 Vector(static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(bv_proxy())));
  emit_construct("sc_lv_base54", "proxy_lv",
                 Vector(static_cast<const sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(lv_proxy())));
  const auto proxy = lv_proxy().range(53, 0);
  emit_construct("sc_lv_base54", "proxy_template", Vector(proxy));
}

/** @brief Reproduce disabled legacy numeric constructor semantics for parity.
 * @tparam Vector sc_bv_base or sc_lv_base.
 * @tparam Number A sc_signed, sc_unsigned, sc_int_base, or sc_uint_base.
 * @param type_name Base owner record name.
 * @param label Inventory constructor overload name.
 * @param source Native number borrowed for this call.
 * @details SC_DT_DEPRECATED is disabled in the packaged library. The old
 *          constructor allocated source.length() bits then assigned source;
 *          this helper reproduces that behavior without modifying SystemC.
 */
template <typename Vector, typename Number>
void emit_legacy_base_constructor(const char* type_name, const char* label, const Number& source)
{
  Vector value(source.length());
  value = source;
  emit_construct(type_name, label, value);
}

/** @brief Assign each typed fixture using its native overload and emit the owner. */
template <typename Owner, typename Value>
void emit_assignment(const char* type_name, const char* overload, Owner& owner, const Value& value)
{
  owner = std::string(static_cast<std::size_t>(owner.length()), '0').c_str();
  owner = value;
  emit("assign", type_name, overload, render(owner));
}

/** @brief Exercise all same-category fixed-width vector assignment overloads. */
template <typename Owner, typename Source>
void assign_fixed(const char* type_name, Owner& owner, const Source& source)
{
  const char* bits = "10100101101001011100110000111100";
  emit_assignment(type_name, "proxy_template", owner, bv_proxy());
  emit_assignment(type_name, "copy", owner, source);
  emit_assignment(type_name, "char_pointer", owner, bits);
  emit_assignment(type_name, "bool_pointer", owner, bool_bits());
  emit_assignment(type_name, "logic_pointer", owner, logic_bits());
  emit_assignment(type_name, "sc_unsigned", owner, unsigned_value());
  emit_assignment(type_name, "sc_signed", owner, signed_value());
  emit_assignment(type_name, "sc_uint_base", owner, uint_value());
  emit_assignment(type_name, "sc_int_base", owner, int_value());
  emit_assignment(type_name, "unsigned_long", owner, static_cast<unsigned long>(0x12345678UL));
  emit_assignment(type_name, "long", owner, static_cast<long>(-0x13579));
  emit_assignment(type_name, "unsigned_int", owner, static_cast<unsigned int>(0xA53C));
  emit_assignment(type_name, "int", owner, 0x5A35);
  emit_assignment(type_name, "uint64", owner, static_cast<sc_dt::uint64>(0x123456789ABCDEF0ULL));
  emit_assignment(type_name, "int64", owner, static_cast<sc_dt::int64>(-0x123456789LL));
}

/** @brief Exercise all base assignments, including BV/LV and generic proxies. */
template <typename Owner>
void assign_base(const char* type_name, Owner& owner, Owner& same_source)
{
  const char* bits = "10100101101001011100110000111100";
  emit_assignment(type_name, "proxy_bv", owner, static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(bv_proxy()));
  emit_assignment(type_name, "proxy_lv", owner, static_cast<const sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(lv_proxy()));
  const auto generic_proxy = bv_proxy().range(53, 0);
  emit_assignment(type_name, "proxy_template", owner, generic_proxy);
  emit_assignment(type_name, "copy", owner, same_source);
  emit_assignment(type_name, "char_pointer", owner, bits);
  emit_assignment(type_name, "bool_pointer", owner, bool_bits());
  emit_assignment(type_name, "logic_pointer", owner, logic_bits());
  emit_assignment(type_name, "sc_unsigned", owner, unsigned_value());
  emit_assignment(type_name, "sc_signed", owner, signed_value());
  emit_assignment(type_name, "sc_uint_base", owner, uint_value());
  emit_assignment(type_name, "sc_int_base", owner, int_value());
  emit_assignment(type_name, "unsigned_long", owner, static_cast<unsigned long>(0x12345678UL));
  emit_assignment(type_name, "long", owner, static_cast<long>(-0x13579));
  emit_assignment(type_name, "unsigned_int", owner, static_cast<unsigned int>(0xA53C));
  emit_assignment(type_name, "int", owner, 0x5A35);
  emit_assignment(type_name, "uint64", owner, static_cast<sc_dt::uint64>(0x123456789ABCDEF0ULL));
  emit_assignment(type_name, "int64", owner, static_cast<sc_dt::int64>(-0x123456789LL));
}

/** @brief Exercise base accessors, protected allocation state, and mutations. */
template <typename Vector>
void emit_storage(const char* type_name, Vector& value)
{
  emit("storage", type_name, value.length(), value.size(), int(value.is_01()), value.get_bit(0), value.get_word(0),
       value.get_cword(0));
  value.set_bit(0, static_cast<typename Vector::value_type>(1));
  value.set_word(0, 0xA5U);
  value.set_cword(0, 0x5AU);
  value.clean_tail();
  emit("storage_after", type_name, render(value), value.get_word(0), value.get_cword(0));
}

/** @brief Return an owning copy after its temporary source has been destroyed. */
sc_dt::sc_bv_base make_bv_copy_after_source()
{
  sc_dt::sc_bv_base copy(55);
  {
    sc_dt::sc_bv_base source("1010010110100101", 55);
    copy = source;
    const auto copied_bit = copy.get_bit(0);
    source.set_bit(0, !copied_bit);
    if (copy.get_bit(0) != copied_bit)
    {
      throw std::runtime_error("BV copy shares source storage");
    }
  }
  return copy;
}

/** @brief Return an owning LV copy after its temporary source has been destroyed. */
sc_dt::sc_lv_base make_lv_copy_after_source()
{
  sc_dt::sc_lv_base copy(54);
  {
    sc_dt::sc_lv_base source("01ZX1010XZ", 54);
    copy = source;
    const auto copied_bit = copy.get_bit(0);
    source.set_bit(0, sc_dt::Log_0);
    if (copy.get_bit(0) != copied_bit)
    {
      throw std::runtime_error("LV copy shares source storage");
    }
  }
  return copy;
}

}  // namespace

int sc_main(int, char**)
{
  emit_bv_constructors();
  emit_lv_constructors();
  emit_bv_base_constructors();
  emit_lv_base_constructors();
  emit_legacy_base_constructor<sc_dt::sc_bv_base>("sc_bv_base55", "sc_signed", signed_value());
  emit_legacy_base_constructor<sc_dt::sc_bv_base>("sc_bv_base55", "sc_unsigned", unsigned_value());
  emit_legacy_base_constructor<sc_dt::sc_bv_base>("sc_bv_base55", "sc_uint_base", uint_value());
  emit_legacy_base_constructor<sc_dt::sc_bv_base>("sc_bv_base55", "sc_int_base", int_value());
  emit_legacy_base_constructor<sc_dt::sc_lv_base>("sc_lv_base54", "sc_signed", signed_value());
  emit_legacy_base_constructor<sc_dt::sc_lv_base>("sc_lv_base54", "sc_unsigned", unsigned_value());
  emit_legacy_base_constructor<sc_dt::sc_lv_base>("sc_lv_base54", "sc_uint_base", uint_value());
  emit_legacy_base_constructor<sc_dt::sc_lv_base>("sc_lv_base54", "sc_int_base", int_value());

  sc_dt::sc_bv<37> bv37("0");
  const sc_dt::sc_bv<37> bv37_source("10100101101001011100110000111100");
  assign_fixed("sc_bv37", bv37, bv37_source);
  sc_dt::sc_lv<38> lv38("0");
  const sc_dt::sc_lv<38> lv38_source("01ZX101001011100110000111100");
  assign_fixed("sc_lv38", lv38, lv38_source);

  sc_dt::sc_bv_base bv55(55);
  sc_dt::sc_bv_base bv55_source("10100101101001011100110000111100", 55);
  assign_base("sc_bv_base55", bv55, bv55_source);
  sc_dt::sc_lv_base lv54(54);
  sc_dt::sc_lv_base lv54_source("01ZX101001011100110000111100", 54);
  assign_base("sc_lv_base54", lv54, lv54_source);

  sc_dt::sc_bv_base bv_storage("01010101010101010101010101010101010101010101010101010101");
  sc_dt::sc_lv_base lv_storage("01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01");
  emit_storage("sc_bv_base", bv_storage);
  emit_storage("sc_lv_base", lv_storage);
  emit("protected", "sc_bv_base", bv_storage_state(bv_storage));
  emit("protected", "sc_lv_base", lv_storage_state(lv_storage));

  sc_dt::sc_bv<137> bv137("01");
  sc_dt::sc_lv<191> lv191("01ZX");
  sc_dt::sc_bv<257> bv257("10");
  sc_dt::sc_lv<257> lv257("ZX01");
  emit("wide", "bv137", bv137.length(), render(bv137));
  emit("wide", "lv191", lv191.length(), render(lv191));
  emit("wide", "bv257", bv257.length(), render(bv257));
  emit("wide", "lv257", lv257.length(), render(lv257));

  const auto wide_lv_proxy = lv_proxy().range(190, 0);
  const sc_dt::sc_lv<191> wide_lv_proxy_constructed(wide_lv_proxy);
  sc_dt::sc_lv<191> wide_lv_proxy_assigned(191);
  wide_lv_proxy_assigned = wide_lv_proxy;
  emit("wide_proxy", "lv191", render(wide_lv_proxy_constructed), render(wide_lv_proxy_assigned));

  const auto bv_copy = make_bv_copy_after_source();
  const auto lv_copy = make_lv_copy_after_source();
  emit("lifetime", "bv_copy", bv_copy.length(), render(bv_copy));
  emit("lifetime", "lv_copy", lv_copy.length(), render(lv_copy));
  emit("traits", "owners_destructible",
       int(std::is_destructible<sc_dt::sc_bv_base>::value && std::is_destructible<sc_dt::sc_lv_base>::value &&
           std::is_destructible<sc_dt::sc_bv<37>>::value && std::is_destructible<sc_dt::sc_lv<38>>::value));

  bool rejected = false;
  try
  {
    sc_dt::sc_bv_base invalid(-1);
  }
  catch (const sc_core::sc_report&)
  {
    rejected = true;
  }
  emit("error", "negative_base_length", int(rejected));
  return 0;
}
