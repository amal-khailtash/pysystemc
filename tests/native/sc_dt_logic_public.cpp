/** @file sc_dt_logic_public.cpp
 * @brief Native reference for the public sc_bit and sc_logic member APIs.
 * @details Emits constructor, assignment, conversion, compound-operation,
 * table, stream, fallback, and lifetime observations for Python comparison.
 */
#include <array>
#include <iostream>
#include <string>

#include "sc_dt_logic_probe.h"

int sc_main(int argc, char** argv)
{
  using sc_dt::sc_bit;
  using sc_dt::sc_logic;
  if (argc > 1 && std::string(argv[1]).find("bit-error") == 0)
  {
    if (std::string(argv[1]) == "bit-error-int")
    {
      sc_bit invalid(2);
    }
    else
    {
      sc_bit invalid('Q');
    }
    return 4;
  }
  auto emit = [](const auto&... fields)
  {
    std::cout << "DT_LOGIC";
    ((std::cout << '\t' << fields), ...);
    std::cout << '\n';
  };
  const std::array<char, 2> bits{'0', '1'};
  const std::array<char, 4> states{'0', '1', 'Z', 'X'};

  sc_bit default_bit;
  emit("bit_ctor", "default", default_bit.to_char());
  emit("bit_ctor", "copy", sc_bit(default_bit).to_char());
  for (char text : bits)
  {
    const bool boolean = text == '1';
    const int integer = boolean ? 1 : 0;
    emit("bit_ctor", "bool", text, sc_bit(boolean).to_char());
    emit("bit_ctor", "char", text, sc_bit(text).to_char());
    emit("bit_ctor", "int", text, sc_bit(integer).to_char());
    emit("bit_ctor", "unsigned", text, sc_bit(sc_dt_logic_probe::Unsigned(integer)).to_char());
    emit("bit_ctor", "long", text, sc_bit(sc_dt_logic_probe::Long(integer)).to_char());
    emit("bit_ctor", "unsigned_long", text, sc_bit(sc_dt_logic_probe::UnsignedLong(integer)).to_char());
    emit("bit_ctor", "int64", text, sc_bit(sc_dt_logic_probe::Int64(integer)).to_char());
    emit("bit_ctor", "uint64", text, sc_bit(sc_dt_logic_probe::UInt64(integer)).to_char());
  }
  for (char state : states)
  {
    emit("bit_ctor", "logic", state, sc_bit(sc_logic(state)).to_char());
  }

  for (char text : bits)
  {
    const bool boolean = text == '1';
    const int integer = boolean ? 1 : 0;
    sc_bit value(false);
    value = text;
    emit("bit_assign", "char", text, value.to_char());
    value = boolean;
    emit("bit_assign", "bool", text, value.to_char());
    value = integer;
    emit("bit_assign", "int", text, value.to_char());
    value = sc_dt_logic_probe::Long(integer);
    emit("bit_assign", "long", text, value.to_char());
    value = sc_dt_logic_probe::UnsignedLong(integer);
    emit("bit_assign", "unsigned_long", text, value.to_char());
    value = sc_dt_logic_probe::Int64(integer);
    emit("bit_assign", "int64", text, value.to_char());
    value = sc_dt_logic_probe::UInt64(integer);
    emit("bit_assign", "uint64", text, value.to_char());
  }
  for (char state : states)
  {
    sc_bit value(false);
    value = sc_logic(state);
    emit("bit_assign", "logic", state, value.to_char());
  }
  sc_bit bit_assigned(false);
  bit_assigned = sc_bit(true);
  emit("bit_assign", "bit", "1", bit_assigned.to_char());

  for (char operation : std::array<char, 3>{'a', 'o', 'x'})
  {
    const std::string operation_name = operation == 'a' ? "and" : operation == 'o' ? "or" : "xor";
    for (char left_char : bits)
    {
      for (const std::string type_name : {"bit", "char", "bool", "int"})
      {
        for (char right_char : bits)
        {
          sc_bit left(left_char);
          const bool right_bool = right_char == '1';
          const int right_int = right_bool ? 1 : 0;
          if (type_name == "bit")
          {
            sc_bit right(right_char);
            if (operation == 'a') left &= right;
            if (operation == 'o') left |= right;
            if (operation == 'x') left ^= right;
          }
          else if (type_name == "char")
          {
            if (operation == 'a') left &= right_char;
            if (operation == 'o') left |= right_char;
            if (operation == 'x') left ^= right_char;
          }
          else if (type_name == "bool")
          {
            if (operation == 'a') left &= right_bool;
            if (operation == 'o') left |= right_bool;
            if (operation == 'x') left ^= right_bool;
          }
          else
          {
            if (operation == 'a') left &= right_int;
            if (operation == 'o') left |= right_int;
            if (operation == 'x') left ^= right_int;
          }
          emit("bit_compound", operation_name, left_char, type_name, right_char, left.to_char());
        }
      }
    }
  }
  for (char state : bits)
  {
    sc_bit value(state);
    emit("bit_convert", state, int(bool(value)), int(!value), int(value.to_bool()), value.to_char(),
         sc_dt_logic_probe::print_bit(value));
    value.b_not();
    emit("bit_b_not", state, value.to_char());
  }
  for (char state : bits)
  {
    emit("bit_scan", state, sc_dt_logic_probe::scan_bit(std::string(1, state).c_str()).to_char());
  }
  const auto previous_bit_error_actions = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  auto check_invalid_bit_operation = [](const char* operation_name, const char* type_name, auto operation)
  {
    sc_bit value(false);
    bool caught = false;
    try
    {
      operation(value);
    }
    catch (const sc_core::sc_report&)
    {
      caught = true;
    }
    if (!caught || value.to_char() != '0') return false;
    std::cout << "DT_LOGIC\tbit_invalid_operation\t" << operation_name << '\t' << type_name << "\tcaught\t"
              << value.to_char() << '\n';
    return true;
  };
  const bool invalid_operations_ok =
    check_invalid_bit_operation("assign", "int", [](sc_bit& value) { value = 2; }) &&
    check_invalid_bit_operation("assign", "char", [](sc_bit& value) { value = 'Q'; }) &&
    check_invalid_bit_operation("and", "int", [](sc_bit& value) { value &= 2; }) &&
    check_invalid_bit_operation("and", "char", [](sc_bit& value) { value &= 'Q'; }) &&
    check_invalid_bit_operation("or", "int", [](sc_bit& value) { value |= 2; }) &&
    check_invalid_bit_operation("or", "char", [](sc_bit& value) { value |= 'Q'; }) &&
    check_invalid_bit_operation("xor", "int", [](sc_bit& value) { value ^= 2; }) &&
    check_invalid_bit_operation("xor", "char", [](sc_bit& value) { value ^= 'Q'; });
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous_bit_error_actions);
  if (!invalid_operations_ok) return 5;
  emit("logic_ctor", "default", sc_logic().to_char());
  for (char state : states)
  {
    sc_logic original(state);
    emit("logic_ctor", "copy", state, sc_logic(original).to_char());
  }
  for (int value = 0; value < 4; ++value)
  {
    const char state = states[static_cast<std::size_t>(value)];
    emit("logic_ctor", "enum", state, sc_logic(static_cast<sc_dt::sc_logic_value_t>(value)).to_char());
    emit("logic_ctor", "int", state, sc_logic(value).to_char());
  }
  for (char state : bits)
  {
    emit("logic_ctor", "bool", state, sc_logic(state == '1').to_char());
  }
  for (char state : states)
  {
    emit("logic_ctor", "char", state, sc_logic(state).to_char());
  }
  for (char state : bits)
  {
    emit("logic_ctor", "bit", state, sc_logic(sc_bit(state)).to_char());
  }

  for (char state : states)
  {
    const int index = static_cast<int>(std::string("01ZX").find(state));
    sc_logic value('0');
    value = static_cast<sc_dt::sc_logic_value_t>(index);
    emit("logic_assign", state, "enum", value.to_char());
    value = state == '1';
    emit("logic_assign", state, "bool", value.to_char());
    value = state;
    emit("logic_assign", state, "char", value.to_char());
    value = index;
    emit("logic_assign", state, "int", value.to_char());
    value = sc_bit(state == '0' ? '0' : '1');
    emit("logic_assign", state, "bit", value.to_char());
    value = sc_logic(state);
    emit("logic_assign", state, "logic", value.to_char());
  }

  for (char operation : std::array<char, 3>{'a', 'o', 'x'})
  {
    const std::string operation_name = operation == 'a' ? "and" : operation == 'o' ? "or" : "xor";
    for (char left_char : states)
    {
      for (const std::string type_name : {"logic", "enum", "bool", "char", "int", "bit"})
      {
        for (char right_char : states)
        {
          const int right_index = static_cast<int>(std::string("01ZX").find(right_char));
          sc_logic left(left_char);
          if (type_name == "logic")
          {
            const sc_logic right(right_char);
            if (operation == 'a') left &= right;
            if (operation == 'o') left |= right;
            if (operation == 'x') left ^= right;
          }
          else if (type_name == "enum")
          {
            const auto right = static_cast<sc_dt::sc_logic_value_t>(right_index);
            if (operation == 'a') left &= right;
            if (operation == 'o') left |= right;
            if (operation == 'x') left ^= right;
          }
          else if (type_name == "bool")
          {
            const bool right = right_char != '0';
            if (operation == 'a') left &= right;
            if (operation == 'o') left |= right;
            if (operation == 'x') left ^= right;
          }
          else if (type_name == "char")
          {
            if (operation == 'a') left &= right_char;
            if (operation == 'o') left |= right_char;
            if (operation == 'x') left ^= right_char;
          }
          else if (type_name == "int")
          {
            if (operation == 'a') left &= right_index;
            if (operation == 'o') left |= right_index;
            if (operation == 'x') left ^= right_index;
          }
          else
          {
            const sc_bit right(right_char == '0' ? '0' : '1');
            if (operation == 'a') left &= right;
            if (operation == 'o') left |= right;
            if (operation == 'x') left ^= right;
          }
          emit("logic_compound", operation_name, left_char, type_name, right_char, left.to_char());
        }
      }
    }
  }

  for (char state : states)
  {
    sc_logic value(state);
    emit("logic_convert", state, int(value.value()), int(value.is_01()), int(value.to_bool()), value.to_char(),
         sc_dt_logic_probe::print_logic(value), (~value).to_char());
    value.b_not();
    emit("logic_b_not", state, value.to_char());
  }
  for (char state : states)
  {
    emit("logic_scan", state, sc_dt_logic_probe::scan_logic(std::string(1, state).c_str()).to_char());
  }

  const std::array<const char*, 3> binary_table_names{"and_table", "or_table", "xor_table"};
  const std::array<const sc_dt::sc_logic_value_t(*)[4], 3> binary_tables{sc_logic::and_table, sc_logic::or_table,
                                                                         sc_logic::xor_table};
  for (std::size_t table = 0; table < binary_tables.size(); ++table)
  {
    std::string values;
    for (int row = 0; row < 4; ++row)
    {
      for (int column = 0; column < 4; ++column)
      {
        if (!values.empty()) values += ',';
        values += std::to_string(binary_tables[table][row][column]);
      }
    }
    emit("logic_table", binary_table_names[table], values);
  }
  std::string not_values;
  for (int index = 0; index < 4; ++index)
  {
    if (!not_values.empty()) not_values += ',';
    not_values += std::to_string(sc_logic::not_table[index]);
  }
  emit("logic_table", "not_table", not_values);
  emit("logic_table", "logic_to_char", std::string(sc_logic::logic_to_char, 4));
  std::string char_values;
  for (int index = 0; index < 128; ++index)
  {
    if (!char_values.empty()) char_values += ',';
    char_values += std::to_string(sc_logic::char_to_logic[index]);
  }
  emit("logic_table", "char_to_logic", char_values);
  const auto previous_error_actions =
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_DO_NOTHING);
  emit("logic_invalid", "int", sc_logic(4).to_char());
  emit("logic_invalid", "char", sc_logic('Q').to_char());
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous_error_actions);

  sc_dt_logic_probe::DestructionCounters counters;
  for (int index = 0; index < 512; ++index)
  {
    {
      sc_dt_logic_probe::CountedBit value(counters, (index & 1) != 0);
      if (value.to_char() != (index & 1 ? '1' : '0')) return 2;
    }
    {
      sc_dt_logic_probe::CountedLogic value(counters, states[static_cast<std::size_t>(index & 3)]);
      if (value.to_char() != states[static_cast<std::size_t>(index & 3)]) return 3;
    }
  }
  if (counters.bit_count != 512 || counters.logic_count != 512) return 4;
  emit("lifetime", counters.bit_count, counters.logic_count);
  return 0;
}
