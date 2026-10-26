/** @file sc_dt_proxy_probe.h
 * @brief Exact overload and custom CRTP probes for sc_dt::sc_proxy.
 * @details All vector views borrow their owners for the duration of the call.
 * Pointer operands must point to at least the left vector's length. The
 * helpers retain no pointers, Python objects, callbacks or kernel state. Calls
 * through cppyy are synchronous and require the GIL; native exceptions pass to
 * the caller.
 */
#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_dt_proxy_probe
{
/** @brief Traits for the custom logic proxy used to instantiate generic APIs. */
struct CustomProxy;

}  // namespace sc_dt_proxy_probe

namespace sc_dt
{
template <>
struct sc_proxy_traits<sc_dt_proxy_probe::CustomProxy>
{
  using traits_type = sc_proxy_traits<sc_dt_proxy_probe::CustomProxy>;
  using value_type = sc_logic_value_t;
  using bit_type = sc_logic;
  using vector_type = sc_dt_proxy_probe::CustomProxy;
};
}  // namespace sc_dt

namespace sc_dt_proxy_probe
{
/** @brief A minimal CRTP owner backed by a native arbitrary-width logic value.
 * @details Its base proxy uses the ordinary public bit/word interface. The
 * internal vector owns storage; the proxy itself borrows the owner, and no
 * Python references or callbacks are involved.
 */
class CustomProxy : public sc_dt::sc_proxy<CustomProxy>
{
 public:
  using base_type = sc_dt::sc_proxy<CustomProxy>;
  using value_type = base_type::value_type;

  /** @brief Allocate an owner-managed logic vector of the requested width. */
  explicit CustomProxy(int length) : value_(length)
  {
  }
  /** @brief Allocate a logic vector initialized from native bit-string text. */
  explicit CustomProxy(const char* value) : value_(value)
  {
  }
  /** @brief Copy the independent owner-managed vector storage. */
  CustomProxy(const CustomProxy&) = default;
  /** @brief Replace this owner's storage from another independent owner. */
  CustomProxy& operator=(const CustomProxy&) = default;
  /** @brief Count destruction through the virtual sc_proxy base. */
  ~CustomProxy() override
  {
    ++destructions;
  }

  /** @brief Return storage width without copying or retaining external state. */
  int length() const
  {
    return value_.length();
  }
  /** @brief Return the number of words used by the inherited algorithms. */
  int size() const
  {
    return value_.size();
  }
  /** @brief Read one bit using the native vector's index convention. */
  value_type get_bit(int index) const
  {
    return value_.get_bit(index);
  }
  /** @brief Write one bit in owner-managed vector storage. */
  void set_bit(int index, value_type bit)
  {
    value_.set_bit(index, bit);
  }
  /** @brief Read one data word for inherited proxy operations. */
  sc_dt::sc_digit get_word(int index) const
  {
    return value_.get_word(index);
  }
  /** @brief Read one four-state control word for inherited operations. */
  sc_dt::sc_digit get_cword(int index) const
  {
    return value_.get_cword(index);
  }
  /** @brief Write one data word in the owned native vector. */
  void set_word(int index, sc_dt::sc_digit word)
  {
    value_.set_word(index, word);
  }
  /** @brief Write one control word in the owned native vector. */
  void set_cword(int index, sc_dt::sc_digit word)
  {
    value_.set_cword(index, word);
  }
  /** @brief Normalize unused bits after inherited proxy mutations. */
  void clean_tail()
  {
    value_.clean_tail();
  }

  /** @brief Assign text synchronously through the inherited native proxy.
   * @details SystemC conversion/report exceptions propagate; the argument is
   * consumed immediately and is not retained.
   */
  CustomProxy& operator=(const char* value)
  {
    base_type::assign_(value);
    return *this;
  }

  /** @brief Borrow the owner-managed logic value for result inspection. */
  const sc_dt::sc_lv_base& storage() const
  {
    return value_;
  }

  inline static int destructions = 0;

 private:
  sc_dt::sc_lv_base value_;
};

/** @brief Stable wide pointer operands shared by native and Python fixtures.
 * @details Returned pointers refer to static storage and remain valid for the
 * process lifetime. Every pointee has 191 entries; native callers must pass
 * the pointer only to a vector operation whose left length is at most 191.
 */
class PointerOperands
{
 public:
  /** @brief Borrow 191 nul-terminated binary characters. */
  const char* char_pointer() const
  {
    return chars().data();
  }
  /** @brief Borrow 191 stable Boolean elements. */
  const bool* bool_pointer() const
  {
    return bools().data();
  }
  /** @brief Borrow 191 stable four-state logic elements. */
  const sc_dt::sc_logic* logic_pointer() const
  {
    return logics().data();
  }

 private:
  static const std::array<char, 192>& chars()
  {
    static const std::array<char, 192> value = []
    {
      std::array<char, 192> result{};
      for (int i = 0; i < 191; ++i) result[static_cast<std::size_t>(i)] = ((i * 7 + 3) % 11 < 5) ? '1' : '0';
      return result;
    }();
    return value;
  }

  static const std::array<bool, 191>& bools()
  {
    static const std::array<bool, 191> value = []
    {
      std::array<bool, 191> result{};
      for (int i = 0; i < 191; ++i) result[static_cast<std::size_t>(i)] = (i * 5 + 1) % 7 < 3;
      return result;
    }();
    return value;
  }

  static const std::array<sc_dt::sc_logic, 191>& logics()
  {
    static const std::array<sc_dt::sc_logic, 191> value = []
    {
      std::array<sc_dt::sc_logic, 191> result{};
      const char logic_bits[] = {'0', '1', 'X', 'Z'};
      for (int i = 0; i < 191; ++i) result[static_cast<std::size_t>(i)] = sc_dt::sc_logic(logic_bits[(i * 3 + 1) % 4]);
      return result;
    }();
    return value;
  }
};

using BvProxy = sc_dt::sc_proxy<sc_dt::sc_bv_base>;
using LvProxy = sc_dt::sc_proxy<sc_dt::sc_lv_base>;
using CustomBase = sc_dt::sc_proxy<CustomProxy>;

/** @brief Assert a proxy member overload without relying on deduction. */
#define SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, Result, Source)                                       \
  static_assert(std::is_same_v<decltype(static_cast<Result (Proxy::*)(Source) const>(&Proxy::Name)), \
                               Result (Proxy::*)(Source) const>,                                     \
                "sc_proxy const member signature changed: " #Name)
#define SC_DT_PROXY_ASSERT_COMPOUND(Proxy, Name, Owner, Source)                                                \
  static_assert(                                                                                               \
    std::is_same_v<decltype(static_cast<Owner& (Proxy::*)(Source)>(&Proxy::Name)), Owner& (Proxy::*)(Source)>, \
    "sc_proxy compound signature changed: " #Name)

#define SC_DT_PROXY_ASSERT_13(Proxy, Owner)                                                         \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const char*);                \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const bool*);                \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const sc_dt::sc_logic*);     \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const sc_dt::sc_unsigned&);  \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const sc_dt::sc_signed&);    \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const sc_dt::sc_uint_base&); \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, const sc_dt::sc_int_base&);  \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, unsigned long);              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, long);                       \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, unsigned int);               \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, int);                        \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, sc_dt::uint64);              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator&, const sc_dt::sc_lv_base, sc_dt::int64);               \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const char*);                               \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const bool*);                               \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const sc_dt::sc_logic*);                    \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const sc_dt::sc_unsigned&);                 \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const sc_dt::sc_signed&);                   \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const sc_dt::sc_uint_base&);                \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, const sc_dt::sc_int_base&);                 \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, unsigned long);                             \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, long);                                      \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, unsigned int);                              \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, int);                                       \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, sc_dt::uint64);                             \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator&=, Owner, sc_dt::int64)

SC_DT_PROXY_ASSERT_13(BvProxy, sc_dt::sc_bv_base);
SC_DT_PROXY_ASSERT_13(LvProxy, sc_dt::sc_lv_base);
SC_DT_PROXY_ASSERT_13(CustomBase, CustomProxy);
#define SC_DT_PROXY_ASSERT_BINARY_13(Proxy, Name)                                              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const char*);                \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const bool*);                \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const sc_dt::sc_logic*);     \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const sc_dt::sc_unsigned&);  \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const sc_dt::sc_signed&);    \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const sc_dt::sc_uint_base&); \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, const sc_dt::sc_int_base&);  \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, unsigned long);              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, long);                       \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, unsigned int);               \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, int);                        \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, sc_dt::uint64);              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, Name, const sc_dt::sc_lv_base, sc_dt::int64)
SC_DT_PROXY_ASSERT_BINARY_13(BvProxy, operator|);
SC_DT_PROXY_ASSERT_BINARY_13(BvProxy, operator^);
SC_DT_PROXY_ASSERT_BINARY_13(LvProxy, operator|);
SC_DT_PROXY_ASSERT_BINARY_13(LvProxy, operator^);
SC_DT_PROXY_ASSERT_BINARY_13(CustomBase, operator|);
SC_DT_PROXY_ASSERT_BINARY_13(CustomBase, operator^);
#undef SC_DT_PROXY_ASSERT_BINARY_13
#define SC_DT_PROXY_ASSERT_RELATION_13(Proxy)                                     \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const char*);                \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const bool*);                \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const sc_dt::sc_logic*);     \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const sc_dt::sc_unsigned&);  \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const sc_dt::sc_signed&);    \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const sc_dt::sc_uint_base&); \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, const sc_dt::sc_int_base&);  \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, unsigned long);              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, long);                       \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, unsigned int);               \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, int);                        \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, sc_dt::uint64);              \
  SC_DT_PROXY_ASSERT_MEMBER(Proxy, operator==, bool, sc_dt::int64)
SC_DT_PROXY_ASSERT_RELATION_13(BvProxy);
SC_DT_PROXY_ASSERT_RELATION_13(LvProxy);
SC_DT_PROXY_ASSERT_RELATION_13(CustomBase);
#define SC_DT_PROXY_ASSERT_OTHER_COMPOUNDS(Proxy, Owner)                             \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const char*);                \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const bool*);                \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const sc_dt::sc_logic*);     \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const sc_dt::sc_unsigned&);  \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const sc_dt::sc_signed&);    \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const sc_dt::sc_uint_base&); \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, const sc_dt::sc_int_base&);  \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, unsigned long);              \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, long);                       \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, unsigned int);               \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, int);                        \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, sc_dt::uint64);              \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator|=, Owner, sc_dt::int64);               \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const char*);                \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const bool*);                \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const sc_dt::sc_logic*);     \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const sc_dt::sc_unsigned&);  \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const sc_dt::sc_signed&);    \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const sc_dt::sc_uint_base&); \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, const sc_dt::sc_int_base&);  \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, unsigned long);              \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, long);                       \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, unsigned int);               \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, int);                        \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, sc_dt::uint64);              \
  SC_DT_PROXY_ASSERT_COMPOUND(Proxy, operator^=, Owner, sc_dt::int64)
SC_DT_PROXY_ASSERT_OTHER_COMPOUNDS(BvProxy, sc_dt::sc_bv_base);
SC_DT_PROXY_ASSERT_OTHER_COMPOUNDS(LvProxy, sc_dt::sc_lv_base);
SC_DT_PROXY_ASSERT_OTHER_COMPOUNDS(CustomBase, CustomProxy);
#undef SC_DT_PROXY_ASSERT_OTHER_COMPOUNDS
#undef SC_DT_PROXY_ASSERT_RELATION_13
#undef SC_DT_PROXY_ASSERT_13
#undef SC_DT_PROXY_ASSERT_COMPOUND
#undef SC_DT_PROXY_ASSERT_MEMBER

static_assert(
  std::is_same_v<decltype(static_cast<bool (BvProxy::*)() const>(&BvProxy::and_reduce)), bool (BvProxy::*)() const>);
static_assert(std::is_same_v<decltype(static_cast<sc_dt::sc_logic_value_t (LvProxy::*)() const>(&LvProxy::and_reduce)),
                             sc_dt::sc_logic_value_t (LvProxy::*)() const>);
static_assert(std::has_virtual_destructor_v<BvProxy> && std::has_virtual_destructor_v<LvProxy>);
static_assert(std::is_base_of_v<sc_dt::sc_proxy<CustomProxy>, CustomProxy>);

/** @brief Return the custom proxy's virtual-destruction count after delete.
 * @details This synchronous probe temporarily owns one derived object through
 * a virtual base pointer, invokes no callbacks, and retains no Python state.
 */
inline int virtual_destruction_count()
{
  CustomProxy::destructions = 0;
  std::unique_ptr<sc_dt::sc_proxy<CustomProxy>> owner(new CustomProxy("101001001"));
  owner.reset();
  return CustomProxy::destructions;
}

}  // namespace sc_dt_proxy_probe
