/** @file Independent native free datatype operation reference. */
#include <iostream>
#include <stdexcept>

#include "sc_dt_free_operations_probe.h"
using namespace datatype_free_probe;
/** @brief Compare actual conditions independently of native assertion flags. */
void verify(bool condition)
{
  if (!condition) throw std::runtime_error("free datatype comparison");
}
/** @brief Print copied scalar/vector/fixed values for exact native comparison. */
template <class T>
void emit(const char* label, const T& value)
{
  std::cout << "DT_FREE " << label << ' ' << value.to_string(sc_dt::SC_BIN) << '\n';
}
void emit(const char* label, const sc_dt::sc_bit& value)
{
  std::cout << "DT_FREE " << label << ' ' << value.to_char() << '\n';
}
/** @brief Preserve X/Z when printing vector compound results. */
void emit(const char* label, const sc_dt::sc_lv_base& value)
{
  std::cout << "DT_FREE " << label << ' ' << value.to_string() << '\n';
}
/** @brief Print binary vector results without numeric re-encoding. */
void emit(const char* label, const sc_dt::sc_bv_base& value)
{
  std::cout << "DT_FREE " << label << ' ' << value.to_string() << '\n';
}
int sc_main(int, char**)
{
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|11", output);
  }
  emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", sc_dt::b_and(sc_dt::sc_bit(0), sc_dt::sc_bit(0)));
  emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", sc_dt::b_and(sc_dt::sc_bit(0), sc_dt::sc_bit(1)));
  emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", sc_dt::b_and(sc_dt::sc_bit(1), sc_dt::sc_bit(0)));
  emit("b_and|const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", sc_dt::b_and(sc_dt::sc_bit(1), sc_dt::sc_bit(1)));
  emit("b_and|bool,const sc_dt::sc_bit &|00", sc_dt::b_and(false, sc_dt::sc_bit(0)));
  emit("b_and|bool,const sc_dt::sc_bit &|01", sc_dt::b_and(false, sc_dt::sc_bit(1)));
  emit("b_and|bool,const sc_dt::sc_bit &|10", sc_dt::b_and(true, sc_dt::sc_bit(0)));
  emit("b_and|bool,const sc_dt::sc_bit &|11", sc_dt::b_and(true, sc_dt::sc_bit(1)));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 0;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 0;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 1;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 1;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = '0';
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = '1';
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = '0';
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = '1';
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|11", output);
  }
  emit("b_and|const sc_dt::sc_bit &,int|00", sc_dt::b_and(sc_dt::sc_bit(0), 0));
  emit("b_and|const sc_dt::sc_bit &,int|01", sc_dt::b_and(sc_dt::sc_bit(0), 1));
  emit("b_and|const sc_dt::sc_bit &,int|10", sc_dt::b_and(sc_dt::sc_bit(1), 0));
  emit("b_and|const sc_dt::sc_bit &,int|11", sc_dt::b_and(sc_dt::sc_bit(1), 1));
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = false;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = false;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = true;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = true;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|11", output);
  }
  emit("b_and|int,const sc_dt::sc_bit &|00", sc_dt::b_and(0, sc_dt::sc_bit(0)));
  emit("b_and|int,const sc_dt::sc_bit &|01", sc_dt::b_and(0, sc_dt::sc_bit(1)));
  emit("b_and|int,const sc_dt::sc_bit &|10", sc_dt::b_and(1, sc_dt::sc_bit(0)));
  emit("b_and|int,const sc_dt::sc_bit &|11", sc_dt::b_and(1, sc_dt::sc_bit(1)));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = 0;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = 1;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = 0;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = 1;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|11", output);
  }
  emit("b_and|const sc_dt::sc_bit &,bool|00", sc_dt::b_and(sc_dt::sc_bit(0), false));
  emit("b_and|const sc_dt::sc_bit &,bool|01", sc_dt::b_and(sc_dt::sc_bit(0), true));
  emit("b_and|const sc_dt::sc_bit &,bool|10", sc_dt::b_and(sc_dt::sc_bit(1), false));
  emit("b_and|const sc_dt::sc_bit &,bool|11", sc_dt::b_and(sc_dt::sc_bit(1), true));
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = false;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = true;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = false;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = true;
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|11", output);
  }
  emit("b_and|const sc_dt::sc_bit &,char|00", sc_dt::b_and(sc_dt::sc_bit(0), '0'));
  emit("b_and|const sc_dt::sc_bit &,char|01", sc_dt::b_and(sc_dt::sc_bit(0), '1'));
  emit("b_and|const sc_dt::sc_bit &,char|10", sc_dt::b_and(sc_dt::sc_bit(1), '0'));
  emit("b_and|const sc_dt::sc_bit &,char|11", sc_dt::b_and(sc_dt::sc_bit(1), '1'));
  emit("b_and|char,const sc_dt::sc_bit &|00", sc_dt::b_and('0', sc_dt::sc_bit(0)));
  emit("b_and|char,const sc_dt::sc_bit &|01", sc_dt::b_and('0', sc_dt::sc_bit(1)));
  emit("b_and|char,const sc_dt::sc_bit &|10", sc_dt::b_and('1', sc_dt::sc_bit(0)));
  emit("b_and|char,const sc_dt::sc_bit &|11", sc_dt::b_and('1', sc_dt::sc_bit(1)));
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '0';
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '0';
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '1';
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '1';
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_and(output, left, right);
    emit("b_and|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '0';
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '0';
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '1';
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '1';
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|11", output);
  }
  emit("b_or|const sc_dt::sc_bit &,bool|00", sc_dt::b_or(sc_dt::sc_bit(0), false));
  emit("b_or|const sc_dt::sc_bit &,bool|01", sc_dt::b_or(sc_dt::sc_bit(0), true));
  emit("b_or|const sc_dt::sc_bit &,bool|10", sc_dt::b_or(sc_dt::sc_bit(1), false));
  emit("b_or|const sc_dt::sc_bit &,bool|11", sc_dt::b_or(sc_dt::sc_bit(1), true));
  emit("b_or|const sc_dt::sc_bit &,char|00", sc_dt::b_or(sc_dt::sc_bit(0), '0'));
  emit("b_or|const sc_dt::sc_bit &,char|01", sc_dt::b_or(sc_dt::sc_bit(0), '1'));
  emit("b_or|const sc_dt::sc_bit &,char|10", sc_dt::b_or(sc_dt::sc_bit(1), '0'));
  emit("b_or|const sc_dt::sc_bit &,char|11", sc_dt::b_or(sc_dt::sc_bit(1), '1'));
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = '0';
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = '1';
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = '0';
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = '1';
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|11", output);
  }
  emit("b_or|int,const sc_dt::sc_bit &|00", sc_dt::b_or(0, sc_dt::sc_bit(0)));
  emit("b_or|int,const sc_dt::sc_bit &|01", sc_dt::b_or(0, sc_dt::sc_bit(1)));
  emit("b_or|int,const sc_dt::sc_bit &|10", sc_dt::b_or(1, sc_dt::sc_bit(0)));
  emit("b_or|int,const sc_dt::sc_bit &|11", sc_dt::b_or(1, sc_dt::sc_bit(1)));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = false;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = true;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = false;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = true;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|11", output);
  }
  emit("b_or|bool,const sc_dt::sc_bit &|00", sc_dt::b_or(false, sc_dt::sc_bit(0)));
  emit("b_or|bool,const sc_dt::sc_bit &|01", sc_dt::b_or(false, sc_dt::sc_bit(1)));
  emit("b_or|bool,const sc_dt::sc_bit &|10", sc_dt::b_or(true, sc_dt::sc_bit(0)));
  emit("b_or|bool,const sc_dt::sc_bit &|11", sc_dt::b_or(true, sc_dt::sc_bit(1)));
  emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", sc_dt::b_or(sc_dt::sc_bit(0), sc_dt::sc_bit(0)));
  emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", sc_dt::b_or(sc_dt::sc_bit(0), sc_dt::sc_bit(1)));
  emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", sc_dt::b_or(sc_dt::sc_bit(1), sc_dt::sc_bit(0)));
  emit("b_or|const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", sc_dt::b_or(sc_dt::sc_bit(1), sc_dt::sc_bit(1)));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = false;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = false;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = true;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = true;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|11", output);
  }
  emit("b_or|char,const sc_dt::sc_bit &|00", sc_dt::b_or('0', sc_dt::sc_bit(0)));
  emit("b_or|char,const sc_dt::sc_bit &|01", sc_dt::b_or('0', sc_dt::sc_bit(1)));
  emit("b_or|char,const sc_dt::sc_bit &|10", sc_dt::b_or('1', sc_dt::sc_bit(0)));
  emit("b_or|char,const sc_dt::sc_bit &|11", sc_dt::b_or('1', sc_dt::sc_bit(1)));
  emit("b_or|const sc_dt::sc_bit &,int|00", sc_dt::b_or(sc_dt::sc_bit(0), 0));
  emit("b_or|const sc_dt::sc_bit &,int|01", sc_dt::b_or(sc_dt::sc_bit(0), 1));
  emit("b_or|const sc_dt::sc_bit &,int|10", sc_dt::b_or(sc_dt::sc_bit(1), 0));
  emit("b_or|const sc_dt::sc_bit &,int|11", sc_dt::b_or(sc_dt::sc_bit(1), 1));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 0;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 0;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 1;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 1;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = 0;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = 1;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = 0;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = 1;
    sc_dt::b_or(output, left, right);
    emit("b_or|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = false;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = true;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = false;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = true;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,bool|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix &|11", output);
  }
  emit("b_xor|char,const sc_dt::sc_bit &|00", sc_dt::b_xor('0', sc_dt::sc_bit(0)));
  emit("b_xor|char,const sc_dt::sc_bit &|01", sc_dt::b_xor('0', sc_dt::sc_bit(1)));
  emit("b_xor|char,const sc_dt::sc_bit &|10", sc_dt::b_xor('1', sc_dt::sc_bit(0)));
  emit("b_xor|char,const sc_dt::sc_bit &|11", sc_dt::b_xor('1', sc_dt::sc_bit(1)));
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix_fast &|11", output);
  }
  emit("b_xor|int,const sc_dt::sc_bit &|00", sc_dt::b_xor(0, sc_dt::sc_bit(0)));
  emit("b_xor|int,const sc_dt::sc_bit &|01", sc_dt::b_xor(0, sc_dt::sc_bit(1)));
  emit("b_xor|int,const sc_dt::sc_bit &|10", sc_dt::b_xor(1, sc_dt::sc_bit(0)));
  emit("b_xor|int,const sc_dt::sc_bit &|11", sc_dt::b_xor(1, sc_dt::sc_bit(1)));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 0;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 0;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 1;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = 1;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,int,const sc_dt::sc_bit &|11", output);
  }
  emit("b_xor|const sc_dt::sc_bit &,bool|00", sc_dt::b_xor(sc_dt::sc_bit(0), false));
  emit("b_xor|const sc_dt::sc_bit &,bool|01", sc_dt::b_xor(sc_dt::sc_bit(0), true));
  emit("b_xor|const sc_dt::sc_bit &,bool|10", sc_dt::b_xor(sc_dt::sc_bit(1), false));
  emit("b_xor|const sc_dt::sc_bit &,bool|11", sc_dt::b_xor(sc_dt::sc_bit(1), true));
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &,const sc_dt::sc_ufix &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &,const sc_dt::sc_fix_fast &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix &|11", output);
  }
  emit("b_xor|const sc_dt::sc_bit &,int|00", sc_dt::b_xor(sc_dt::sc_bit(0), 0));
  emit("b_xor|const sc_dt::sc_bit &,int|01", sc_dt::b_xor(sc_dt::sc_bit(0), 1));
  emit("b_xor|const sc_dt::sc_bit &,int|10", sc_dt::b_xor(sc_dt::sc_bit(1), 0));
  emit("b_xor|const sc_dt::sc_bit &,int|11", sc_dt::b_xor(sc_dt::sc_bit(1), 1));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = 0;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = 1;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = 0;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = 1;
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,int|11", output);
  }
  emit("b_xor|bool,const sc_dt::sc_bit &|00", sc_dt::b_xor(false, sc_dt::sc_bit(0)));
  emit("b_xor|bool,const sc_dt::sc_bit &|01", sc_dt::b_xor(false, sc_dt::sc_bit(1)));
  emit("b_xor|bool,const sc_dt::sc_bit &|10", sc_dt::b_xor(true, sc_dt::sc_bit(0)));
  emit("b_xor|bool,const sc_dt::sc_bit &|11", sc_dt::b_xor(true, sc_dt::sc_bit(1)));
  emit("b_xor|const sc_dt::sc_bit &,char|00", sc_dt::b_xor(sc_dt::sc_bit(0), '0'));
  emit("b_xor|const sc_dt::sc_bit &,char|01", sc_dt::b_xor(sc_dt::sc_bit(0), '1'));
  emit("b_xor|const sc_dt::sc_bit &,char|10", sc_dt::b_xor(sc_dt::sc_bit(1), '0'));
  emit("b_xor|const sc_dt::sc_bit &,char|11", sc_dt::b_xor(sc_dt::sc_bit(1), '1'));
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '0';
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '0';
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '1';
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = '1';
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,char,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", output);
  }
  emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|00", sc_dt::b_xor(sc_dt::sc_bit(0), sc_dt::sc_bit(0)));
  emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|01", sc_dt::b_xor(sc_dt::sc_bit(0), sc_dt::sc_bit(1)));
  emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|10", sc_dt::b_xor(sc_dt::sc_bit(1), sc_dt::sc_bit(0)));
  emit("b_xor|const sc_dt::sc_bit &,const sc_dt::sc_bit &|11", sc_dt::b_xor(sc_dt::sc_bit(1), sc_dt::sc_bit(1)));
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(5.75);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_ufix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_ufix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_ufix &,const sc_dt::sc_ufix &,const sc_dt::sc_ufix_fast &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = false;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = false;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = true;
    const auto right = sc_dt::sc_bit(0);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = true;
    const auto right = sc_dt::sc_bit(1);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,bool,const sc_dt::sc_bit &|11", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|00", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(5.75);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|01", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(5.75);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|10", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto left = fixed_value<sc_dt::sc_fix>(-3.25);
    const auto right = fixed_value<sc_dt::sc_fix_fast>(-3.25);
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_fix &,const sc_dt::sc_fix &,const sc_dt::sc_fix_fast &|11", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = '0';
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|00", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(0);
    const auto right = '1';
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|01", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = '0';
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|10", output);
  }
  {
    auto output = sc_dt::sc_bit(1);
    const auto left = sc_dt::sc_bit(1);
    const auto right = '1';
    sc_dt::b_xor(output, left, right);
    emit("b_xor|sc_dt::sc_bit &,const sc_dt::sc_bit &,char|11", output);
  }
  {
    auto output = sc_dt::sc_fxval(7.5);
    const auto source = fixed_value<sc_dt::sc_fix>(1.25);
    sc_dt::neg(output, source);
    verify(output.to_double() == -1.25);
    emit("neg|sc_fxval|sc_fix|1.25", output);
  }
  {
    auto output = sc_dt::sc_fxval(7.5);
    const auto source = fixed_value<sc_dt::sc_fix>(-3.75);
    sc_dt::neg(output, source);
    verify(output.to_double() == 3.75);
    emit("neg|sc_fxval|sc_fix|-3.75", output);
  }
  {
    auto output = sc_dt::sc_fxval_fast(7.5);
    const auto source = fixed_value<sc_dt::sc_fix_fast>(1.25);
    sc_dt::neg(output, source);
    verify(output.to_double() == -1.25);
    emit("neg|sc_fxval_fast|sc_fix_fast|1.25", output);
  }
  {
    auto output = sc_dt::sc_fxval_fast(7.5);
    const auto source = fixed_value<sc_dt::sc_fix_fast>(-3.75);
    sc_dt::neg(output, source);
    verify(output.to_double() == 3.75);
    emit("neg|sc_fxval_fast|sc_fix_fast|-3.75", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(7.5);
    const auto source = fixed_value<sc_dt::sc_fix>(1.25);
    sc_dt::neg(output, source);
    verify(output.to_double() == -1.25);
    emit("neg|sc_fix|sc_fix|1.25", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix>(7.5);
    const auto source = fixed_value<sc_dt::sc_fix>(-3.75);
    sc_dt::neg(output, source);
    verify(output.to_double() == 3.75);
    emit("neg|sc_fix|sc_fix|-3.75", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(7.5);
    const auto source = fixed_value<sc_dt::sc_fix_fast>(1.25);
    sc_dt::neg(output, source);
    verify(output.to_double() == -1.25);
    emit("neg|sc_fix_fast|sc_fix_fast|1.25", output);
  }
  {
    auto output = fixed_value<sc_dt::sc_fix_fast>(7.5);
    const auto source = fixed_value<sc_dt::sc_fix_fast>(-3.75);
    sc_dt::neg(output, source);
    verify(output.to_double() == 3.75);
    emit("neg|sc_fix_fast|sc_fix_fast|-3.75", output);
  }
  {
    auto output = sc_dt::sc_fxval(7.5);
    const auto source = sc_dt::sc_fxval(1.25);
    sc_dt::neg(output, source);
    verify(output.to_double() == -1.25);
    emit("neg|sc_fxval|sc_fxval|1.25", output);
  }
  {
    auto output = sc_dt::sc_fxval(7.5);
    const auto source = sc_dt::sc_fxval(-3.75);
    sc_dt::neg(output, source);
    verify(output.to_double() == 3.75);
    emit("neg|sc_fxval|sc_fxval|-3.75", output);
  }
  {
    auto output = sc_dt::sc_fxval_fast(7.5);
    const auto source = sc_dt::sc_fxval_fast(1.25);
    sc_dt::neg(output, source);
    verify(output.to_double() == -1.25);
    emit("neg|sc_fxval_fast|sc_fxval_fast|1.25", output);
  }
  {
    auto output = sc_dt::sc_fxval_fast(7.5);
    const auto source = sc_dt::sc_fxval_fast(-3.75);
    sc_dt::neg(output, source);
    verify(output.to_double() == 3.75);
    emit("neg|sc_fxval_fast|sc_fxval_fast|-3.75", output);
  }
  emit("complement|sc_bit", ~sc_dt::sc_bit(false));
  {
    sc_dt::sc_signed source(191);
    source = "0x100000000000000000000000000000000000000000005a";
    auto result = ~source;
    verify(result.length() == 191);
    emit("complement|sc_signed", result);
  }
  {
    sc_dt::sc_unsigned source(191);
    source = "0x100000000000000000000000000000000000000000005a";
    auto result = ~source;
    verify(result.length() == 192);
    emit("complement|sc_unsigned", result);
  }
  {
    sc_dt::sc_lv_base left(
      "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ"
      "01XZ01XZ01XZ01XZ01XZ01XZ1");
    sc_dt::sc_lv_base right(
      "10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX"
      "10ZX10ZX10ZX10ZX10ZX10ZX0");
    auto& result = sc_dt::operator&=(static_cast<sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_lv_base|sc_lv_base|0", left);
  }
  {
    sc_dt::sc_lv_base left(
      "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ"
      "01XZ01XZ01XZ01XZ01XZ01XZ1");
    sc_dt::sc_lv_base right(
      "10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX"
      "10ZX10ZX10ZX10ZX10ZX10ZX0");
    auto& result = sc_dt::operator|=(static_cast<sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_lv_base|sc_lv_base|1", left);
  }
  {
    sc_dt::sc_lv_base left(
      "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ"
      "01XZ01XZ01XZ01XZ01XZ01XZ1");
    sc_dt::sc_lv_base right(
      "10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX10ZX"
      "10ZX10ZX10ZX10ZX10ZX10ZX0");
    auto& result = sc_dt::operator^=(static_cast<sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_lv_base|sc_lv_base|2", left);
  }
  {
    sc_dt::sc_lv_base left(
      "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ"
      "01XZ01XZ01XZ01XZ01XZ01XZ1");
    sc_dt::sc_bv_base right(
      "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010"
      "1010101010101010101010100");
    auto& result = sc_dt::operator&=(static_cast<sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_lv_base|sc_bv_base|0", left);
  }
  {
    sc_dt::sc_lv_base left(
      "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ"
      "01XZ01XZ01XZ01XZ01XZ01XZ1");
    sc_dt::sc_bv_base right(
      "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010"
      "1010101010101010101010100");
    auto& result = sc_dt::operator|=(static_cast<sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_lv_base|sc_bv_base|1", left);
  }
  {
    sc_dt::sc_lv_base left(
      "01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ01XZ"
      "01XZ01XZ01XZ01XZ01XZ01XZ1");
    sc_dt::sc_bv_base right(
      "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010"
      "1010101010101010101010100");
    auto& result = sc_dt::operator^=(static_cast<sc_dt::sc_proxy<sc_dt::sc_lv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_lv_base|sc_bv_base|2", left);
  }
  {
    sc_dt::sc_bv_base left(
      "0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"
      "0101010101010101010101011");
    sc_dt::sc_bv_base right(
      "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010"
      "1010101010101010101010100");
    auto& result = sc_dt::operator&=(static_cast<sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_bv_base|sc_bv_base|0", left);
  }
  {
    sc_dt::sc_bv_base left(
      "0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"
      "0101010101010101010101011");
    sc_dt::sc_bv_base right(
      "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010"
      "1010101010101010101010100");
    auto& result = sc_dt::operator|=(static_cast<sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_bv_base|sc_bv_base|1", left);
  }
  {
    sc_dt::sc_bv_base left(
      "0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"
      "0101010101010101010101011");
    sc_dt::sc_bv_base right(
      "1010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010"
      "1010101010101010101010100");
    auto& result = sc_dt::operator^=(static_cast<sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(left),
                                     static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(right));
    verify(&result == &left);
    emit("compound|sc_bv_base|sc_bv_base|2", left);
  }
  std::cout << "SC_DT_FREE_OPERATIONS_PUBLIC_OK\n";
  return 0;
}
