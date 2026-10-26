/** @file sc_dt_concat_matrix_probe.h
 * @brief Exact native calls for all unique concat and comma inventory signatures.
 * @details Function pointers select declarations whose parameters match each
 *          inventory signature. Eight stale LV result declarations are mapped
 *          to the linked BV definitions documented in the signature audit.
 *          Vector proxies borrow the owners declared by the matrix function;
 *          all records are copied before the owners leave scope. No callbacks,
 *          Python references, GIL changes, or kernel state are involved.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_CONCAT_MATRIX_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_CONCAT_MATRIX_PROBE_H

#include <iostream>
#include <systemc>

#include "sc_dt_concat_functions_probe.h"

namespace sc_dt_concat_functions_probe
{

/** @brief Emit one machine-readable matrix record. */
template <typename... Values>
void emit_matrix(const char *label, const Values &...values)
{
  std::cout << "SCF\t" << label;
  ((std::cout << '\t' << values), ...);
  std::cout << '\n';
}

/** @brief Call every distinct inventoried signature in both function families.
 * @details All native vector/proxy parents remain alive until their borrowed
 *          results have been printed. Eight documented stale LV declarations
 *          use exact BV function pointers because only those definitions link.
 */
inline void run_signature_matrix()
{
  using namespace sc_dt;
  // Full signature calls are generated from the frozen inventory signatures.
  using BV = sc_dt::sc_bv_base;
  using LV = sc_dt::sc_lv_base;
  BV bv(8);
  bv = "10100101";
  LV lv(8);
  lv = "10ZX0101";
  const BV &cbv = bv;
  const LV &clv = lv;
  auto bit_bv = bv.bit(1);
  auto bit_lv = lv.bit(1);
  auto bitr_bv = sc_dt::sc_bitref_r<BV>(cbv, 1);
  auto bitr_lv = sc_dt::sc_bitref_r<LV>(clv, 1);
  auto range_bv = bv.range(3, 0);
  auto range_lv = lv.range(3, 0);
  auto ranger_bv = cbv.range(3, 0);
  auto ranger_lv = clv.range(3, 0);
  auto pair_bb = sc_dt::concat(bv, bv);
  auto pairr_bb = concat_readonly(cbv, cbv);
  auto pair_bl = sc_dt::concat(bv, lv);
  auto pairr_bl = concat_readonly(cbv, clv);
  auto pair_lb = sc_dt::concat(lv, bv);
  auto pairr_lb = concat_readonly(clv, cbv);
  auto pair_ll = sc_dt::concat(lv, lv);
  auto pairr_ll = concat_readonly(clv, clv);
  sc_dt::sc_logic logic(sc_dt::SC_LOGIC_Z);
  sc_dt::sc_signed signed_value(8);
  signed_value = 5;
  sc_dt::sc_unsigned unsigned_value(8);
  unsigned_value = 9;
  {
    using Fn_concat_000 = const sc_dt::sc_concatref &(*)(bool, const sc_dt::sc_value_base &);
    const auto function = static_cast<Fn_concat_000>(&sc_dt::concat);
    decltype(auto) result = function(true, signed_value);
    emit_matrix("concat_000", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_concat_001 = const sc_dt::sc_concatref &(*)(const sc_dt::sc_value_base &, bool);
    const auto function = static_cast<Fn_concat_001>(&sc_dt::concat);
    decltype(auto) result = function(signed_value, true);
    emit_matrix("concat_001", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_concat_002 = const sc_dt::sc_concatref &(*)(const sc_dt::sc_value_base &, const sc_dt::sc_value_base &);
    const auto function = static_cast<Fn_concat_002>(&sc_dt::concat);
    decltype(auto) result = function(signed_value, signed_value);
    emit_matrix("concat_002", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_concat_003 = sc_concref<BV, LV> (*)(sc_proxy<BV> &, sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_003>(&sc_dt::concat);
    decltype(auto) result = function(bv, lv);
    emit_matrix("concat_003", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_004 = sc_concref<BV, sc_bitref<LV>> (*)(sc_proxy<BV> &, sc_bitref<LV>);
    const auto function = static_cast<Fn_concat_004>(&sc_dt::concat);
    decltype(auto) result = function(bv, bit_lv);
    emit_matrix("concat_004", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_005 = sc_concref<BV, sc_concref<LV, BV>> (*)(sc_proxy<BV> &, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_concat_005>(&sc_dt::concat);
    decltype(auto) result = function(bv, pair_lb);
    emit_matrix("concat_005", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_006 = sc_concref<BV, sc_subref<LV>> (*)(sc_proxy<BV> &, sc_subref<LV>);
    const auto function = static_cast<Fn_concat_006>(&sc_dt::concat);
    decltype(auto) result = function(bv, range_lv);
    emit_matrix("concat_006", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_007 = sc_concref<sc_bitref<BV>, LV> (*)(sc_bitref<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_007>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, lv);
    emit_matrix("concat_007", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_008 = sc_concref<sc_bitref<BV>, sc_bitref<LV>> (*)(sc_bitref<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_concat_008>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, bit_lv);
    emit_matrix("concat_008", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_009 = sc_concref<sc_bitref<BV>, sc_concref<LV, BV>> (*)(sc_bitref<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_concat_009>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, pair_lb);
    emit_matrix("concat_009", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_010 = sc_concref<sc_bitref<BV>, sc_subref<LV>> (*)(sc_bitref<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_concat_010>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, range_lv);
    emit_matrix("concat_010", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_011 = sc_concref<sc_concref<BV, LV>, BV> (*)(sc_concref<BV, LV>, sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_011>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, bv);
    emit_matrix("concat_011", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_012 = sc_concref<sc_concref<BV, LV>, sc_bitref<BV>> (*)(sc_concref<BV, LV>, sc_bitref<BV>);
    const auto function = static_cast<Fn_concat_012>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, bit_bv);
    emit_matrix("concat_012", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_013 =
      sc_concref<sc_concref<BV, LV>, sc_concref<BV, LV>> (*)(sc_concref<BV, LV>, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_concat_013>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, pair_bl);
    emit_matrix("concat_013", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_014 = sc_concref<sc_concref<BV, LV>, sc_subref<BV>> (*)(sc_concref<BV, LV>, sc_subref<BV>);
    const auto function = static_cast<Fn_concat_014>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, range_bv);
    emit_matrix("concat_014", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_015 = sc_concref<sc_subref<BV>, LV> (*)(sc_subref<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_015>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, lv);
    emit_matrix("concat_015", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_016 = sc_concref<sc_subref<BV>, sc_bitref<LV>> (*)(sc_subref<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_concat_016>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, bit_lv);
    emit_matrix("concat_016", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_017 = sc_concref<sc_subref<BV>, sc_concref<LV, BV>> (*)(sc_subref<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_concat_017>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, pair_lb);
    emit_matrix("concat_017", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_018 = sc_concref<sc_subref<BV>, sc_subref<LV>> (*)(sc_subref<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_concat_018>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, range_lv);
    emit_matrix("concat_018", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_019 = sc_concref_r<BV, BV> (*)(const sc_proxy<BV> &, bool);
    const auto function = static_cast<Fn_concat_019>(&sc_dt::concat);
    decltype(auto) result = function(bv, true);
    emit_matrix("concat_019", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_020 = sc_concref_r<BV, BV> (*)(sc_proxy<BV> &, bool);
    const auto function = static_cast<Fn_concat_020>(&sc_dt::concat);
    decltype(auto) result = function(bv, true);
    emit_matrix("concat_020", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_021 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, const char *);
    const auto function = static_cast<Fn_concat_021>(&sc_dt::concat);
    decltype(auto) result = function(bv, "10XZ01");
    emit_matrix("concat_021", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_022 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_022>(&sc_dt::concat);
    decltype(auto) result = function(bv, logic);
    emit_matrix("concat_022", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_023 = sc_concref_r<BV, LV> (*)(sc_proxy<BV> &, const char *);
    const auto function = static_cast<Fn_concat_023>(&sc_dt::concat);
    decltype(auto) result = function(bv, "10XZ01");
    emit_matrix("concat_023", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_024 = sc_concref_r<BV, LV> (*)(sc_proxy<BV> &, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_024>(&sc_dt::concat);
    decltype(auto) result = function(bv, logic);
    emit_matrix("concat_024", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_025 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_025>(&sc_dt::concat);
    decltype(auto) result = function(bv, lv);
    emit_matrix("concat_025", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_026 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_026>(&sc_dt::concat);
    decltype(auto) result = function(bv, lv);
    emit_matrix("concat_026", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_027 = sc_concref_r<BV, LV> (*)(sc_proxy<BV> &, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_027>(&sc_dt::concat);
    decltype(auto) result = function(bv, lv);
    emit_matrix("concat_027", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_028 = sc_concref_r<BV, sc_bitref_r<LV>> (*)(const sc_proxy<BV> &, sc_bitref<LV>);
    const auto function = static_cast<Fn_concat_028>(&sc_dt::concat);
    decltype(auto) result = function(bv, bit_lv);
    emit_matrix("concat_028", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_029 = sc_concref_r<BV, sc_bitref_r<LV>> (*)(const sc_proxy<BV> &, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_concat_029>(&sc_dt::concat);
    decltype(auto) result = function(bv, bitr_lv);
    emit_matrix("concat_029", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_030 = sc_concref_r<BV, sc_bitref_r<LV>> (*)(sc_proxy<BV> &, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_concat_030>(&sc_dt::concat);
    decltype(auto) result = function(bv, bitr_lv);
    emit_matrix("concat_030", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_031 = sc_concref_r<BV, sc_concref_r<LV, BV>> (*)(const sc_proxy<BV> &, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_concat_031>(&sc_dt::concat);
    decltype(auto) result = function(bv, pair_lb);
    emit_matrix("concat_031", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_032 = sc_concref_r<BV, sc_concref_r<LV, BV>> (*)(const sc_proxy<BV> &, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_concat_032>(&sc_dt::concat);
    decltype(auto) result = function(bv, pairr_lb);
    emit_matrix("concat_032", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_033 = sc_concref_r<BV, sc_concref_r<LV, BV>> (*)(sc_proxy<BV> &, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_concat_033>(&sc_dt::concat);
    decltype(auto) result = function(bv, pairr_lb);
    emit_matrix("concat_033", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_034 = sc_concref_r<BV, sc_subref_r<LV>> (*)(const sc_proxy<BV> &, sc_subref<LV>);
    const auto function = static_cast<Fn_concat_034>(&sc_dt::concat);
    decltype(auto) result = function(bv, range_lv);
    emit_matrix("concat_034", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_035 = sc_concref_r<BV, sc_subref_r<LV>> (*)(const sc_proxy<BV> &, sc_subref_r<LV>);
    const auto function = static_cast<Fn_concat_035>(&sc_dt::concat);
    decltype(auto) result = function(bv, ranger_lv);
    emit_matrix("concat_035", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_036 = sc_concref_r<BV, sc_subref_r<LV>> (*)(sc_proxy<BV> &, sc_subref_r<LV>);
    const auto function = static_cast<Fn_concat_036>(&sc_dt::concat);
    decltype(auto) result = function(bv, ranger_lv);
    emit_matrix("concat_036", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_037 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_037>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, lv);
    emit_matrix("concat_037", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_038 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_038>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, lv);
    emit_matrix("concat_038", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_039 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_039>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, lv);
    emit_matrix("concat_039", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_040 = sc_concref_r<sc_bitref_r<BV>, sc_bitref_r<LV>> (*)(sc_bitref<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_concat_040>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, bitr_lv);
    emit_matrix("concat_040", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_041 = sc_concref_r<sc_bitref_r<BV>, sc_bitref_r<LV>> (*)(sc_bitref_r<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_concat_041>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, bit_lv);
    emit_matrix("concat_041", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_042 = sc_concref_r<sc_bitref_r<BV>, sc_bitref_r<LV>> (*)(sc_bitref_r<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_concat_042>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, bitr_lv);
    emit_matrix("concat_042", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_043 = sc_concref_r<sc_bitref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_bitref<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_concat_043>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, pairr_lb);
    emit_matrix("concat_043", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_044 = sc_concref_r<sc_bitref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_bitref_r<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_concat_044>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, pair_lb);
    emit_matrix("concat_044", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_045 =
      sc_concref_r<sc_bitref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_bitref_r<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_concat_045>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, pairr_lb);
    emit_matrix("concat_045", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_046 = sc_concref_r<sc_bitref_r<BV>, sc_subref_r<LV>> (*)(sc_bitref<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_concat_046>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, ranger_lv);
    emit_matrix("concat_046", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_047 = sc_concref_r<sc_bitref_r<BV>, sc_subref_r<LV>> (*)(sc_bitref_r<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_concat_047>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, range_lv);
    emit_matrix("concat_047", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_048 = sc_concref_r<sc_bitref_r<BV>, sc_subref_r<LV>> (*)(sc_bitref_r<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_concat_048>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, ranger_lv);
    emit_matrix("concat_048", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_049 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref<BV>, bool);
    const auto function = static_cast<Fn_concat_049>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, true);
    emit_matrix("concat_049", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_050 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref_r<BV>, bool);
    const auto function = static_cast<Fn_concat_050>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, true);
    emit_matrix("concat_050", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_051 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref<BV>, bool);
    const auto function = static_cast<Fn_concat_051>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, true);
    emit_matrix("concat_051", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_052 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref<BV>, const char *);
    const auto function = static_cast<Fn_concat_052>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, "10XZ01");
    emit_matrix("concat_052", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_053 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_053>(&sc_dt::concat);
    decltype(auto) result = function(bit_bv, logic);
    emit_matrix("concat_053", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_054 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref_r<BV>, bool);
    const auto function = static_cast<Fn_concat_054>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, true);
    emit_matrix("concat_054", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_055 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, const char *);
    const auto function = static_cast<Fn_concat_055>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, "10XZ01");
    emit_matrix("concat_055", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_056 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_056>(&sc_dt::concat);
    decltype(auto) result = function(bitr_bv, logic);
    emit_matrix("concat_056", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_057 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref<BV, LV>, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_057>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, bv);
    emit_matrix("concat_057", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_058 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref_r<BV, LV>, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_058>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, bv);
    emit_matrix("concat_058", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_059 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref_r<BV, LV>, sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_059>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, bv);
    emit_matrix("concat_059", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_060 = sc_concref_r<sc_concref_r<BV, LV>, sc_bitref_r<BV>> (*)(sc_concref<BV, LV>, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_concat_060>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, bitr_bv);
    emit_matrix("concat_060", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_061 = sc_concref_r<sc_concref_r<BV, LV>, sc_bitref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_bitref<BV>);
    const auto function = static_cast<Fn_concat_061>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, bit_bv);
    emit_matrix("concat_061", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_062 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_bitref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_concat_062>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, bitr_bv);
    emit_matrix("concat_062", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_063 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_concref_r<BV, LV>> (*)(sc_concref<BV, LV>, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_concat_063>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, pairr_bl);
    emit_matrix("concat_063", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_064 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_concref_r<BV, LV>> (*)(sc_concref_r<BV, LV>, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_concat_064>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, pair_bl);
    emit_matrix("concat_064", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_065 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_concref_r<BV, LV>> (*)(sc_concref_r<BV, LV>, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_concat_065>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, pairr_bl);
    emit_matrix("concat_065", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_066 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref<BV, LV>, bool);
    const auto function = static_cast<Fn_concat_066>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, true);
    emit_matrix("concat_066", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_067 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref_r<BV, LV>, bool);
    const auto function = static_cast<Fn_concat_067>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, true);
    emit_matrix("concat_067", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_068 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref<BV, LV>, const char *);
    const auto function = static_cast<Fn_concat_068>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, "10XZ01");
    emit_matrix("concat_068", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_069 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref<BV, LV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_069>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, logic);
    emit_matrix("concat_069", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_070 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref_r<BV, LV>, const char *);
    const auto function = static_cast<Fn_concat_070>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, "10XZ01");
    emit_matrix("concat_070", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_071 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref_r<BV, LV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_071>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, logic);
    emit_matrix("concat_071", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_072 = sc_concref_r<sc_concref_r<BV, LV>, sc_subref_r<BV>> (*)(sc_concref<BV, LV>, sc_subref_r<BV>);
    const auto function = static_cast<Fn_concat_072>(&sc_dt::concat);
    decltype(auto) result = function(pair_bl, ranger_bv);
    emit_matrix("concat_072", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_073 = sc_concref_r<sc_concref_r<BV, LV>, sc_subref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_subref<BV>);
    const auto function = static_cast<Fn_concat_073>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, range_bv);
    emit_matrix("concat_073", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_074 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_subref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_subref_r<BV>);
    const auto function = static_cast<Fn_concat_074>(&sc_dt::concat);
    decltype(auto) result = function(pairr_bl, ranger_bv);
    emit_matrix("concat_074", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_075 = sc_concref_r<BV, BV> (*)(bool, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_075>(&sc_dt::concat);
    decltype(auto) result = function(true, bv);
    emit_matrix("concat_075", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_076 = sc_concref_r<BV, BV> (*)(bool, sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_076>(&sc_dt::concat);
    decltype(auto) result = function(true, bv);
    emit_matrix("concat_076", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_077 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref<BV>);
    const auto function = static_cast<Fn_concat_077>(&sc_dt::concat);
    decltype(auto) result = function(true, bit_bv);
    emit_matrix("concat_077", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_078 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_concat_078>(&sc_dt::concat);
    decltype(auto) result = function(true, bitr_bv);
    emit_matrix("concat_078", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_079 = sc_concref_r<BV, sc_concref_r<BV, LV>> (*)(bool, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_concat_079>(&sc_dt::concat);
    decltype(auto) result = function(true, pair_bl);
    emit_matrix("concat_079", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_080 = sc_concref_r<BV, sc_concref_r<BV, LV>> (*)(bool, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_concat_080>(&sc_dt::concat);
    decltype(auto) result = function(true, pairr_bl);
    emit_matrix("concat_080", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_081 = sc_concref_r<BV, sc_subref_r<BV>> (*)(bool, sc_subref<BV>);
    const auto function = static_cast<Fn_concat_081>(&sc_dt::concat);
    decltype(auto) result = function(true, range_bv);
    emit_matrix("concat_081", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_082 = sc_concref_r<BV, sc_subref_r<BV>> (*)(bool, sc_subref_r<BV>);
    const auto function = static_cast<Fn_concat_082>(&sc_dt::concat);
    decltype(auto) result = function(true, ranger_bv);
    emit_matrix("concat_082", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_083 = sc_concref_r<LV, BV> (*)(const char *, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_083>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", bv);
    emit_matrix("concat_083", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_084 = sc_concref_r<LV, BV> (*)(const char *, sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_084>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", bv);
    emit_matrix("concat_084", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_085 = sc_concref_r<LV, BV> (*)(const sc_dt::sc_logic &, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_085>(&sc_dt::concat);
    decltype(auto) result = function(logic, bv);
    emit_matrix("concat_085", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_086 = sc_concref_r<LV, BV> (*)(const sc_dt::sc_logic &, sc_proxy<BV> &);
    const auto function = static_cast<Fn_concat_086>(&sc_dt::concat);
    decltype(auto) result = function(logic, bv);
    emit_matrix("concat_086", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_087 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref<BV>);
    const auto function = static_cast<Fn_concat_087>(&sc_dt::concat);
    decltype(auto) result = function(true, bit_bv);
    emit_matrix("concat_087", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_088 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_concat_088>(&sc_dt::concat);
    decltype(auto) result = function(true, bitr_bv);
    emit_matrix("concat_088", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_089 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const char *, sc_bitref<BV>);
    const auto function = static_cast<Fn_concat_089>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", bit_bv);
    emit_matrix("concat_089", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_090 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const char *, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_concat_090>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", bitr_bv);
    emit_matrix("concat_090", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_091 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const sc_dt::sc_logic &, sc_bitref<BV>);
    const auto function = static_cast<Fn_concat_091>(&sc_dt::concat);
    decltype(auto) result = function(logic, bit_bv);
    emit_matrix("concat_091", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_092 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const sc_dt::sc_logic &, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_concat_092>(&sc_dt::concat);
    decltype(auto) result = function(logic, bitr_bv);
    emit_matrix("concat_092", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_093 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const char *, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_concat_093>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", pair_bl);
    emit_matrix("concat_093", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_094 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const char *, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_concat_094>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", pairr_bl);
    emit_matrix("concat_094", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_095 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const sc_dt::sc_logic &, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_concat_095>(&sc_dt::concat);
    decltype(auto) result = function(logic, pair_bl);
    emit_matrix("concat_095", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_096 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const sc_dt::sc_logic &, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_concat_096>(&sc_dt::concat);
    decltype(auto) result = function(logic, pairr_bl);
    emit_matrix("concat_096", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_097 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const char *, sc_subref<BV>);
    const auto function = static_cast<Fn_concat_097>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", range_bv);
    emit_matrix("concat_097", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_098 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const char *, sc_subref_r<BV>);
    const auto function = static_cast<Fn_concat_098>(&sc_dt::concat);
    decltype(auto) result = function("10XZ01", ranger_bv);
    emit_matrix("concat_098", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_099 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const sc_dt::sc_logic &, sc_subref<BV>);
    const auto function = static_cast<Fn_concat_099>(&sc_dt::concat);
    decltype(auto) result = function(logic, range_bv);
    emit_matrix("concat_099", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_100 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const sc_dt::sc_logic &, sc_subref_r<BV>);
    const auto function = static_cast<Fn_concat_100>(&sc_dt::concat);
    decltype(auto) result = function(logic, ranger_bv);
    emit_matrix("concat_100", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_101 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_101>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, lv);
    emit_matrix("concat_101", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_102 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_102>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, lv);
    emit_matrix("concat_102", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_103 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_concat_103>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, lv);
    emit_matrix("concat_103", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_104 = sc_concref_r<sc_subref_r<BV>, sc_bitref_r<LV>> (*)(sc_subref<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_concat_104>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, bitr_lv);
    emit_matrix("concat_104", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_105 = sc_concref_r<sc_subref_r<BV>, sc_bitref_r<LV>> (*)(sc_subref_r<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_concat_105>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, bit_lv);
    emit_matrix("concat_105", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_106 = sc_concref_r<sc_subref_r<BV>, sc_bitref_r<LV>> (*)(sc_subref_r<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_concat_106>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, bitr_lv);
    emit_matrix("concat_106", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_107 = sc_concref_r<sc_subref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_subref<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_concat_107>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, pairr_lb);
    emit_matrix("concat_107", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_108 = sc_concref_r<sc_subref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_subref_r<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_concat_108>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, pair_lb);
    emit_matrix("concat_108", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_109 =
      sc_concref_r<sc_subref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_subref_r<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_concat_109>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, pairr_lb);
    emit_matrix("concat_109", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_110 = sc_concref_r<sc_subref_r<BV>, sc_subref_r<LV>> (*)(sc_subref<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_concat_110>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, ranger_lv);
    emit_matrix("concat_110", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_111 = sc_concref_r<sc_subref_r<BV>, sc_subref_r<LV>> (*)(sc_subref_r<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_concat_111>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, range_lv);
    emit_matrix("concat_111", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_112 = sc_concref_r<sc_subref_r<BV>, sc_subref_r<LV>> (*)(sc_subref_r<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_concat_112>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, ranger_lv);
    emit_matrix("concat_112", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_113 = sc_concref_r<sc_subref_r<BV>, BV> (*)(sc_subref<BV>, bool);
    const auto function = static_cast<Fn_concat_113>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, true);
    emit_matrix("concat_113", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_114 = sc_concref_r<sc_subref_r<BV>, BV> (*)(sc_subref_r<BV>, bool);
    const auto function = static_cast<Fn_concat_114>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, true);
    emit_matrix("concat_114", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_115 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref<BV>, const char *);
    const auto function = static_cast<Fn_concat_115>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, "10XZ01");
    emit_matrix("concat_115", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_116 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_116>(&sc_dt::concat);
    decltype(auto) result = function(range_bv, logic);
    emit_matrix("concat_116", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_117 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, const char *);
    const auto function = static_cast<Fn_concat_117>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, "10XZ01");
    emit_matrix("concat_117", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_118 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_concat_118>(&sc_dt::concat);
    decltype(auto) result = function(ranger_bv, logic);
    emit_matrix("concat_118", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_concat_119 = sc_dt::sc_concatref &(*)(sc_dt::sc_value_base &, sc_dt::sc_value_base &);
    const auto function = static_cast<Fn_concat_119>(&sc_dt::concat);
    decltype(auto) result = function(signed_value, signed_value);
    emit_matrix("concat_119", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_comma_000 = const sc_dt::sc_concatref &(*)(bool, const sc_dt::sc_value_base &);
    const auto function = static_cast<Fn_comma_000>(&sc_dt::operator, );
    decltype(auto) result = function(true, signed_value);
    emit_matrix("comma_000", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_comma_001 = const sc_dt::sc_concatref &(*)(const sc_dt::sc_value_base &, bool);
    const auto function = static_cast<Fn_comma_001>(&sc_dt::operator, );
    decltype(auto) result = function(signed_value, true);
    emit_matrix("comma_001", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_comma_002 = const sc_dt::sc_concatref &(*)(const sc_dt::sc_value_base &, const sc_dt::sc_value_base &);
    const auto function = static_cast<Fn_comma_002>(&sc_dt::operator, );
    decltype(auto) result = function(signed_value, signed_value);
    emit_matrix("comma_002", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    using Fn_comma_003 = sc_concref<BV, LV> (*)(sc_proxy<BV> &, sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_003>(&sc_dt::operator, );
    decltype(auto) result = function(bv, lv);
    emit_matrix("comma_003", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_004 = sc_concref<BV, sc_bitref<LV>> (*)(sc_proxy<BV> &, sc_bitref<LV>);
    const auto function = static_cast<Fn_comma_004>(&sc_dt::operator, );
    decltype(auto) result = function(bv, bit_lv);
    emit_matrix("comma_004", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_005 = sc_concref<BV, sc_concref<LV, BV>> (*)(sc_proxy<BV> &, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_comma_005>(&sc_dt::operator, );
    decltype(auto) result = function(bv, pair_lb);
    emit_matrix("comma_005", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_006 = sc_concref<BV, sc_subref<LV>> (*)(sc_proxy<BV> &, sc_subref<LV>);
    const auto function = static_cast<Fn_comma_006>(&sc_dt::operator, );
    decltype(auto) result = function(bv, range_lv);
    emit_matrix("comma_006", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_007 = sc_concref<sc_bitref<BV>, LV> (*)(sc_bitref<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_007>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, lv);
    emit_matrix("comma_007", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_008 = sc_concref<sc_bitref<BV>, sc_bitref<LV>> (*)(sc_bitref<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_comma_008>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, bit_lv);
    emit_matrix("comma_008", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_009 = sc_concref<sc_bitref<BV>, sc_concref<LV, BV>> (*)(sc_bitref<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_comma_009>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, pair_lb);
    emit_matrix("comma_009", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_010 = sc_concref<sc_bitref<BV>, sc_subref<LV>> (*)(sc_bitref<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_comma_010>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, range_lv);
    emit_matrix("comma_010", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_011 = sc_concref<sc_concref<BV, LV>, BV> (*)(sc_concref<BV, LV>, sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_011>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, bv);
    emit_matrix("comma_011", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_012 = sc_concref<sc_concref<BV, LV>, sc_bitref<BV>> (*)(sc_concref<BV, LV>, sc_bitref<BV>);
    const auto function = static_cast<Fn_comma_012>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, bit_bv);
    emit_matrix("comma_012", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_013 = sc_concref<sc_concref<BV, LV>, sc_concref<BV, LV>> (*)(sc_concref<BV, LV>, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_comma_013>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, pair_bl);
    emit_matrix("comma_013", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_014 = sc_concref<sc_concref<BV, LV>, sc_subref<BV>> (*)(sc_concref<BV, LV>, sc_subref<BV>);
    const auto function = static_cast<Fn_comma_014>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, range_bv);
    emit_matrix("comma_014", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_015 = sc_concref<sc_subref<BV>, LV> (*)(sc_subref<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_015>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, lv);
    emit_matrix("comma_015", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_016 = sc_concref<sc_subref<BV>, sc_bitref<LV>> (*)(sc_subref<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_comma_016>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, bit_lv);
    emit_matrix("comma_016", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_017 = sc_concref<sc_subref<BV>, sc_concref<LV, BV>> (*)(sc_subref<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_comma_017>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, pair_lb);
    emit_matrix("comma_017", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_018 = sc_concref<sc_subref<BV>, sc_subref<LV>> (*)(sc_subref<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_comma_018>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, range_lv);
    emit_matrix("comma_018", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_019 = sc_concref_r<BV, BV> (*)(const sc_proxy<BV> &, bool);
    const auto function = static_cast<Fn_comma_019>(&sc_dt::operator, );
    decltype(auto) result = function(bv, true);
    emit_matrix("comma_019", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_020 = sc_concref_r<BV, BV> (*)(sc_proxy<BV> &, bool);
    const auto function = static_cast<Fn_comma_020>(&sc_dt::operator, );
    decltype(auto) result = function(bv, true);
    emit_matrix("comma_020", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_021 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, const char *);
    const auto function = static_cast<Fn_comma_021>(&sc_dt::operator, );
    decltype(auto) result = function(bv, "10XZ01");
    emit_matrix("comma_021", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_022 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_022>(&sc_dt::operator, );
    decltype(auto) result = function(bv, logic);
    emit_matrix("comma_022", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_023 = sc_concref_r<BV, LV> (*)(sc_proxy<BV> &, const char *);
    const auto function = static_cast<Fn_comma_023>(&sc_dt::operator, );
    decltype(auto) result = function(bv, "10XZ01");
    emit_matrix("comma_023", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_024 = sc_concref_r<BV, LV> (*)(sc_proxy<BV> &, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_024>(&sc_dt::operator, );
    decltype(auto) result = function(bv, logic);
    emit_matrix("comma_024", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_025 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_025>(&sc_dt::operator, );
    decltype(auto) result = function(bv, lv);
    emit_matrix("comma_025", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_026 = sc_concref_r<BV, LV> (*)(const sc_proxy<BV> &, sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_026>(&sc_dt::operator, );
    decltype(auto) result = function(bv, lv);
    emit_matrix("comma_026", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_027 = sc_concref_r<BV, LV> (*)(sc_proxy<BV> &, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_027>(&sc_dt::operator, );
    decltype(auto) result = function(bv, lv);
    emit_matrix("comma_027", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_028 = sc_concref_r<BV, sc_bitref_r<LV>> (*)(const sc_proxy<BV> &, sc_bitref<LV>);
    const auto function = static_cast<Fn_comma_028>(&sc_dt::operator, );
    decltype(auto) result = function(bv, bit_lv);
    emit_matrix("comma_028", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_029 = sc_concref_r<BV, sc_bitref_r<LV>> (*)(const sc_proxy<BV> &, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_comma_029>(&sc_dt::operator, );
    decltype(auto) result = function(bv, bitr_lv);
    emit_matrix("comma_029", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_030 = sc_concref_r<BV, sc_bitref_r<LV>> (*)(sc_proxy<BV> &, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_comma_030>(&sc_dt::operator, );
    decltype(auto) result = function(bv, bitr_lv);
    emit_matrix("comma_030", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_031 = sc_concref_r<BV, sc_concref_r<LV, BV>> (*)(const sc_proxy<BV> &, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_comma_031>(&sc_dt::operator, );
    decltype(auto) result = function(bv, pair_lb);
    emit_matrix("comma_031", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_032 = sc_concref_r<BV, sc_concref_r<LV, BV>> (*)(const sc_proxy<BV> &, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_comma_032>(&sc_dt::operator, );
    decltype(auto) result = function(bv, pairr_lb);
    emit_matrix("comma_032", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_033 = sc_concref_r<BV, sc_concref_r<LV, BV>> (*)(sc_proxy<BV> &, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_comma_033>(&sc_dt::operator, );
    decltype(auto) result = function(bv, pairr_lb);
    emit_matrix("comma_033", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_034 = sc_concref_r<BV, sc_subref_r<LV>> (*)(const sc_proxy<BV> &, sc_subref<LV>);
    const auto function = static_cast<Fn_comma_034>(&sc_dt::operator, );
    decltype(auto) result = function(bv, range_lv);
    emit_matrix("comma_034", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_035 = sc_concref_r<BV, sc_subref_r<LV>> (*)(const sc_proxy<BV> &, sc_subref_r<LV>);
    const auto function = static_cast<Fn_comma_035>(&sc_dt::operator, );
    decltype(auto) result = function(bv, ranger_lv);
    emit_matrix("comma_035", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_036 = sc_concref_r<BV, sc_subref_r<LV>> (*)(sc_proxy<BV> &, sc_subref_r<LV>);
    const auto function = static_cast<Fn_comma_036>(&sc_dt::operator, );
    decltype(auto) result = function(bv, ranger_lv);
    emit_matrix("comma_036", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_037 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_037>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, lv);
    emit_matrix("comma_037", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_038 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_038>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, lv);
    emit_matrix("comma_038", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_039 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_039>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, lv);
    emit_matrix("comma_039", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_040 = sc_concref_r<sc_bitref_r<BV>, sc_bitref_r<LV>> (*)(sc_bitref<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_comma_040>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, bitr_lv);
    emit_matrix("comma_040", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_041 = sc_concref_r<sc_bitref_r<BV>, sc_bitref_r<LV>> (*)(sc_bitref_r<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_comma_041>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, bit_lv);
    emit_matrix("comma_041", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_042 = sc_concref_r<sc_bitref_r<BV>, sc_bitref_r<LV>> (*)(sc_bitref_r<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_comma_042>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, bitr_lv);
    emit_matrix("comma_042", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_043 = sc_concref_r<sc_bitref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_bitref<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_comma_043>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, pairr_lb);
    emit_matrix("comma_043", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_044 = sc_concref_r<sc_bitref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_bitref_r<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_comma_044>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, pair_lb);
    emit_matrix("comma_044", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_045 = sc_concref_r<sc_bitref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_bitref_r<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_comma_045>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, pairr_lb);
    emit_matrix("comma_045", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_046 = sc_concref_r<sc_bitref_r<BV>, sc_subref_r<LV>> (*)(sc_bitref<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_comma_046>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, ranger_lv);
    emit_matrix("comma_046", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_047 = sc_concref_r<sc_bitref_r<BV>, sc_subref_r<LV>> (*)(sc_bitref_r<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_comma_047>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, range_lv);
    emit_matrix("comma_047", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_048 = sc_concref_r<sc_bitref_r<BV>, sc_subref_r<LV>> (*)(sc_bitref_r<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_comma_048>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, ranger_lv);
    emit_matrix("comma_048", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_049 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref<BV>, bool);
    const auto function = static_cast<Fn_comma_049>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, true);
    emit_matrix("comma_049", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_050 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref_r<BV>, bool);
    const auto function = static_cast<Fn_comma_050>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, true);
    emit_matrix("comma_050", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_051 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref<BV>, bool);
    const auto function = static_cast<Fn_comma_051>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, true);
    emit_matrix("comma_051", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_052 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref<BV>, const char *);
    const auto function = static_cast<Fn_comma_052>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, "10XZ01");
    emit_matrix("comma_052", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_053 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_053>(&sc_dt::operator, );
    decltype(auto) result = function(bit_bv, logic);
    emit_matrix("comma_053", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_054 = sc_concref_r<sc_bitref_r<BV>, BV> (*)(sc_bitref_r<BV>, bool);
    const auto function = static_cast<Fn_comma_054>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, true);
    emit_matrix("comma_054", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_055 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, const char *);
    const auto function = static_cast<Fn_comma_055>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, "10XZ01");
    emit_matrix("comma_055", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_056 = sc_concref_r<sc_bitref_r<BV>, LV> (*)(sc_bitref_r<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_056>(&sc_dt::operator, );
    decltype(auto) result = function(bitr_bv, logic);
    emit_matrix("comma_056", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_057 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref<BV, LV>, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_057>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, bv);
    emit_matrix("comma_057", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_058 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref_r<BV, LV>, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_058>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, bv);
    emit_matrix("comma_058", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_059 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref_r<BV, LV>, sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_059>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, bv);
    emit_matrix("comma_059", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_060 = sc_concref_r<sc_concref_r<BV, LV>, sc_bitref_r<BV>> (*)(sc_concref<BV, LV>, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_comma_060>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, bitr_bv);
    emit_matrix("comma_060", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_061 = sc_concref_r<sc_concref_r<BV, LV>, sc_bitref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_bitref<BV>);
    const auto function = static_cast<Fn_comma_061>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, bit_bv);
    emit_matrix("comma_061", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_062 = sc_concref_r<sc_concref_r<BV, LV>, sc_bitref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_comma_062>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, bitr_bv);
    emit_matrix("comma_062", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_063 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_concref_r<BV, LV>> (*)(sc_concref<BV, LV>, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_comma_063>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, pairr_bl);
    emit_matrix("comma_063", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_064 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_concref_r<BV, LV>> (*)(sc_concref_r<BV, LV>, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_comma_064>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, pair_bl);
    emit_matrix("comma_064", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_065 =
      sc_concref_r<sc_concref_r<BV, LV>, sc_concref_r<BV, LV>> (*)(sc_concref_r<BV, LV>, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_comma_065>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, pairr_bl);
    emit_matrix("comma_065", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_066 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref<BV, LV>, bool);
    const auto function = static_cast<Fn_comma_066>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, true);
    emit_matrix("comma_066", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_067 = sc_concref_r<sc_concref_r<BV, LV>, BV> (*)(sc_concref_r<BV, LV>, bool);
    const auto function = static_cast<Fn_comma_067>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, true);
    emit_matrix("comma_067", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_068 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref<BV, LV>, const char *);
    const auto function = static_cast<Fn_comma_068>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, "10XZ01");
    emit_matrix("comma_068", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_069 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref<BV, LV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_069>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, logic);
    emit_matrix("comma_069", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_070 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref_r<BV, LV>, const char *);
    const auto function = static_cast<Fn_comma_070>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, "10XZ01");
    emit_matrix("comma_070", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_071 = sc_concref_r<sc_concref_r<BV, LV>, LV> (*)(sc_concref_r<BV, LV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_071>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, logic);
    emit_matrix("comma_071", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_072 = sc_concref_r<sc_concref_r<BV, LV>, sc_subref_r<BV>> (*)(sc_concref<BV, LV>, sc_subref_r<BV>);
    const auto function = static_cast<Fn_comma_072>(&sc_dt::operator, );
    decltype(auto) result = function(pair_bl, ranger_bv);
    emit_matrix("comma_072", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_073 = sc_concref_r<sc_concref_r<BV, LV>, sc_subref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_subref<BV>);
    const auto function = static_cast<Fn_comma_073>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, range_bv);
    emit_matrix("comma_073", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_074 = sc_concref_r<sc_concref_r<BV, LV>, sc_subref_r<BV>> (*)(sc_concref_r<BV, LV>, sc_subref_r<BV>);
    const auto function = static_cast<Fn_comma_074>(&sc_dt::operator, );
    decltype(auto) result = function(pairr_bl, ranger_bv);
    emit_matrix("comma_074", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_075 = sc_concref_r<BV, BV> (*)(bool, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_075>(&sc_dt::operator, );
    decltype(auto) result = function(true, bv);
    emit_matrix("comma_075", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_076 = sc_concref_r<BV, BV> (*)(bool, sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_076>(&sc_dt::operator, );
    decltype(auto) result = function(true, bv);
    emit_matrix("comma_076", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_077 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref<BV>);
    const auto function = static_cast<Fn_comma_077>(&sc_dt::operator, );
    decltype(auto) result = function(true, bit_bv);
    emit_matrix("comma_077", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_078 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_comma_078>(&sc_dt::operator, );
    decltype(auto) result = function(true, bitr_bv);
    emit_matrix("comma_078", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_079 = sc_concref_r<BV, sc_concref_r<BV, LV>> (*)(bool, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_comma_079>(&sc_dt::operator, );
    decltype(auto) result = function(true, pair_bl);
    emit_matrix("comma_079", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_080 = sc_concref_r<BV, sc_concref_r<BV, LV>> (*)(bool, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_comma_080>(&sc_dt::operator, );
    decltype(auto) result = function(true, pairr_bl);
    emit_matrix("comma_080", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_081 = sc_concref_r<BV, sc_subref_r<BV>> (*)(bool, sc_subref<BV>);
    const auto function = static_cast<Fn_comma_081>(&sc_dt::operator, );
    decltype(auto) result = function(true, range_bv);
    emit_matrix("comma_081", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_082 = sc_concref_r<BV, sc_subref_r<BV>> (*)(bool, sc_subref_r<BV>);
    const auto function = static_cast<Fn_comma_082>(&sc_dt::operator, );
    decltype(auto) result = function(true, ranger_bv);
    emit_matrix("comma_082", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_083 = sc_concref_r<LV, BV> (*)(const char *, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_083>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", bv);
    emit_matrix("comma_083", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_084 = sc_concref_r<LV, BV> (*)(const char *, sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_084>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", bv);
    emit_matrix("comma_084", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_085 = sc_concref_r<LV, BV> (*)(const sc_dt::sc_logic &, const sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_085>(&sc_dt::operator, );
    decltype(auto) result = function(logic, bv);
    emit_matrix("comma_085", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_086 = sc_concref_r<LV, BV> (*)(const sc_dt::sc_logic &, sc_proxy<BV> &);
    const auto function = static_cast<Fn_comma_086>(&sc_dt::operator, );
    decltype(auto) result = function(logic, bv);
    emit_matrix("comma_086", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_087 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref<BV>);
    const auto function = static_cast<Fn_comma_087>(&sc_dt::operator, );
    decltype(auto) result = function(true, bit_bv);
    emit_matrix("comma_087", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_088 = sc_concref_r<BV, sc_bitref_r<BV>> (*)(bool, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_comma_088>(&sc_dt::operator, );
    decltype(auto) result = function(true, bitr_bv);
    emit_matrix("comma_088", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_089 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const char *, sc_bitref<BV>);
    const auto function = static_cast<Fn_comma_089>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", bit_bv);
    emit_matrix("comma_089", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_090 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const char *, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_comma_090>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", bitr_bv);
    emit_matrix("comma_090", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_091 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const sc_dt::sc_logic &, sc_bitref<BV>);
    const auto function = static_cast<Fn_comma_091>(&sc_dt::operator, );
    decltype(auto) result = function(logic, bit_bv);
    emit_matrix("comma_091", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_092 = sc_concref_r<LV, sc_bitref_r<BV>> (*)(const sc_dt::sc_logic &, sc_bitref_r<BV>);
    const auto function = static_cast<Fn_comma_092>(&sc_dt::operator, );
    decltype(auto) result = function(logic, bitr_bv);
    emit_matrix("comma_092", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_093 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const char *, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_comma_093>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", pair_bl);
    emit_matrix("comma_093", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_094 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const char *, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_comma_094>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", pairr_bl);
    emit_matrix("comma_094", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_095 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const sc_dt::sc_logic &, sc_concref<BV, LV>);
    const auto function = static_cast<Fn_comma_095>(&sc_dt::operator, );
    decltype(auto) result = function(logic, pair_bl);
    emit_matrix("comma_095", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_096 = sc_concref_r<LV, sc_concref_r<BV, LV>> (*)(const sc_dt::sc_logic &, sc_concref_r<BV, LV>);
    const auto function = static_cast<Fn_comma_096>(&sc_dt::operator, );
    decltype(auto) result = function(logic, pairr_bl);
    emit_matrix("comma_096", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_097 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const char *, sc_subref<BV>);
    const auto function = static_cast<Fn_comma_097>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", range_bv);
    emit_matrix("comma_097", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_098 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const char *, sc_subref_r<BV>);
    const auto function = static_cast<Fn_comma_098>(&sc_dt::operator, );
    decltype(auto) result = function("10XZ01", ranger_bv);
    emit_matrix("comma_098", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_099 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const sc_dt::sc_logic &, sc_subref<BV>);
    const auto function = static_cast<Fn_comma_099>(&sc_dt::operator, );
    decltype(auto) result = function(logic, range_bv);
    emit_matrix("comma_099", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_100 = sc_concref_r<LV, sc_subref_r<BV>> (*)(const sc_dt::sc_logic &, sc_subref_r<BV>);
    const auto function = static_cast<Fn_comma_100>(&sc_dt::operator, );
    decltype(auto) result = function(logic, ranger_bv);
    emit_matrix("comma_100", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_101 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_101>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, lv);
    emit_matrix("comma_101", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_102 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, const sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_102>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, lv);
    emit_matrix("comma_102", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_103 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, sc_proxy<LV> &);
    const auto function = static_cast<Fn_comma_103>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, lv);
    emit_matrix("comma_103", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_104 = sc_concref_r<sc_subref_r<BV>, sc_bitref_r<LV>> (*)(sc_subref<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_comma_104>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, bitr_lv);
    emit_matrix("comma_104", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_105 = sc_concref_r<sc_subref_r<BV>, sc_bitref_r<LV>> (*)(sc_subref_r<BV>, sc_bitref<LV>);
    const auto function = static_cast<Fn_comma_105>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, bit_lv);
    emit_matrix("comma_105", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_106 = sc_concref_r<sc_subref_r<BV>, sc_bitref_r<LV>> (*)(sc_subref_r<BV>, sc_bitref_r<LV>);
    const auto function = static_cast<Fn_comma_106>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, bitr_lv);
    emit_matrix("comma_106", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_107 = sc_concref_r<sc_subref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_subref<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_comma_107>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, pairr_lb);
    emit_matrix("comma_107", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_108 = sc_concref_r<sc_subref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_subref_r<BV>, sc_concref<LV, BV>);
    const auto function = static_cast<Fn_comma_108>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, pair_lb);
    emit_matrix("comma_108", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_109 = sc_concref_r<sc_subref_r<BV>, sc_concref_r<LV, BV>> (*)(sc_subref_r<BV>, sc_concref_r<LV, BV>);
    const auto function = static_cast<Fn_comma_109>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, pairr_lb);
    emit_matrix("comma_109", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_110 = sc_concref_r<sc_subref_r<BV>, sc_subref_r<LV>> (*)(sc_subref<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_comma_110>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, ranger_lv);
    emit_matrix("comma_110", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_111 = sc_concref_r<sc_subref_r<BV>, sc_subref_r<LV>> (*)(sc_subref_r<BV>, sc_subref<LV>);
    const auto function = static_cast<Fn_comma_111>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, range_lv);
    emit_matrix("comma_111", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_112 = sc_concref_r<sc_subref_r<BV>, sc_subref_r<LV>> (*)(sc_subref_r<BV>, sc_subref_r<LV>);
    const auto function = static_cast<Fn_comma_112>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, ranger_lv);
    emit_matrix("comma_112", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_113 = sc_concref_r<sc_subref_r<BV>, BV> (*)(sc_subref<BV>, bool);
    const auto function = static_cast<Fn_comma_113>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, true);
    emit_matrix("comma_113", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_114 = sc_concref_r<sc_subref_r<BV>, BV> (*)(sc_subref_r<BV>, bool);
    const auto function = static_cast<Fn_comma_114>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, true);
    emit_matrix("comma_114", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_115 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref<BV>, const char *);
    const auto function = static_cast<Fn_comma_115>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, "10XZ01");
    emit_matrix("comma_115", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_116 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_116>(&sc_dt::operator, );
    decltype(auto) result = function(range_bv, logic);
    emit_matrix("comma_116", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_117 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, const char *);
    const auto function = static_cast<Fn_comma_117>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, "10XZ01");
    emit_matrix("comma_117", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_118 = sc_concref_r<sc_subref_r<BV>, LV> (*)(sc_subref_r<BV>, const sc_dt::sc_logic &);
    const auto function = static_cast<Fn_comma_118>(&sc_dt::operator, );
    decltype(auto) result = function(ranger_bv, logic);
    emit_matrix("comma_118", result.length(), render(result), result_type_name(result));
  }
  {
    using Fn_comma_119 = sc_dt::sc_concatref &(*)(sc_dt::sc_value_base &, sc_dt::sc_value_base &);
    const auto function = static_cast<Fn_comma_119>(&sc_dt::operator, );
    decltype(auto) result = function(signed_value, signed_value);
    emit_matrix("comma_119", result.length(), result.to_string(sc_dt::SC_BIN), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::concat<BV, LV>(bitr_bv, bitr_lv);
    emit_matrix("parent_bit_pair", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::operator, <BV, LV>(bitr_bv, bitr_lv);
    emit_matrix("comma_parent_bit_pair", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::concat<BV, BV, LV>(bitr_bv, pairr_bl);
    emit_matrix("parent_bit_concat", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::operator, <BV, BV, LV>(bitr_bv, pairr_bl);
    emit_matrix("comma_parent_bit_concat", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::concat<BV>(bitr_bv, "10XZ01");
    emit_matrix("parent_bit_text_bv", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::operator, <BV>(bitr_bv, "10XZ01");
    emit_matrix("comma_parent_bit_text_bv", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::concat<LV>(bitr_lv, "10XZ01");
    emit_matrix("parent_bit_text_lv", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::operator, <LV>(bitr_lv, "10XZ01");
    emit_matrix("comma_parent_bit_text_lv", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::concat<BV, LV, BV, LV>(pairr_bl, pairr_bl);
    emit_matrix("parent_nested_pair", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::operator, <BV, LV, BV, LV>(pairr_bl, pairr_bl);
    emit_matrix("comma_parent_nested_pair", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::concat<BV, LV>(pairr_bl, "10XZ01");
    emit_matrix("parent_concat_text", result.length(), render(result), result_type_name(result));
  }
  {
    decltype(auto) result = sc_dt::operator, <BV, LV>(pairr_bl, "10XZ01");
    emit_matrix("comma_parent_concat_text", result.length(), render(result), result_type_name(result));
  }
}

}  // namespace sc_dt_concat_functions_probe

#endif  // PYSYSTEMC_TEST_SC_DT_CONCAT_MATRIX_PROBE_H
