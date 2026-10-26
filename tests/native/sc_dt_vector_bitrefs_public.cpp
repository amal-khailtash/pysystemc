/** @file Independent vector bit-reference overload and lifetime reference. */
#include <iostream>
#include <stdexcept>

#include "sc_dt_vector_bitrefs_probe.h"
using namespace sc_dt;
/** @brief Enforce checks even when native sc_assert is disabled. */
void verify(bool value)
{
  if (!value) throw std::runtime_error("vector bitref parity");
}
/** @brief Print copied deterministic fields. */
template <class... Args>
void emit(Args... args)
{
  std::cout << "VBR";
  ((std::cout << ' ' << args), ...);
  std::cout << '\n';
}
/** @brief Compare all native assign and compound overloads for an operand. */
template <class T, class A>
void operand(sc_bitref<T>& view, int width, bool binary, char state, const char* kind, const A& argument)
{
  view = state;
  auto& assigned = (view = argument);
  verify(&assigned == &view);
  emit(width, binary, "assign", state, kind, view.to_char());
  if constexpr (!std::is_same<A, sc_bit>::value)
  {
    view = state;
    auto& conjunction = (view &= argument);
    verify(&conjunction == &view);
    emit(width, binary, "iand", state, kind, view.to_char());
    view = state;
    auto& disjunction = (view |= argument);
    verify(&disjunction == &view);
    emit(width, binary, "ior", state, kind, view.to_char());
    view = state;
    auto& exclusive = (view ^= argument);
    verify(&exclusive == &view);
    emit(width, binary, "ixor", state, kind, view.to_char());
  }
}
/** @brief Run an independent native matrix with live vector parents. */
template <class T>
void exercise(int width, bool binary)
{
  T parent = [](int n)
  {
    if constexpr (std::is_same<T, sc_lv_base>::value || std::is_same<T, sc_bv_base>::value)
      return T(n);
    else
      return T();
  }(width);
  parent = std::string(width, '0').c_str();
  sc_bitref<T> view(parent, 67), copy(view);
  sc_bitref_r<T> read(parent, 67), read_copy(read);
  std::unique_ptr<sc_bitref<T>> clone(view.clone());
  std::unique_ptr<sc_bitref_r<T>> read_clone(read.clone());
  const std::string states = binary ? "01" : "01XZ";
  for (char state : states)
  {
    view = state;
    auto complement = ~read;
    auto converted = vector_bitref_probe::converted(read);
    char complemented, conversion;
    if constexpr (std::is_same<typename T::traits_type::bit_type, bool>::value)
    {
      complemented = complement ? '1' : '0';
      conversion = converted ? '1' : '0';
    }
    else
    {
      complemented = complement.to_char();
      conversion = converted.to_char();
    }
    emit(width, binary, "read", state, read.length(), read.size(), int(read.value()), read.is_01(), read.to_bool(),
         read.to_bool(), !read.to_bool(), read.to_char(), int(read.get_bit(0)), read.get_word(0), read.get_cword(0),
         complemented, conversion, vector_bitref_probe::printed(read), copy.to_char(), read_copy.to_char(),
         clone->to_char(), read_clone->to_char());
  }
  T other = [](int n)
  {
    if constexpr (std::is_same<T, sc_lv_base>::value || std::is_same<T, sc_bv_base>::value)
      return T(n);
    else
      return T();
  }(width);
  other = std::string(width, '0').c_str();
  sc_bitref<T> other_view(other, 67);
  sc_bitref_r<T> other_read(other, 67);
  for (char state : states)
  {
    other_view = state;
    operand(view, width, binary, state, "bool", false);
    operand(view, width, binary, state, "bool", true);
    operand(view, width, binary, state, "int", 0);
    operand(view, width, binary, state, "int", 1);
    operand(view, width, binary, state, "char", '0');
    operand(view, width, binary, state, "char", '1');
    operand(view, width, binary, state, "enum", Log_0);
    operand(view, width, binary, state, "enum", Log_1);
    operand(view, width, binary, state, "logic", sc_logic('0'));
    operand(view, width, binary, state, "logic", sc_logic('1'));
    operand(view, width, binary, state, "bit", sc_bit(false));
    operand(view, width, binary, state, "bit", sc_bit(true));
    if (!binary)
    {
      operand(view, width, binary, state, "int", 2);
      operand(view, width, binary, state, "int", 3);
      operand(view, width, binary, state, "char", 'X');
      operand(view, width, binary, state, "char", 'Z');
      operand(view, width, binary, state, "enum", Log_X);
      operand(view, width, binary, state, "enum", Log_Z);
      operand(view, width, binary, state, "logic", sc_logic('X'));
      operand(view, width, binary, state, "logic", sc_logic('Z'));
    }
    operand(view, width, binary, state, "read", other_read);
    operand(view, width, binary, state, "mutable", other_view);
  }
  for (char state : states)
  {
    view = state;
    verify(&view.b_not() == &view);
    emit(width, binary, "b_not", state, view.to_char());
  }
  view.set_bit(0, Log_1);
  view.set_word(0, 0);
  view.set_cword(0, binary ? 0 : 1);
  view.clean_tail();
  emit(width, binary, "words", view.to_char(), view.get_word(0), view.get_cword(0));
  for (char state : states)
  {
    std::istringstream stream(std::string(1, state));
    view.scan(stream);
    emit(width, binary, "scan", state, view.to_char());
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  const char* methods[] = {"get_bit", "get_word", "get_cword", "set_bit", "set_word", "set_cword"};
  for (int method = 0; method < 6; ++method)
  {
    const std::string before = parent.to_string();
    bool threw = false;
    try
    {
      switch (method)
      {
      case 0:
        read.get_bit(1);
        break;
      case 1:
        read.get_word(1);
        break;
      case 2:
        read.get_cword(1);
        break;
      case 3:
        view.set_bit(1, Log_0);
        break;
      case 4:
        view.set_word(1, 0);
        break;
      case 5:
        view.set_cword(1, 0);
        break;
      }
    }
    catch (const sc_core::sc_report& error)
    {
      threw = true;
      emit(width, binary, "bounds", methods[method], error.get_id(), before == parent.to_string());
    }
    verify(threw);
  }
  const char* operations[] = {"__assign__", "__iand__", "__ior__", "__ixor__"};
  for (int operation = 0; operation < 4; ++operation)
    for (int invalid = 0; invalid < 2; ++invalid)
    {
      const std::string before = parent.to_string();
      bool threw = false;
      try
      {
        if (invalid == 0)
        {
          switch (operation)
          {
          case 0:
            view = 4;
            break;
          case 1:
            view &= 4;
            break;
          case 2:
            view |= 4;
            break;
          case 3:
            view ^= 4;
            break;
          }
        }
        else
        {
          switch (operation)
          {
          case 0:
            view = static_cast<char>(128);
            break;
          case 1:
            view &= static_cast<char>(128);
            break;
          case 2:
            view |= static_cast<char>(128);
            break;
          case 3:
            view ^= static_cast<char>(128);
            break;
          }
        }
      }
      catch (const sc_core::sc_report&)
      {
        threw = true;
        emit(width, binary, "invalid", operations[operation], invalid == 0 ? "4" : "128", before == parent.to_string());
      }
      verify(threw);
    }
}
/** @brief Run wide native vectors, preserving warning/error policy semantics. */
int sc_main(int, char**)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);
  verify(vector_bitref_probe::traits());
  // Base classes use the exact configured widths; template cases prove arbitrary T.
  exercise<sc_lv_base>(137, false);
  exercise<sc_bv_base>(191, true);
  exercise<sc_lv<191>>(191, false);
  exercise<sc_bv<137>>(137, true);
  sc_int<19> signed_owner(1);
  sc_uint<19> unsigned_owner(1);
  emit("legacy_bitwidth", "sc_dt::sc_int<19>", signed_owner[0].length());
  emit("legacy_bitwidth", "sc_dt::sc_uint<19>", unsigned_owner[0].length());
  std::cout << "VECTOR_BITREFS_PUBLIC_OK\n";
  return 0;
}
