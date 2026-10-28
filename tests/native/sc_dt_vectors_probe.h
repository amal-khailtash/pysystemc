/** @file sc_dt_vectors_probe.h
 * @brief Exact native fixtures and protected-state probes for vector owners.
 * @details Fixture arrays and numeric values have static native lifetime and
 *          contain no Python references. Returned vectors are independent
 *          owning values. The protected-state helpers copy owners before
 *          inspecting their allocation fields. Calls hold the GIL, do not
 *          enter the scheduler, and propagate native SystemC reports.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_VECTORS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_VECTORS_PROBE_H

#include <array>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_dt_vectors_probe
{

static_assert(std::is_destructible<sc_dt::sc_bv_base>::value, "BV base owns destructible storage");
static_assert(std::is_destructible<sc_dt::sc_lv_base>::value, "LV base owns destructible data/control storage");
static_assert(std::is_destructible<sc_dt::sc_bv<37>>::value, "parameterized BV owns base storage");
static_assert(std::is_destructible<sc_dt::sc_lv<38>>::value, "parameterized LV owns base storage");
static_assert(std::is_same<sc_dt::sc_bv_base::value_type, bool>::value, "BV base exposes binary values");
static_assert(std::is_same<sc_dt::sc_lv_base::value_type, sc_dt::sc_logic_value_t>::value,
              "LV base exposes four-state values");

/** @brief Return static bool input bits for 257-bit owner constructors/assignments.
 * @return Borrowed pointer to stable static storage.
 */
inline const bool* bool_bits()
{
  static const bool bits[257] = {true, false, true, true, false, true, false, false, true, true};
  return bits;
}

/** @brief Return static two-state logic input bits for vector owner overloads.
 * @return Borrowed pointer to stable static storage.
 */
inline const sc_dt::sc_logic* logic_bits()
{
  static const std::array<sc_dt::sc_logic, 257> bits = []
  {
    std::array<sc_dt::sc_logic, 257> value;
    value.fill(sc_dt::SC_LOGIC_0);
    value[0] = sc_dt::SC_LOGIC_1;
    value[1] = sc_dt::SC_LOGIC_0;
    value[2] = sc_dt::SC_LOGIC_1;
    value[3] = sc_dt::SC_LOGIC_1;
    value[4] = sc_dt::SC_LOGIC_0;
    value[5] = sc_dt::SC_LOGIC_1;
    value[6] = sc_dt::SC_LOGIC_0;
    value[7] = sc_dt::SC_LOGIC_0;
    value[8] = sc_dt::SC_LOGIC_1;
    value[9] = sc_dt::SC_LOGIC_1;
    return value;
  }();
  return bits.data();
}

/** @brief Return a static native arbitrary unsigned value for constructors.
 * @return Borrowed 32-bit sc_unsigned fixture.
 */
inline const sc_dt::sc_unsigned& unsigned_value()
{
  static const sc_dt::sc_unsigned value = []
  {
    sc_dt::sc_unsigned result(32);
    result = "10100101101001011100110000111100";
    return result;
  }();
  return value;
}

/** @brief Return a static native arbitrary signed value for constructors.
 * @return Borrowed 32-bit sc_signed fixture.
 */
inline const sc_dt::sc_signed& signed_value()
{
  static const sc_dt::sc_signed value = []
  {
    sc_dt::sc_signed result(32);
    result = "10100101101001011100110000111100";
    return result;
  }();
  return value;
}

/** @brief Return a static fixed-width unsigned value for constructor tests.
 * @return Borrowed 32-bit sc_uint fixture.
 */
inline const sc_dt::sc_uint_base& uint_value()
{
  static const sc_dt::sc_uint<32> value("10100101101001011100110000111100");
  return value;
}

/** @brief Return a static fixed-width signed value for constructor tests.
 * @return Borrowed 32-bit sc_int fixture.
 */
inline const sc_dt::sc_int_base& int_value()
{
  static const sc_dt::sc_int<32> value(static_cast<sc_dt::int64>(0xA5A5CC3C));
  return value;
}

/** @brief Return a static BV proxy source for generic vector constructor tests.
 * @return Borrowed 257-bit bit vector.
 */
inline const sc_dt::sc_bv<257>& bv_proxy()
{
  static const sc_dt::sc_bv<257> value("10100101101001011100110000111100");
  return value;
}

/** @brief Return a static LV proxy source containing all four logic states.
 * @return Borrowed 257-bit logic vector.
 */
inline const sc_dt::sc_lv<257>& lv_proxy()
{
  static const sc_dt::sc_lv<257> value("01ZX01ZX01ZX01ZX");
  return value;
}

/** @brief Copy a BV base and inspect its protected length/allocation metadata.
 * @param source Live owner to copy; it remains borrowed for the call.
 * @return Length, allocated word count, and whether data storage exists.
 */
inline std::string bv_storage_state(const sc_dt::sc_bv_base& source)
{
  struct state_view : sc_dt::sc_bv_base
  {
    explicit state_view(const sc_dt::sc_bv_base& value) : sc_dt::sc_bv_base(value)
    {
    }
    std::string state() const
    {
      std::ostringstream output;
      output << m_len << '\t' << m_size << '\t' << int(m_data != nullptr);
      return output.str();
    }
  } copy(source);
  return copy.state();
}

/** @brief Copy an LV base and inspect protected data/control allocations.
 * @param source Live owner to copy; it remains borrowed for the call.
 * @return Length, allocated word count, and whether both arrays exist.
 */
inline std::string lv_storage_state(const sc_dt::sc_lv_base& source)
{
  struct state_view : sc_dt::sc_lv_base
  {
    explicit state_view(const sc_dt::sc_lv_base& value) : sc_dt::sc_lv_base(value)
    {
    }
    std::string state() const
    {
      std::ostringstream output;
      output << m_len << '\t' << m_size << '\t' << int(m_data != nullptr && m_ctrl != nullptr);
      return output.str();
    }
  } copy(source);
  return copy.state();
}

/** @brief Render any native vector owner without changing or retaining it.
 * @tparam Vector BV or LV owner type.
 * @param value Live owner.
 * @return Copied SystemC bit-string.
 */
template <typename Vector>
inline std::string render(const Vector& value)
{
  std::ostringstream output;
  output << value;
  return output.str();
}

}  // namespace sc_dt_vectors_probe

#endif  // PYSYSTEMC_TEST_SC_DT_VECTORS_PROBE_H
