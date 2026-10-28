/** @file Independent native behavior reference for sc_string_old. */
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_string_old_probe.h"

namespace
{
using String = sc_dt::sc_string_old;
using sc_dt_string_old_probe::copied_text;

template <typename... Fields>
void emit(const Fields&... fields)
{
  std::cout << "SO";
  ((std::cout << '\t' << fields), ...);
  std::cout << '\n';
}

int sign(int value)
{
  return (value > 0) - (value < 0);
}

template <typename Left, typename Right>
void compare(const char* right_kind, const char* op, const Left& left, const Right& right, bool result)
{
  emit("compare", right_kind, op, result);
}

int error_mode(const std::string& mode)
{
  const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  try
  {
    String value("abc");
    if (mode == "insert-upper")
      value.insert(String("x"), 4);
    else if (mode == "remove-negative")
      value.remove(static_cast<unsigned>(-1), 1);
    else if (mode == "delimiter-upper")
      value.is_delimiter(String("abc"), 3);
    else
      return 4;
  }
  catch (const sc_core::sc_report& report)
  {
    emit("report", mode, report.get_msg_type(), report.get_msg());
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
    return 0;
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
  return 5;
}

}  // namespace

/** @brief Emit an independent reference matrix for legacy string semantics.
 * @param argc Argument count.
 * @param argv An optional separate report mode.
 * @return Zero after a complete behavior matrix or captured report.
 */
int sc_main(int argc, char** argv)
{
  if (argc > 1) return error_mode(argv[1]);

  emit("type", "sc_dt::sc_string_old", std::is_same<String, std::string>::value);
  const String empty;
  const String reserved(33);
  const String text("alpha");
  const String prefix("alphabet", 4);
  const String copied(text);
  emit("construct", "default", empty.length(), copied_text(empty));
  emit("construct", "size", reserved.length(), copied_text(reserved));
  emit("construct", "char_pointer", text.length(), copied_text(text));
  emit("construct", "prefix", prefix.length(), copied_text(prefix));
  emit("construct", "copy", copied.length(), copied_text(copied));
  emit("visibility", sc_dt_string_old_probe::public_internal_member_mask());

  String copy_source("shared");
  String copy_alias(copy_source);
  copy_alias.set(0, 'S');
  emit("cow", "copy_ctor", copied_text(copy_source), copied_text(copy_alias));
  String assignment_source("assigned");
  String assignment_alias("old");
  assignment_alias = assignment_source;
  assignment_alias.set(0, 'A');
  emit("cow", "copy_assign", copied_text(assignment_source), copied_text(assignment_alias));

  String assigned_pointer("old");
  String* pointer_result = &(assigned_pointer = "from-char-pointer");
  emit("assign", "char_pointer", pointer_result == &assigned_pointer, copied_text(assigned_pointer));
  String assigned_object("old");
  String object_source("from-object");
  String* object_result = &(assigned_object = object_source);
  emit("assign", "object", object_result == &assigned_object, copied_text(assigned_object));

  String appended_pointer("a");
  String* append_pointer_result = &(appended_pointer += "bc");
  emit("append", "char_pointer", append_pointer_result == &appended_pointer, copied_text(appended_pointer));
  String appended_char("a");
  String* append_char_result = &(appended_char += 'b');
  emit("append", "char", append_char_result == &appended_char, copied_text(appended_char));
  String appended_object("a");
  String object_suffix("bc");
  String* append_object_result = &(appended_object += object_suffix);
  emit("append", "object", append_object_result == &appended_object, copied_text(appended_object));

  const String base("alpha");
  const String suffix("bet");
  emit("concat", "char_pointer", copied_text(base + "bet"));
  emit("concat", "char", copied_text(base + '!'));
  emit("concat", "object", copied_text(base + suffix));
  emit("concat", "prefix", sc_dt_string_old_probe::prefixed_text("pre-", suffix));

  const String lhs("beta");
  const String rhs("bet");
  compare("char_equal", "eq", lhs, "beta", lhs == "beta");
  compare("char_unequal", "ne", lhs, "bet", lhs != "bet");
  compare("char_lt", "lt", lhs, "zeta", lhs < "zeta");
  compare("char_le", "le", lhs, "beta", lhs <= "beta");
  compare("char_gt", "gt", lhs, "alpha", lhs > "alpha");
  compare("char_ge", "ge", lhs, "beta", lhs >= "beta");
  compare("object_equal", "eq", lhs, String("beta"), lhs == String("beta"));
  compare("object_unequal", "ne", lhs, rhs, lhs != rhs);
  compare("object_lt", "lt", lhs, String("zeta"), lhs < String("zeta"));
  compare("object_le", "le", lhs, String("beta"), lhs <= String("beta"));
  compare("object_gt", "gt", lhs, String("alpha"), lhs > String("alpha"));
  compare("object_ge", "ge", lhs, String("beta"), lhs >= String("beta"));
  emit("cmp", "char_pointer", sign(lhs.cmp("beta")), sign(lhs.cmp("zeta")), sign(lhs.cmp("alpha")));
  emit("cmp", "object", sign(lhs.cmp(String("beta"))), sign(lhs.cmp(String("zeta"))), sign(lhs.cmp(String("alpha"))));

  String indexed("index");
  const String& const_indexed = indexed;
  const char const_character = sc_dt_string_old_probe::read_character_const(const_indexed, 2);
  const char mutable_character = indexed[3];
  const auto reference_update = sc_dt_string_old_probe::assign_character_reference(indexed, 1, 'X');
  emit("index", "const", const_character);
  emit("index", "mutable_read", mutable_character);
  emit("index", "mutable_reference", reference_update[0], reference_update[1], copied_text(indexed));
  indexed.set(4, '!');
  emit("index", "set", copied_text(indexed));

  const String substr_source("012345");
  emit("substr", "middle", copied_text(substr_source.substr(2, 4)));
  emit("substr", "left_invalid", copied_text(substr_source.substr(-1, 2)));
  emit("substr", "reversed", copied_text(substr_source.substr(4, 2)));
  emit("substr", "right_invalid", copied_text(substr_source.substr(3, 9)));

  String edited("abcdef");
  String* inserted = &(edited.insert(String("XY"), 3));
  emit("insert", "middle", inserted == &edited, copied_text(edited));
  String* removed = &(edited.remove(2, 3));
  emit("remove", "middle", removed == &edited, copied_text(edited));
  String append_end("abc");
  append_end.insert(String("!"), 3);
  emit("insert", "end", copied_text(append_end));

  const String delimiter("-:/");
  const String scan_text("one-two");
  emit("search", scan_text.contains('-'), scan_text.contains('z'), scan_text.is_delimiter(delimiter, 3),
       scan_text.is_delimiter(delimiter, 1), scan_text.pos(String("two")), scan_text.pos(String("missing")),
       scan_text.pos(String("")));
  emit("length", scan_text.length(), copied_text(scan_text), copied_text(String(scan_text.c_str())));
  emit("conversion", std::string(static_cast<const char*>(scan_text)));
  emit("case", copied_text(String("MiXeD").uppercase()), copied_text(String("MiXeD").lowercase()));
  emit("make_str", copied_text(String::make_str(-123456789L)));

  String formatted("%s");
  formatted.fmt(String("object"));
  emit("fmt", "object", copied_text(formatted));
  String templated_int("value=%04d");
  templated_int.fmt(7);
  emit("fmt", "int", copied_text(templated_int));
  String templated_string("name=%s");
  templated_string.fmt("item");
  emit("fmt", "char_pointer", copied_text(templated_string));
  String chained_format("n=%04d s=%s");
  chained_format.fmt(7).fmt("ok");
  emit("fmt", "chain", copied_text(chained_format));
  emit("format_varargs", sc_dt_string_old_probe::formatted_varargs());
  const auto dynamic_short = sc_dt_string_old_probe::formatted_dynamic(1300);
  emit("format_dynamic", 1300, std::get<0>(dynamic_short), std::get<1>(dynamic_short), std::get<2>(dynamic_short));
  const auto dynamic_long = sc_dt_string_old_probe::formatted_dynamic(66000);
  emit("format_truncated", 66000, std::get<0>(dynamic_long), std::get<1>(dynamic_long), std::get<2>(dynamic_long));

  std::ostringstream printed;
  scan_text.print(printed);
  emit("print", printed.str());
  emit("ostream", sc_dt_string_old_probe::streamed_text(scan_text));
  const auto extracted = sc_dt_string_old_probe::extracted_text("  first second");
  emit("istream", extracted.first, extracted.second);
  emit("lifetime", sc_dt_string_old_probe::lifetime_roundtrip(64));
  return 0;
}
