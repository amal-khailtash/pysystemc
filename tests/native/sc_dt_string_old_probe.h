/** @file Native probes for the legacy sc_string_old value type.
 * @brief Exercise private implementation boundaries through public behavior.
 * @details Every string value owns one reference-counted representation and
 *          copies share it until a mutating operation detaches. C++ helpers
 *          return copied strings or values and retain no references. No Python
 *          callbacks or waits occur; call with the GIL held in Python and let
 *          all borrowed character references die before their string owner.
 *          Native report actions and allocation exceptions propagate.
 */
#ifndef TESTS_NATIVE_SC_DT_STRING_OLD_PROBE_H
#define TESTS_NATIVE_SC_DT_STRING_OLD_PROBE_H

#include <sysc/utils/sc_string.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tuple>
#include <type_traits>
#include <utility>

namespace sc_dt_string_old_probe
{
using String = sc_dt::sc_string_old;

static_assert(std::is_copy_constructible<String>::value, "sc_string_old exposes its sharing copy constructor");
static_assert(std::is_copy_assignable<String>::value, "sc_string_old exposes copy assignment");
static_assert(std::is_destructible<String>::value, "sc_string_old exposes its representation-releasing destructor");
static_assert(std::is_same<decltype(std::declval<String&>()[0]), char&>::value,
              "mutable indexing returns a borrowed character reference");
static_assert(std::is_same<decltype(std::declval<const String&>()[0]), char>::value,
              "const indexing returns a copied character");
static_assert(std::is_same<decltype(operator<<(std::declval<std::ostream&>(), std::declval<const String&>())),
                           std::ostream&>::value,
              "stream insertion returns the supplied borrowed stream");
static_assert(
  std::is_same<decltype(operator>>(std::declval<std::istream&>(), std::declval<String&>())), std::istream&>::value,
  "stream extraction returns the supplied borrowed stream");
static_assert(!std::is_constructible<String, sc_dt::sc_string_rep*>::value,
              "the representation-adopting constructor remains private");

template <typename T, typename = void>
struct has_public_rep : std::false_type
{
};

template <typename T>
struct has_public_rep<T, std::void_t<decltype(std::declval<T&>().rep)>> : std::true_type
{
};

template <typename T, typename = void>
struct has_public_test_hook : std::false_type
{
};

template <typename T>
struct has_public_test_hook<T, std::void_t<decltype(std::declval<const T&>().test(0))>> : std::true_type
{
};

template <typename T, typename = void>
struct has_public_format_scanner : std::false_type
{
};

template <typename T>
struct has_public_format_scanner<T, std::void_t<decltype(std::declval<const T&>().fmt_length())>> : std::true_type
{
};

static_assert(!has_public_rep<String>::value, "representation storage remains private");
static_assert(!has_public_test_hook<String>::value, "the bounds-abort hook remains private");
static_assert(!has_public_format_scanner<String>::value, "format scanning remains private");

/** @brief Report compile-time visibility of legacy implementation details.
 * @return Bit mask with one bit per accidentally public private member.
 */
inline unsigned public_internal_member_mask()
{
  return unsigned(has_public_rep<String>::value) | (unsigned(has_public_test_hook<String>::value) << 1) |
         (unsigned(has_public_format_scanner<String>::value) << 2);
}

/** @brief Copy a legacy string into ordinary owned text.
 * @param value Borrowed native value.
 * @return Independent standard string copy.
 */
inline std::string copied_text(const String& value)
{
  return value.c_str();
}

/** @brief Assign through the public mutable character-reference overload.
 * @param value Mutable borrowed string.
 * @param index Valid byte index.
 * @param character Replacement byte.
 * @return Old and new character values copied into a pair.
 */
inline std::string assign_character_reference(String& value, int index, char character)
{
  char& slot = value[index];
  const char previous = slot;
  slot = character;
  return {previous, slot};
}

/** @brief Read a character through the exact const indexing overload.
 * @param value Borrowed const string.
 * @param index Valid byte index.
 * @return Character value copied by the native const overload.
 */
inline char read_character_const(const String& value, int index)
{
  return value[index];
}

/** @brief Invoke the hidden friend output overload through native ADL.
 * @param value Borrowed string.
 * @return Independent stream text.
 */
inline std::string streamed_text(const String& value)
{
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

/** @brief Invoke the hidden friend input overload and return extracted text.
 * @param input Whitespace-delimited input bytes.
 * @return Extracted value and the next remaining token.
 */
inline std::pair<std::string, std::string> extracted_text(const char* input)
{
  std::istringstream stream(input);
  String value("old");
  stream >> value;
  std::string next;
  stream >> next;
  return {value.c_str(), next};
}

/** @brief Invoke the namespace-level `const char* + sc_string_old` overload.
 * @param prefix Borrowed null-terminated prefix.
 * @param suffix Borrowed string.
 * @return Result text copied before temporary destruction.
 */
inline std::string prefixed_text(const char* prefix, const String& suffix)
{
  return (prefix + suffix).c_str();
}

/** @brief Call printf-style formatting with correctly typed native varargs.
 * @return Formatted text copied from the owned native result.
 * @details The native C varargs format and arguments must match exactly.
 */
inline std::string formatted_varargs()
{
  const auto result = String::to_string("%s:%d:%.2f", "item", 17, 1.25);
  return result.c_str();
}

/** @brief Exercise the template formatter with a native integer argument.
 * @return Correctly formatted owned result copied to standard text.
 */
inline std::string formatted_template_int()
{
  String result("value=%04d");
  result.fmt(7);
  return result.c_str();
}

/** @brief Exercise the dynamic allocation path with a literal-only format.
 * @param count Number of literal bytes to produce.
 * @return Length, prefix and suffix copied from the owned result.
 */
inline std::tuple<int, char, char> formatted_dynamic(int count)
{
  const std::string pattern(static_cast<std::size_t>(count), 'D');
  const auto result = String::to_string(pattern.c_str());
  return {result.length(), result[0], result[result.length() - 1]};
}

/** @brief Exercise representation sharing, copy-on-write, and destruction.
 * @param rounds Number of short-lived copy/mutation lifetimes.
 * @return Number of verified copy-on-write rounds.
 * @throws sc_core::sc_report If a representation invariant fails.
 */
inline int lifetime_roundtrip(int rounds)
{
  for (int index = 0; index < rounds; ++index)
  {
    String original("shared representation");
    String copy(original);
    copy.set(0, 'S');
    if (original.c_str()[0] != 's' || copy.c_str()[0] != 'S')
      throw std::runtime_error("sc_string_old copy-on-write mismatch");

    String assigned("discard me");
    assigned = original;
    assigned[1] = 'H';
    if (original.c_str()[1] != 'h' || assigned.c_str()[1] != 'H')
      throw std::runtime_error("sc_string_old assignment detach mismatch");
  }
  return rounds;
}

}  // namespace sc_dt_string_old_probe

#endif  // TESTS_NATIVE_SC_DT_STRING_OLD_PROBE_H
