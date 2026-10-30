/** @file datatype_adapter_edges.cpp
 * @brief Native SystemC references for selected datatype adapter edge paths.
 * @details Concatenation references use native temporary-pool storage and
 *          borrow their numeric parents, which remain live through updates.
 *          Fixed-range snapshots own their copied bits after their owners and
 *          views leave scope. All work is synchronous, holds no Python
 *          references or callbacks, and performs no kernel reset or waits.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

#include "../../src/amal/eda/systemc/sc_dt/concat_adapters.h"
#include "sc_dt_fixed_subrefs_probe.h"

namespace
{
/** @brief Emit one tab-separated record for native/Python comparison. */
template <class... Values>
void emit(const char* family, const Values&... values)
{
  std::cout << "DAE\t" << family;
  using swallow = int[];
  (void)swallow{0, ((std::cout << '\t' << values), 0)...};
  std::cout << '\n';
}

/** @brief Render a native concatenation shell without copying its leaves. */
std::string binary(const sc_dt::sc_concatref& value)
{
  return value.to_string(sc_dt::SC_BIN, true);
}

/** @brief Render an owned native vector snapshot in binary notation. */
template <class Vector>
std::string binary(const Vector& value)
{
  return value.to_string(sc_dt::SC_BIN, true);
}

/** @brief Compare text-ended native concat shells before and after parent writes. */
void text_concatenation()
{
  sc_dt::sc_uint<13> head_owner(22);
  const auto& head = sc_dt::concat("101", head_owner);
  emit("CONCAT", "head", "initial", binary(head));
  head_owner = 5;
  emit("CONCAT", "head", "updated", binary(head), head_owner.to_uint());

  sc_dt::sc_uint<13> tail_owner(22);
  const auto& tail = sc_dt::concat(tail_owner, "011");
  emit("CONCAT", "tail", "initial", binary(tail));
  tail_owner = 5;
  emit("CONCAT", "tail", "updated", binary(tail), tail_owner.to_uint());

  sc_dt::sc_bv<8> vector_head_owner("01011010");
  const auto& vector_head = sc_dt::concat("10", vector_head_owner);
  emit("CONCAT", "vector_head", "initial", binary(vector_head));
  vector_head_owner = "10101010";
  emit("CONCAT", "vector_head", "updated", binary(vector_head), binary(vector_head_owner));

  sc_dt::sc_bv<8> vector_tail_owner("01011010");
  const auto& vector_tail = sc_dt::concat(vector_tail_owner, "10");
  emit("CONCAT", "vector_tail", "initial", binary(vector_tail));
  vector_tail_owner = "10101010";
  emit("CONCAT", "vector_tail", "updated", binary(vector_tail), binary(vector_tail_owner));
}

/** @brief Copy a const fixed range and check parent and view destruction. */
template <class Number, class SelectRange>
void fixed_range_copy(const char* label, SelectRange select_range)
{
  sc_dt::sc_bv<8> copied;
  std::string initial;
  {
    Number parent("5.25");
    const Number& readonly_parent = parent;
    auto view = select_range(readonly_parent, 11, 4);
    copied = sc_dt::sc_bv<8>(static_cast<sc_dt::sc_bv_base>(view));
    initial = binary(copied);
    emit("RANGE", label, "copy", initial, copied.length());
    parent = 0;
    emit("RANGE", label, "parent_updated", parent.to_string(sc_dt::SC_DEC), binary(copied));
  }
  emit("RANGE", label, "parent_deleted", 1, binary(copied));
}

/** @brief Copy a mutable fixed range and check independent vector ownership. */
template <class Number>
void mutable_fixed_range_copy(const char* label)
{
  sc_dt::sc_bv<8> copied;
  {
    Number parent("5.25");
    auto view = parent.range(11, 4);
    copied = sc_dt::sc_bv<8>(static_cast<sc_dt::sc_bv_base>(view));
    emit("RANGE", label, "copy", binary(copied), copied.length());
    parent = 0;
    emit("RANGE", label, "parent_updated", binary(view), binary(copied));
  }
  emit("RANGE", label, "parent_deleted", 1, binary(copied));
}

/** @brief Assign a decimal integer above 64 bits using native text conversion. */
void exact_integer_assignment()
{
  sc_dt::sc_fxval target("0");
  target = "18446744073709551637";
  emit("ASSIGN", "wide_integer", target.to_string(sc_dt::SC_DEC));
}

/** @brief Check standard string extraction, chaining and stream state natively. */
void ordinary_stream_extraction()
{
  std::istringstream stream("first second");
  std::string first;
  std::string second;
  stream >> first >> second;
  std::string third;
  stream >> third;
  emit("STREAM", "ordinary_string", first, second, third, stream.eof(), stream.fail());

  std::istringstream integer_stream("41");
  int integer_target = 0;
  integer_stream >> integer_target;
  emit("STREAM", "mutable_ctypes_int", integer_target, integer_stream.eof(), integer_stream.fail());
}
}  // namespace

/** @brief Detect whether a standard stream can extract into the given target. */
template <class Stream, class Target, class = void>
struct CanExtract : std::false_type
{
};

template <class Stream, class Target>
struct CanExtract<Stream, Target, std::void_t<decltype(std::declval<Stream&>() >> std::declval<Target>())>>
  : std::true_type
{
};

using IntegerOwner = sc_dt::sc_uint<13>;
using IntegerTextAdapter = pysystemc_datatypes::ConcatText<IntegerOwner>;
using IntegerTextHead =
  decltype(IntegerTextAdapter::head(std::declval<const std::string&>(), std::declval<IntegerOwner&>()));
using IntegerTextTail =
  decltype(IntegerTextAdapter::tail(std::declval<IntegerOwner&>(), std::declval<const std::string&>()));
using NativeIntegerTextHead = decltype(sc_dt::concat(std::declval<const char*>(), std::declval<IntegerOwner&>()));
using NativeIntegerTextTail = decltype(sc_dt::concat(std::declval<IntegerOwner&>(), std::declval<const char*>()));
static_assert(std::is_same_v<IntegerTextHead, NativeIntegerTextHead>);
static_assert(std::is_same_v<IntegerTextTail, NativeIntegerTextTail>);
static_assert(std::is_lvalue_reference_v<IntegerTextHead>);
static_assert(std::is_lvalue_reference_v<IntegerTextTail>);

using VectorOwner = sc_dt::sc_bv<8>;
using VectorTextAdapter = pysystemc_datatypes::ConcatText<VectorOwner>;
using VectorTextHead =
  decltype(VectorTextAdapter::head(std::declval<const std::string&>(), std::declval<VectorOwner&>()));
using VectorTextTail =
  decltype(VectorTextAdapter::tail(std::declval<VectorOwner&>(), std::declval<const std::string&>()));
using NativeVectorTextHead = decltype(sc_dt::concat(std::declval<const char*>(), std::declval<VectorOwner&>()));
using NativeVectorTextTail = decltype(sc_dt::concat(std::declval<VectorOwner&>(), std::declval<const char*>()));
static_assert(std::is_same_v<VectorTextHead, NativeVectorTextHead>);
static_assert(std::is_same_v<VectorTextTail, NativeVectorTextTail>);
static_assert(!std::is_reference_v<VectorTextHead>);
static_assert(!std::is_reference_v<VectorTextTail>);

using BitVector = sc_dt::sc_bv<8>;
using BitReference = decltype(std::declval<BitVector&>()[0]);
using BoolVectorConcat =
  decltype(pysystemc_datatypes::BoolBitConcat<BitReference>::head(false, std::declval<BitReference>()));
static_assert(!std::is_reference_v<BoolVectorConcat>);
static_assert(CanExtract<std::istream, std::string&>::value);
static_assert(CanExtract<std::istream, int&>::value);
static_assert(!CanExtract<std::istream, int>::value, "standard extraction requires a mutable target lvalue");

/** @brief Emit native parity records for selected datatype adapters. */
int sc_main(int, char**)
{
  text_concatenation();
  fixed_range_copy<sc_dt::sc_fixed<16, 8>>(
    "slow", [](const sc_dt::sc_fixed<16, 8>& value, int high, int low) { return value.range(high, low); });
  fixed_range_copy<sc_dt::sc_fixed_fast<16, 8>>(
    "fast", [](const sc_dt::sc_fixed_fast<16, 8>& value, int high, int low) { return value.range(high, low); });
  mutable_fixed_range_copy<sc_dt::sc_fixed<16, 8>>("mutable_slow");
  mutable_fixed_range_copy<sc_dt::sc_fixed_fast<16, 8>>("mutable_fast");
  exact_integer_assignment();
  ordinary_stream_extraction();
  return 0;
}
