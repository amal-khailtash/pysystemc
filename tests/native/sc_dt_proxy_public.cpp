/** @file sc_dt_proxy_public.cpp
 * @brief Independent native semantic reference for sc_dt::sc_proxy.
 * @details Every pointer source remains alive and has at least 191 elements.
 * Borrowed bits/ranges are inspected while their owners remain in scope.
 * Native reports are caught only in the deliberate negative-shift probe.
 */
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <systemc>

#include "sc_dt_proxy_probe.h"

namespace
{
using namespace sc_dt_proxy_probe;

/** @brief Emit one stable native observation for Python parity. */
template <class... Fields>
void emit(const char* family, const char* group, const char* label, const Fields&... fields)
{
  std::cout << "SDP\t" << family << '\t' << group << '\t' << label;
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

/** @brief Copy a full-width vector spelling while the native value is live. */
template <class Vector>
std::string bits(const Vector& value)
{
  return value.to_string();
}

/** @brief Exercise all declared source forms against one proxy owner. */
template <class Target, class Source>
void source_case(const char* family, const char* label, const Target& target, const Source& source)
{
  using Proxy = sc_dt::sc_proxy<Target>;
  using Operand = std::conditional_t<std::is_class_v<Source>, const Source&, Source>;
  using Binary = const sc_dt::sc_lv_base (Proxy::*)(Operand) const;
  using Equal = bool (Proxy::*)(Operand) const;
  using NotEqual = bool (*)(const Proxy&, Operand);
  using Compound = Target& (Proxy::*)(Operand);
  const auto and_member = static_cast<Binary>(&Proxy::operator&);
  const auto or_member = static_cast<Binary>(&Proxy::operator|);
  const auto xor_member = static_cast<Binary>(&Proxy::operator^);
  const auto equal_member = static_cast<Equal>(&Proxy::operator==);
  const auto not_equal_function = static_cast<NotEqual>(&sc_dt::operator!=);
  const sc_dt::sc_lv_base and_value = (static_cast<const Proxy&>(target).*and_member)(source);
  const sc_dt::sc_lv_base or_value = (static_cast<const Proxy&>(target).*or_member)(source);
  const sc_dt::sc_lv_base xor_value = (static_cast<const Proxy&>(target).*xor_member)(source);
  Target and_target(target);
  Target or_target(target);
  Target xor_target(target);
  const auto and_assign = static_cast<Compound>(&Proxy::operator&=);
  const auto or_assign = static_cast<Compound>(&Proxy::operator|=);
  const auto xor_assign = static_cast<Compound>(&Proxy::operator^=);
  Target& and_result = (static_cast<Proxy&>(and_target).*and_assign)(source);
  Target& or_result = (static_cast<Proxy&>(or_target).*or_assign)(source);
  Target& xor_result = (static_cast<Proxy&>(xor_target).*xor_assign)(source);
  const bool equal = (static_cast<const Proxy&>(target).*equal_member)(source);
  const bool not_equal = not_equal_function(static_cast<const Proxy&>(target), source);
  emit(family, "source", label, bits(and_value), bits(or_value), bits(xor_value), int(equal), int(not_equal),
       bits(and_target), int(&and_result == &and_target), bits(or_target), int(&or_result == &or_target),
       bits(xor_target), int(&xor_result == &xor_target));
}

/** @brief Keep every pointer source valid across all synchronous operations. */
struct Sources
{
  PointerOperands pointers;
  sc_dt::sc_unsigned unsigned_value{191};
  sc_dt::sc_signed signed_value{191};
  sc_dt::sc_uint_base uint_value{63};
  sc_dt::sc_int_base int_value{63};

  Sources()
  {
    unsigned_value = "0x123456789abcdef0123456789abcdef";
    signed_value = "-81985529216486895";
    uint_value = static_cast<sc_dt::uint64>(0x123456789abcdef0ULL);
    int_value = static_cast<sc_dt::int64>(-0x123456789abLL);
  }
};

/** @brief Run 13-source binary, equality and compound coverage. */
template <class Target>
void source_matrix(const char* family, const Target& target, const Sources& source)
{
  source_case(family, "char_pointer", target, source.pointers.char_pointer());
  source_case(family, "bool_pointer", target, source.pointers.bool_pointer());
  source_case(family, "logic_pointer", target, source.pointers.logic_pointer());
  source_case(family, "unsigned", target, source.unsigned_value);
  source_case(family, "signed", target, source.signed_value);
  source_case(family, "uint_base", target, source.uint_value);
  source_case(family, "int_base", target, source.int_value);
  source_case(family, "unsigned_long", target, static_cast<unsigned long>(0x12345678UL));
  source_case(family, "long", target, static_cast<long>(-0x13579L));
  source_case(family, "unsigned_int", target, static_cast<unsigned int>(0xA53CU));
  source_case(family, "int", target, static_cast<int>(-0x5A35));
  source_case(family, "uint64", target, static_cast<sc_dt::uint64>(0xfedcba9876543210ULL));
  source_case(family, "int64", target, static_cast<sc_dt::int64>(-0x123456789ABCDELL));
}

/** @brief Observe inherited shifts, rotations, reductions, conversions and IO. */
template <class Target>
void member_matrix(const char* family, Target target)
{
  Target formatted(191);
  formatted = "101001001101011001011001110101011";
  std::ostringstream printed;
  target.print(printed);
  emit(family, "observe", "initial", target.length(), int(target.and_reduce()), int(target.nand_reduce()),
       int(target.or_reduce()), int(target.nor_reduce()), int(target.xor_reduce()), int(target.xnor_reduce()),
       target.to_int(), target.to_uint(), target.to_long(), target.to_ulong(), target.to_int64(), target.to_uint64(),
       target.to_string(), formatted.to_string(sc_dt::SC_BIN), formatted.to_string(sc_dt::SC_BIN, true), printed.str());

  if (family == std::string("lv"))
  {
    const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ID_INVALID_FX_VALUE_, sc_core::SC_THROW);
    bool caught = false;
    std::string type;
    std::string message;
    try
    {
      (void)target.to_string(sc_dt::SC_BIN);
    }
    catch (const sc_core::sc_report& report)
    {
      caught = true;
      type = report.get_msg_type();
      message = report.get_msg();
    }
    sc_core::sc_report_handler::set_actions(sc_core::SC_ID_INVALID_FX_VALUE_, previous);
    emit(family, "format_report", "binary_with_xz", int(caught), type, message);
  }

  Target left(target);
  Target right(target);
  Target reversed(target);
  Target inverted(target);
  Target& left_result = (left <<= 67);
  Target& right_result = (right >>= 71);
  Target& reverse_result = reversed.reverse();
  const sc_dt::sc_lv_base complement = ~target;
  inverted = "0";
  emit(family, "transform", "shift_reverse", bits(left), int(&left_result == &left), bits(right),
       int(&right_result == &right), bits(reversed), int(&reverse_result == &reversed), bits(complement));

  Target lrot(target);
  Target rrot(target);
  Target& lrot_result = lrot.lrotate(43);
  Target& rrot_result = rrot.rrotate(53);
  emit(family, "transform", "rotate", bits(lrot), int(&lrot_result == &lrot), bits(rrot), int(&rrot_result == &rrot));

  auto bit = target.bit(23);
  auto index = target[161];
  auto range = target.range(151, 73);
  const Target& const_target = target;
  auto read_bit = const_target.bit(23);
  auto read_index = const_target[161];
  auto read_range = const_target.range(151, 73);
  auto copied_range = std::make_unique<decltype(range)>(range);
  emit(family, "view", "selection", bits(sc_dt::sc_lv_base(bit)), bits(sc_dt::sc_lv_base(index)),
       bits(sc_dt::sc_lv_base(range)), bits(sc_dt::sc_lv_base(read_bit)), bits(sc_dt::sc_lv_base(read_index)),
       bits(sc_dt::sc_lv_base(read_range)), range.length(), read_range.length());
  range = "0x123456789abcdef";
  emit(family, "view", "write_through", bits(target));
  emit(family, "view_copy", "after_shell_write", bits(sc_dt::sc_lv_base(*copied_range)), copied_range->length());

  std::istringstream scan_input("101101001011");
  Target scanned(191);
  scanned.scan(scan_input);
  emit(family, "io", "scan", bits(scanned), int(scan_input.good()));
}

/** @brief Exercise the generic CRTP owner through its public base interface. */
void custom_proxy_case()
{
  CustomProxy owner("1010010010110101");
  CustomProxy source("1100101010011010");
  CustomProxy& assigned = owner.assign_(source);
  const sc_dt::sc_lv_base expression = owner ^ source;
  emit("custom", "crtp", "generic", owner.length(), bits(owner.storage()), bits(expression), int(&assigned == &owner),
       int(owner.xor_reduce()), owner.to_int(), owner.to_string());
  emit("custom", "lifetime", "virtual_delete", virtual_destruction_count());
}

/** @brief Capture native negative-shift reports without hiding other reports. */
void report_case()
{
  const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ID_OUT_OF_BOUNDS_, sc_core::SC_THROW);
  sc_dt::sc_bv_base value("101101");
  bool caught = false;
  std::string type;
  std::string message;
  try
  {
    value <<= -3;
  }
  catch (const sc_core::sc_report& report)
  {
    caught = true;
    type = report.get_msg_type();
    message = report.get_msg();
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ID_OUT_OF_BOUNDS_, previous);
  emit("bv", "report", "negative_shift", int(caught), type, message, bits(value));
}
}  // namespace

/** @brief Run this standalone datatype probe without starting kernel work. */
int sc_main(int, char**)
{
  Sources source;
  sc_dt::sc_bv_base bits_owner(191);
  bits_owner = "101001001101011001011001110101011";
  sc_dt::sc_lv_base logic_owner(191);
  logic_owner = "10XZ1001101011001011001110101011";
  CustomProxy generic_owner(191);
  generic_owner = "101001001101011001011001110101011";
  source_matrix("bv", bits_owner, source);
  member_matrix("bv", bits_owner);
  source_matrix("lv", logic_owner, source);
  member_matrix("lv", logic_owner);
  custom_proxy_case();
  source_matrix("custom", generic_owner, source);
  report_case();
  return 0;
}
