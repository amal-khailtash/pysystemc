/** @file Native concatref lifetime, conversion and four-state value probes.
 * @details Pool shells borrow their operands. value()/implicit refs additionally
 * borrow reusable native unsigned shells and temporary heap storage. Copies
 * returned by helpers own numeric storage. Hold the GIL; no waits, callbacks or
 * Python references are retained. Native reports and allocations propagate.
 */
#pragma once
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>
namespace concatref_probe
{
/** @brief Own a private-constructor concat pool and explicitly release its array.
 * @details Native sc_vpool intentionally does not delete its array. This test
 * owner frees only its own array, exercising public concat destructors without
 * deleting borrowed operands or changing the global pool.
 */
struct OwnedPool : sc_core::sc_vpool<sc_dt::sc_concatref>
{
  /** @brief Allocate four owned native concat shells. */
  OwnedPool() : sc_core::sc_vpool<sc_dt::sc_concatref>(2)
  {
  }
  /** @brief Prohibit copying the owned pool array. */
  OwnedPool(const OwnedPool&) = delete;
  /** @brief Prohibit shallow assignment of the owned pool array. */
  OwnedPool& operator=(const OwnedPool&) = delete;
  /** @brief Destroy only owned shells, retaining every borrowed operand. */
  ~OwnedPool()
  {
    delete[] m_pool_p;
  }
};
/** @brief Return a borrowed shell initialized with mutable live operands. */
inline sc_dt::sc_concatref& select(sc_dt::sc_value_base& left, sc_dt::sc_value_base& right)
{
  auto* node = sc_dt::sc_concatref::m_pool.allocate();
  node->initialize(left, right);
  return *node;
}
/** @brief Return a borrowed shell using the exact const initializer. */
inline sc_dt::sc_concatref& const_select(const sc_dt::sc_value_base& left, const sc_dt::sc_value_base& right)
{
  auto* node = sc_dt::sc_concatref::m_pool.allocate();
  node->initialize(left, right);
  return *node;
}
/** @brief Return the native unsigned implicit conversion's borrowed reference. */
inline const sc_dt::sc_unsigned& implicit_reference(const sc_dt::sc_concatref& node)
{
  return node;
}
/** @brief Return an independent copy of a live concat's numeric value. */
inline sc_dt::sc_unsigned snapshot(const sc_dt::sc_concatref& node)
{
  return node.value();
}
/** @brief Preserve the exact implicit uint64 conversion. */
inline sc_dt::uint64 implicit_integer(const sc_dt::sc_concatref& node)
{
  return node;
}
/** @brief Copy native printed text. */
inline std::string printed(const sc_dt::sc_concatref& node)
{
  std::ostringstream out;
  node.print(out);
  return out.str();
}
/** @brief Inspect public flags and private constructor restrictions. */
inline bool traits()
{
  return !std::is_default_constructible<sc_dt::sc_concatref>::value &&
         !std::is_copy_constructible<sc_dt::sc_concatref>::value &&
         std::has_virtual_destructor<sc_dt::sc_concatref>::value && sc_dt::sc_concatref::cf_none == 0 &&
         sc_dt::sc_concatref::cf_xz_present == 1;
}
/** @brief Read the exact native concat_length bool-pointer result. */
inline bool has_xz(const sc_dt::sc_concatref& node)
{
  bool result = false;
  node.concat_length(&result);
  return result;
}
/** @brief Return native unsigned value-shell capacity without resetting it. */
inline std::size_t value_capacity()
{
  return sc_dt::sc_unsigned::m_pool.size();
}
/** @brief Use the native legacy width equivalent when deprecated members are disabled. */
inline int native_bitwidth(const sc_dt::sc_concatref& node)
{
#ifdef SC_DT_DEPRECATED
  return node.bitwidth();
#else
  return node.length();
#endif
}
/** @brief Return enough native digits to pack a live node at the given offset. */
inline int packing_digits(const sc_dt::sc_concatref& node, int low)
{
  return std::max(3, int((node.length() + low + BITS_PER_DIGIT - 1) / BITS_PER_DIGIT));
}
/** @brief Return native concat pool capacity without resetting it. */
inline std::size_t capacity()
{
  return sc_dt::sc_concatref::m_pool.size();
}
/** @brief Own a four-state value-base operand using native logic encodings.
 * @details This test operand borrows nothing. Virtual concat hooks pack digits
 * with native BITS_PER_DIGIT, exposing X/Z controls to concatref initialization.
 * Parent destruction is counted to verify concat destruction never owns leaves.
 */
struct LogicLeaf : sc_dt::sc_value_base
{
  std::string bits;
  static int destroyed;
  /** @brief Copy a four-state spelling into independent operand storage. */
  explicit LogicLeaf(const std::string& value) : bits(value)
  {
  }
  /** @brief Count destruction without touching any borrowing concat shell. */
  ~LogicLeaf() override
  {
    ++destroyed;
  }
  /** @brief Read one native logic encoding at a valid little-endian bit index. */
  int state(int bit) const
  {
    return sc_dt::sc_logic(bits[bits.size() - 1 - bit]).value();
  }
  /** @brief Return operand width and write optional live X/Z presence. */
  int concat_length(bool* xz = nullptr) const override
  {
    if (xz) *xz = bits.find_first_of("XZ") != std::string::npos;
    return bits.size();
  }
  /** @brief Borrow a sufficiently sized digit array and pack one bit plane. */
  bool packed(sc_dt::sc_digit* output, int low, bool control) const
  {
    bool present = false;
    for (int i = 0; i < int(bits.size()); ++i)
    {
      int bit = control ? (state(i) >> 1) : (state(i) & 1);
      auto mask = sc_dt::sc_digit(1) << ((low + i) % BITS_PER_DIGIT);
      auto& word = output[(low + i) / BITS_PER_DIGIT];
      word = (word & ~mask) | (bit ? mask : 0);
      present = present || bit;
    }
    return present;
  }
  /** @brief Pack X/Z control bits while preserving outside destination bits. */
  bool concat_get_ctrl(sc_dt::sc_digit* output, int low) const override
  {
    return packed(output, low, true);
  }
  /** @brief Pack data bits while preserving outside destination bits. */
  bool concat_get_data(sc_dt::sc_digit* output, int low) const override
  {
    return packed(output, low, false);
  }
  /** @brief Return the low 64 native data bits without X/Z control bits. */
  sc_dt::uint64 concat_get_uint64() const override
  {
    sc_dt::uint64 value = 0;
    for (int i = 0; i < std::min<int>(bits.size(), 64); ++i) value |= sc_dt::uint64(state(i) & 1) << i;
    return value;
  }
  /** @brief Change only the data bit plane, preserving X/Z controls. */
  void concat_clear_data(bool ones) override
  {
    for (char& bit : bits)
      bit = sc_dt::sc_logic(sc_dt::sc_logic_value_t((sc_dt::sc_logic(bit).value() & 2) | (ones ? 1 : 0))).to_char();
  }
  /** @brief Copy unsigned source bits with zero extension into owned storage. */
  void concat_set(const sc_dt::sc_unsigned& source, int low) override
  {
    for (int i = 0; i < int(bits.size()); ++i)
      bits[bits.size() - 1 - i] = (low + i < source.length() && source[low + i].to_bool()) ? '1' : '0';
  }
  /** @brief Copy signed source bits with sign extension into owned storage. */
  void concat_set(const sc_dt::sc_signed& source, int low) override
  {
    for (int i = 0; i < int(bits.size()); ++i)
      bits[bits.size() - 1 - i] =
        (low + i < source.length() ? source[low + i].to_bool() : source.sign() < 0) ? '1' : '0';
  }
  /** @brief Copy machine unsigned bits with zero extension into owned storage. */
  void concat_set(sc_dt::uint64 source, int low) override
  {
    for (int i = 0; i < int(bits.size()); ++i)
      bits[bits.size() - 1 - i] = (low + i < 64 && (source >> (low + i)) & 1) ? '1' : '0';
  }
  /** @brief Copy machine signed bits with sign extension into owned storage. */
  void concat_set(sc_dt::int64 source, int low) override
  {
    for (int i = 0; i < int(bits.size()); ++i)
      bits[bits.size() - 1 - i] = (low + i < 64 ? ((sc_dt::uint64(source) >> (low + i)) & 1) : source < 0) ? '1' : '0';
  }
};
inline int LogicLeaf::destroyed = 0;
}  // namespace concatref_probe
