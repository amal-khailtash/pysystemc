/** @file Independent native fixed-point value reference and overload matrix. */
#include <algorithm>
#include <iostream>
#include <systemc>

#include "sc_dt_fxvalues_probe.h"

namespace sc_dt_fxvalues_probe
{
void run_constructor_matrix()
{
  Sources sources;
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, float>("sc_dt::sc_fxval|float", Scalars::real_float());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, unsigned long>("sc_dt::sc_fxval|unsigned long",
                                                                             Scalars::unsigned_long_integer());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval|const sc_dt::sc_unsigned &", SourceView::unsigned_value(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval|const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::uint64>("sc_dt::sc_fxval|sc_dt::uint64",
                                                                             Scalars::unsigned_64());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval|const sc_dt::sc_fxnum_fast &", SourceView::number_fast(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_signed>(
    "sc_dt::sc_fxval|const sc_dt::sc_signed &", SourceView::signed_value(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval|const sc_dt::sc_fxnum &", SourceView::number(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval|const sc_dt::sc_int_base &", SourceView::sint(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, unsigned int>("sc_dt::sc_fxval|unsigned int",
                                                                            Scalars::unsigned_integer());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, const char*>("sc_dt::sc_fxval|const char *",
                                                                           Scalars::text());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval|const sc_dt::sc_fxval &", SourceView::value(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval|const sc_dt::sc_uint_base &", SourceView::uint(sources));
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::int64>("sc_dt::sc_fxval|sc_dt::int64",
                                                                            Scalars::signed_64());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, long>("sc_dt::sc_fxval|long", Scalars::long_integer());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, double>("sc_dt::sc_fxval|double", Scalars::real_double());
  emit_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer, int>("sc_dt::sc_fxval|int", Scalars::integer());
  emit_empty_constructor<sc_dt::sc_fxval, sc_dt::sc_fxval_observer>("sc_dt::sc_fxval|observer");
  emit_empty_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer>("sc_dt::sc_fxval_fast|observer");
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_fxnum &", SourceView::number(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, unsigned int>(
    "sc_dt::sc_fxval_fast|unsigned int", Scalars::unsigned_integer());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_int_base &", SourceView::sint(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, int>("sc_dt::sc_fxval_fast|int",
                                                                             Scalars::integer());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, unsigned long>(
    "sc_dt::sc_fxval_fast|unsigned long", Scalars::unsigned_long_integer());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_fxnum_fast &", SourceView::number_fast(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, float>("sc_dt::sc_fxval_fast|float",
                                                                               Scalars::real_float());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_unsigned &", SourceView::unsigned_value(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, long>("sc_dt::sc_fxval_fast|long",
                                                                              Scalars::long_integer());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, const char*>(
    "sc_dt::sc_fxval_fast|const char *", Scalars::text());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, double>("sc_dt::sc_fxval_fast|double",
                                                                                Scalars::real_double());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_fxval &", SourceView::value(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_signed>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_signed &", SourceView::signed_value(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::uint64>(
    "sc_dt::sc_fxval_fast|sc_dt::uint64", Scalars::unsigned_64());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::int64>(
    "sc_dt::sc_fxval_fast|sc_dt::int64", Scalars::signed_64());
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources));
  emit_constructor<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval_fast|const sc_dt::sc_uint_base &", SourceView::uint(sources));
}

void run_assignment_matrix()
{
  Sources sources;
  emit_assignment<sc_dt::sc_fxval, sc_dt::int64>("sc_dt::sc_fxval|sc_dt::int64", Scalars::signed_64());
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_signed>("sc_dt::sc_fxval|const sc_dt::sc_signed &",
                                                     SourceView::signed_value(sources));
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_fxnum>("sc_dt::sc_fxval|const sc_dt::sc_fxnum &",
                                                    SourceView::number(sources));
  emit_assignment<sc_dt::sc_fxval, unsigned int>("sc_dt::sc_fxval|unsigned int", Scalars::unsigned_integer());
  emit_assignment<sc_dt::sc_fxval, unsigned long>("sc_dt::sc_fxval|unsigned long", Scalars::unsigned_long_integer());
  emit_assignment<sc_dt::sc_fxval, const char*>("sc_dt::sc_fxval|const char *", Scalars::text());
  emit_assignment<sc_dt::sc_fxval, double>("sc_dt::sc_fxval|double", Scalars::real_double());
  emit_assignment<sc_dt::sc_fxval, sc_dt::uint64>("sc_dt::sc_fxval|sc_dt::uint64", Scalars::unsigned_64());
  emit_assignment<sc_dt::sc_fxval, int>("sc_dt::sc_fxval|int", Scalars::integer());
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast>("sc_dt::sc_fxval|const sc_dt::sc_fxnum_fast &",
                                                         SourceView::number_fast(sources));
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_int_base>("sc_dt::sc_fxval|const sc_dt::sc_int_base &",
                                                       SourceView::sint(sources));
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_fxval_fast>("sc_dt::sc_fxval|const sc_dt::sc_fxval_fast &",
                                                         SourceView::value_fast(sources));
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_uint_base>("sc_dt::sc_fxval|const sc_dt::sc_uint_base &",
                                                        SourceView::uint(sources));
  emit_assignment<sc_dt::sc_fxval, long>("sc_dt::sc_fxval|long", Scalars::long_integer());
  emit_assignment<sc_dt::sc_fxval, float>("sc_dt::sc_fxval|float", Scalars::real_float());
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_unsigned>("sc_dt::sc_fxval|const sc_dt::sc_unsigned &",
                                                       SourceView::unsigned_value(sources));
  emit_assignment<sc_dt::sc_fxval, sc_dt::sc_fxval>("sc_dt::sc_fxval|const sc_dt::sc_fxval &",
                                                    SourceView::value(sources));
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast>("sc_dt::sc_fxval_fast|const sc_dt::sc_fxnum_fast &",
                                                              SourceView::number_fast(sources));
  emit_assignment<sc_dt::sc_fxval_fast, long>("sc_dt::sc_fxval_fast|long", Scalars::long_integer());
  emit_assignment<sc_dt::sc_fxval_fast, float>("sc_dt::sc_fxval_fast|float", Scalars::real_float());
  emit_assignment<sc_dt::sc_fxval_fast, double>("sc_dt::sc_fxval_fast|double", Scalars::real_double());
  emit_assignment<sc_dt::sc_fxval_fast, unsigned long>("sc_dt::sc_fxval_fast|unsigned long",
                                                       Scalars::unsigned_long_integer());
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum>("sc_dt::sc_fxval_fast|const sc_dt::sc_fxnum &",
                                                         SourceView::number(sources));
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::uint64>("sc_dt::sc_fxval_fast|sc_dt::uint64", Scalars::unsigned_64());
  emit_assignment<sc_dt::sc_fxval_fast, int>("sc_dt::sc_fxval_fast|int", Scalars::integer());
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_int_base>("sc_dt::sc_fxval_fast|const sc_dt::sc_int_base &",
                                                            SourceView::sint(sources));
  emit_assignment<sc_dt::sc_fxval_fast, unsigned int>("sc_dt::sc_fxval_fast|unsigned int", Scalars::unsigned_integer());
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast>("sc_dt::sc_fxval_fast|const sc_dt::sc_fxval_fast &",
                                                              SourceView::value_fast(sources));
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::int64>("sc_dt::sc_fxval_fast|sc_dt::int64", Scalars::signed_64());
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_fxval>("sc_dt::sc_fxval_fast|const sc_dt::sc_fxval &",
                                                         SourceView::value(sources));
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base>("sc_dt::sc_fxval_fast|const sc_dt::sc_uint_base &",
                                                             SourceView::uint(sources));
  emit_assignment<sc_dt::sc_fxval_fast, const char*>("sc_dt::sc_fxval_fast|const char *", Scalars::text());
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned>("sc_dt::sc_fxval_fast|const sc_dt::sc_unsigned &",
                                                            SourceView::unsigned_value(sources));
  emit_assignment<sc_dt::sc_fxval_fast, sc_dt::sc_signed>("sc_dt::sc_fxval_fast|const sc_dt::sc_signed &",
                                                          SourceView::signed_value(sources));
}

void run_compound_matrix()
{
  Sources sources;
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_fxnum &",
                                                     SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_uint_base, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_uint_base &",
                                                         SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_fxnum_fast &",
                                                          SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval, int, 0>("sc_dt::sc_fxval::operator+=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::int64, 0>("sc_dt::sc_fxval::operator+=|sc_dt::int64", Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval, float, 0>("sc_dt::sc_fxval::operator+=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_signed, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_signed &",
                                                      SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval, long, 0>("sc_dt::sc_fxval::operator+=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval, const char*, 0>("sc_dt::sc_fxval::operator+=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval, sc_dt::uint64, 0>("sc_dt::sc_fxval::operator+=|sc_dt::uint64", Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_fxval &",
                                                     SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_fxval_fast &",
                                                          SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_unsigned, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_unsigned &",
                                                        SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval, double, 0>("sc_dt::sc_fxval::operator+=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_int_base, 0>("sc_dt::sc_fxval::operator+=|const sc_dt::sc_int_base &",
                                                        SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval, unsigned long, 0>("sc_dt::sc_fxval::operator+=|unsigned long",
                                                   Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval, unsigned int, 0>("sc_dt::sc_fxval::operator+=|unsigned int",
                                                  Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval, double, 1>("sc_dt::sc_fxval::operator-=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_uint_base, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_uint_base &",
                                                         SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_fxval &",
                                                     SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_int_base, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_int_base &",
                                                        SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_fxnum_fast &",
                                                          SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_signed, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_signed &",
                                                      SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval, const char*, 1>("sc_dt::sc_fxval::operator-=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_fxnum &",
                                                     SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_fxval_fast &",
                                                          SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::int64, 1>("sc_dt::sc_fxval::operator-=|sc_dt::int64", Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval, long, 1>("sc_dt::sc_fxval::operator-=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::uint64, 1>("sc_dt::sc_fxval::operator-=|sc_dt::uint64", Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval, int, 1>("sc_dt::sc_fxval::operator-=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_unsigned, 1>("sc_dt::sc_fxval::operator-=|const sc_dt::sc_unsigned &",
                                                        SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval, float, 1>("sc_dt::sc_fxval::operator-=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval, unsigned long, 1>("sc_dt::sc_fxval::operator-=|unsigned long",
                                                   Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval, unsigned int, 1>("sc_dt::sc_fxval::operator-=|unsigned int",
                                                  Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_fxnum &",
                                                     SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_signed, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_signed &",
                                                      SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_uint_base, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_uint_base &",
                                                         SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval, unsigned int, 2>("sc_dt::sc_fxval::operator*=|unsigned int",
                                                  Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval, unsigned long, 2>("sc_dt::sc_fxval::operator*=|unsigned long",
                                                   Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval, const char*, 2>("sc_dt::sc_fxval::operator*=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval, sc_dt::int64, 2>("sc_dt::sc_fxval::operator*=|sc_dt::int64", Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval, sc_dt::uint64, 2>("sc_dt::sc_fxval::operator*=|sc_dt::uint64", Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval, int, 2>("sc_dt::sc_fxval::operator*=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_fxval &",
                                                     SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_fxnum_fast &",
                                                          SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_fxval_fast &",
                                                          SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval, float, 2>("sc_dt::sc_fxval::operator*=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_int_base, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_int_base &",
                                                        SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval, double, 2>("sc_dt::sc_fxval::operator*=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval, long, 2>("sc_dt::sc_fxval::operator*=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_unsigned, 2>("sc_dt::sc_fxval::operator*=|const sc_dt::sc_unsigned &",
                                                        SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval, int, 3>("sc_dt::sc_fxval::operator/=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_fxval &",
                                                     SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_signed, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_signed &",
                                                      SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval, long, 3>("sc_dt::sc_fxval::operator/=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval, const char*, 3>("sc_dt::sc_fxval::operator/=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_fxnum_fast &",
                                                          SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval, float, 3>("sc_dt::sc_fxval::operator/=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_uint_base, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_uint_base &",
                                                         SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval, unsigned int, 3>("sc_dt::sc_fxval::operator/=|unsigned int",
                                                  Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval, sc_dt::int64, 3>("sc_dt::sc_fxval::operator/=|sc_dt::int64", Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_fxval_fast &",
                                                          SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::uint64, 3>("sc_dt::sc_fxval::operator/=|sc_dt::uint64", Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_fxnum, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_fxnum &",
                                                     SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_int_base, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_int_base &",
                                                        SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval, unsigned long, 3>("sc_dt::sc_fxval::operator/=|unsigned long",
                                                   Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval, double, 3>("sc_dt::sc_fxval::operator/=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval, sc_dt::sc_unsigned, 3>("sc_dt::sc_fxval::operator/=|const sc_dt::sc_unsigned &",
                                                        SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval, int, 4>("sc_dt::sc_fxval::operator<<=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval, int, 5>("sc_dt::sc_fxval::operator>>=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, 0>(
    "sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, 0>(
    "sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_int_base &", SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval_fast, long, 0>("sc_dt::sc_fxval_fast::operator+=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, 0>(
    "sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_unsigned &", SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::uint64, 0>("sc_dt::sc_fxval_fast::operator+=|sc_dt::uint64",
                                                        Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, 0>(
    "sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_fxnum_fast &", SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, 0>(
    "sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_uint_base &", SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_signed, 0>("sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_signed &",
                                                           SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, unsigned long, 0>("sc_dt::sc_fxval_fast::operator+=|unsigned long",
                                                        Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, 0>("sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_fxnum &",
                                                          SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval_fast, double, 0>("sc_dt::sc_fxval_fast::operator+=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval_fast, int, 0>("sc_dt::sc_fxval_fast::operator+=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval, 0>("sc_dt::sc_fxval_fast::operator+=|const sc_dt::sc_fxval &",
                                                          SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval_fast, unsigned int, 0>("sc_dt::sc_fxval_fast::operator+=|unsigned int",
                                                       Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval_fast, float, 0>("sc_dt::sc_fxval_fast::operator+=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval_fast, const char*, 0>("sc_dt::sc_fxval_fast::operator+=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::int64, 0>("sc_dt::sc_fxval_fast::operator+=|sc_dt::int64",
                                                       Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval_fast, long, 1>("sc_dt::sc_fxval_fast::operator-=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, 1>(
    "sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_int_base &", SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval_fast, const char*, 1>("sc_dt::sc_fxval_fast::operator-=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, 1>(
    "sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, 1>(
    "sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_unsigned &", SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_signed, 1>("sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_signed &",
                                                           SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, int, 1>("sc_dt::sc_fxval_fast::operator-=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval_fast, float, 1>("sc_dt::sc_fxval_fast::operator-=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::uint64, 1>("sc_dt::sc_fxval_fast::operator-=|sc_dt::uint64",
                                                        Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, 1>(
    "sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_uint_base &", SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval, 1>("sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_fxval &",
                                                          SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, 1>("sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_fxnum &",
                                                          SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval_fast, unsigned int, 1>("sc_dt::sc_fxval_fast::operator-=|unsigned int",
                                                       Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval_fast, unsigned long, 1>("sc_dt::sc_fxval_fast::operator-=|unsigned long",
                                                        Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval_fast, double, 1>("sc_dt::sc_fxval_fast::operator-=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, 1>(
    "sc_dt::sc_fxval_fast::operator-=|const sc_dt::sc_fxnum_fast &", SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::int64, 1>("sc_dt::sc_fxval_fast::operator-=|sc_dt::int64",
                                                       Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, 2>(
    "sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_uint_base &", SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::uint64, 2>("sc_dt::sc_fxval_fast::operator*=|sc_dt::uint64",
                                                        Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::int64, 2>("sc_dt::sc_fxval_fast::operator*=|sc_dt::int64",
                                                       Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, 2>(
    "sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_fxnum_fast &", SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, const char*, 2>("sc_dt::sc_fxval_fast::operator*=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, 2>(
    "sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_int_base &", SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval_fast, int, 2>("sc_dt::sc_fxval_fast::operator*=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, 2>("sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_fxnum &",
                                                          SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval_fast, unsigned int, 2>("sc_dt::sc_fxval_fast::operator*=|unsigned int",
                                                       Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval_fast, unsigned long, 2>("sc_dt::sc_fxval_fast::operator*=|unsigned long",
                                                        Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval_fast, float, 2>("sc_dt::sc_fxval_fast::operator*=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval, 2>("sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_fxval &",
                                                          SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, 2>(
    "sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, double, 2>("sc_dt::sc_fxval_fast::operator*=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_signed, 2>("sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_signed &",
                                                           SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, long, 2>("sc_dt::sc_fxval_fast::operator*=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, 2>(
    "sc_dt::sc_fxval_fast::operator*=|const sc_dt::sc_unsigned &", SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, 3>(
    "sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_int_base &", SourceView::sint(sources));
  emit_compound<sc_dt::sc_fxval_fast, float, 3>("sc_dt::sc_fxval_fast::operator/=|float", Scalars::real_float());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, 3>(
    "sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, double, 3>("sc_dt::sc_fxval_fast::operator/=|double", Scalars::real_double());
  emit_compound<sc_dt::sc_fxval_fast, unsigned long, 3>("sc_dt::sc_fxval_fast::operator/=|unsigned long",
                                                        Scalars::unsigned_long_integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, 3>(
    "sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_unsigned &", SourceView::unsigned_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_signed, 3>("sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_signed &",
                                                           SourceView::signed_value(sources));
  emit_compound<sc_dt::sc_fxval_fast, unsigned int, 3>("sc_dt::sc_fxval_fast::operator/=|unsigned int",
                                                       Scalars::unsigned_integer());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, 3>(
    "sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_uint_base &", SourceView::uint(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, 3>(
    "sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_fxnum_fast &", SourceView::number_fast(sources));
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, 3>("sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_fxnum &",
                                                          SourceView::number(sources));
  emit_compound<sc_dt::sc_fxval_fast, long, 3>("sc_dt::sc_fxval_fast::operator/=|long", Scalars::long_integer());
  emit_compound<sc_dt::sc_fxval_fast, int, 3>("sc_dt::sc_fxval_fast::operator/=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval_fast, const char*, 3>("sc_dt::sc_fxval_fast::operator/=|const char *", Scalars::text());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::int64, 3>("sc_dt::sc_fxval_fast::operator/=|sc_dt::int64",
                                                       Scalars::signed_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::uint64, 3>("sc_dt::sc_fxval_fast::operator/=|sc_dt::uint64",
                                                        Scalars::unsigned_64());
  emit_compound<sc_dt::sc_fxval_fast, sc_dt::sc_fxval, 3>("sc_dt::sc_fxval_fast::operator/=|const sc_dt::sc_fxval &",
                                                          SourceView::value(sources));
  emit_compound<sc_dt::sc_fxval_fast, int, 4>("sc_dt::sc_fxval_fast::operator<<=|int", Scalars::integer());
  emit_compound<sc_dt::sc_fxval_fast, int, 5>("sc_dt::sc_fxval_fast::operator>>=|int", Scalars::integer());
}

void run_binary_matrix()
{
  Sources sources;
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxnum &", SourceView::number_fast(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_unsigned &", SourceView::value(sources),
    SourceView::unsigned_value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, const char*>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const char *",
                                                             SourceView::number(sources), Scalars::text());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_int_base &", SourceView::value(sources),
    SourceView::sint(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_unsigned &", SourceView::number(sources),
    SourceView::unsigned_value(sources));
  emit_binary<sc_dt::sc_fxval, unsigned int, sc_dt::sc_fxval>("sc_dt::sc_fxval &,unsigned int,const sc_dt::sc_fxval &",
                                                              Scalars::unsigned_integer(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, int>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,int",
                                                     SourceView::value(sources), Scalars::integer());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval &", SourceView::value_fast(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::int64, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,sc_dt::int64,const sc_dt::sc_fxnum &",
                                                              Scalars::signed_64(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, long>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,long",
                                                      SourceView::value(sources), Scalars::long_integer());
  emit_binary<sc_dt::sc_fxval, float, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,float,const sc_dt::sc_fxnum &",
                                                       Scalars::real_float(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, int, sc_dt::sc_fxval>("sc_dt::sc_fxval &,int,const sc_dt::sc_fxval &",
                                                     Scalars::integer(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxval_fast &", SourceView::number(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval, const char*, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,const char *,const sc_dt::sc_fxnum &",
                                                             Scalars::text(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, unsigned long, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,unsigned long,const sc_dt::sc_fxval &", Scalars::unsigned_long_integer(),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxval &", SourceView::number(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, int>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,int",
                                                     SourceView::number(sources), Scalars::integer());
  emit_binary<sc_dt::sc_fxval, float, sc_dt::sc_fxval>("sc_dt::sc_fxval &,float,const sc_dt::sc_fxval &",
                                                       Scalars::real_float(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_signed, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_signed &,const sc_dt::sc_fxval &", SourceView::signed_value(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, const char*, sc_dt::sc_fxval>("sc_dt::sc_fxval &,const char *,const sc_dt::sc_fxval &",
                                                             Scalars::text(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &", SourceView::value(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &", SourceView::value(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval, double, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,double,const sc_dt::sc_fxnum &",
                                                        Scalars::real_double(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::uint64, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,sc_dt::uint64,const sc_dt::sc_fxval &", Scalars::unsigned_64(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::uint64>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,sc_dt::uint64", SourceView::number(sources), Scalars::unsigned_64());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_unsigned, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxval &", SourceView::unsigned_value(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, unsigned long>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,unsigned long", SourceView::value(sources),
    Scalars::unsigned_long_integer());
  emit_binary<sc_dt::sc_fxval, double, sc_dt::sc_fxval>("sc_dt::sc_fxval &,double,const sc_dt::sc_fxval &",
                                                        Scalars::real_double(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_uint_base, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxval &", SourceView::uint(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::int64>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,sc_dt::int64",
                                                              SourceView::value(sources), Scalars::signed_64());
  emit_binary<sc_dt::sc_fxval, unsigned int, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,unsigned int,const sc_dt::sc_fxnum &",
                                                              Scalars::unsigned_integer(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_signed>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_signed &", SourceView::value(sources),
    SourceView::signed_value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxval &", SourceView::number_fast(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, unsigned int>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,unsigned int",
                                                              SourceView::value(sources), Scalars::unsigned_integer());
  emit_binary<sc_dt::sc_fxval, unsigned long, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,unsigned long,const sc_dt::sc_fxnum &", Scalars::unsigned_long_integer(),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, long, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,long,const sc_dt::sc_fxnum &",
                                                      Scalars::long_integer(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_int_base, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_int_base &,const sc_dt::sc_fxval &", SourceView::sint(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxval &", SourceView::value(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::uint64>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,sc_dt::uint64", SourceView::value(sources), Scalars::unsigned_64());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, unsigned long>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,unsigned long", SourceView::number(sources),
    Scalars::unsigned_long_integer());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_uint_base &", SourceView::value(sources),
    SourceView::uint(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_uint_base, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxnum &", SourceView::uint(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::uint64, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,sc_dt::uint64,const sc_dt::sc_fxnum &", Scalars::unsigned_64(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_int_base, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_int_base &,const sc_dt::sc_fxnum &", SourceView::sint(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::int64>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,sc_dt::int64",
                                                              SourceView::number(sources), Scalars::signed_64());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, float>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,float",
                                                       SourceView::value(sources), Scalars::real_float());
  emit_binary<sc_dt::sc_fxval, long, sc_dt::sc_fxval>("sc_dt::sc_fxval &,long,const sc_dt::sc_fxval &",
                                                      Scalars::long_integer(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_signed, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_signed &,const sc_dt::sc_fxnum &", SourceView::signed_value(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum &", SourceView::value_fast(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::int64, sc_dt::sc_fxval>("sc_dt::sc_fxval &,sc_dt::int64,const sc_dt::sc_fxval &",
                                                              Scalars::signed_64(), SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_signed>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_signed &", SourceView::number(sources),
    SourceView::signed_value(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, long>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,long",
                                                      SourceView::number(sources), Scalars::long_integer());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_uint_base &", SourceView::number(sources),
    SourceView::uint(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_int_base &", SourceView::number(sources),
    SourceView::sint(sources));
  emit_binary<sc_dt::sc_fxval, int, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,int,const sc_dt::sc_fxnum &",
                                                     Scalars::integer(), SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, double>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,double",
                                                        SourceView::number(sources), Scalars::real_double());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, unsigned int>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,unsigned int",
                                                              SourceView::number(sources), Scalars::unsigned_integer());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, double>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,double",
                                                        SourceView::value(sources), Scalars::real_double());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, float>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,float",
                                                       SourceView::number(sources), Scalars::real_float());
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxnum &", SourceView::value(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_unsigned, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxnum &", SourceView::unsigned_value(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxnum_fast &", SourceView::number(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxnum &", SourceView::number(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval, sc_dt::sc_fxval, const char*>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const char *",
                                                             SourceView::value(sources), Scalars::text());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &", SourceView::value(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, unsigned long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,unsigned long", SourceView::value_fast(sources),
    Scalars::unsigned_long_integer());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_uint_base &", SourceView::value_fast(sources),
    SourceView::uint(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxval &", SourceView::number_fast(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxnum_fast &", SourceView::number(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::int64, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::int64,const sc_dt::sc_fxnum_fast &", Scalars::signed_64(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &", SourceView::value_fast(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::uint64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,sc_dt::uint64", SourceView::number_fast(sources),
    Scalars::unsigned_64());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxval_fast &",
    SourceView::number_fast(sources), SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxval_fast &", SourceView::number(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxval_fast &",
    SourceView::unsigned_value(sources), SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::uint64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,sc_dt::uint64", SourceView::value_fast(sources),
    Scalars::unsigned_64());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_int_base &,const sc_dt::sc_fxval_fast &", SourceView::sint(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,int", SourceView::number_fast(sources), Scalars::integer());
  emit_binary<sc_dt::sc_fxval_fast, float, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,float,const sc_dt::sc_fxval_fast &", Scalars::real_float(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::int64, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::int64,const sc_dt::sc_fxval_fast &", Scalars::signed_64(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxnum_fast &", SourceView::uint(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::uint64, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::uint64,const sc_dt::sc_fxval_fast &", Scalars::unsigned_64(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, float>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,float", SourceView::value_fast(sources),
    Scalars::real_float());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &", SourceView::value(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_unsigned &", SourceView::value_fast(sources),
    SourceView::unsigned_value(sources));
  emit_binary<sc_dt::sc_fxval_fast, int, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,int,const sc_dt::sc_fxval_fast &", Scalars::integer(), SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, unsigned int, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,unsigned int,const sc_dt::sc_fxval_fast &", Scalars::unsigned_integer(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_signed, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_signed &,const sc_dt::sc_fxnum_fast &", SourceView::signed_value(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, long, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,long,const sc_dt::sc_fxval_fast &", Scalars::long_integer(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_unsigned &", SourceView::number_fast(sources),
    SourceView::unsigned_value(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, const char*>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const char *", SourceView::number_fast(sources),
    Scalars::text());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, double>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,double", SourceView::value_fast(sources),
    Scalars::real_double());
  emit_binary<sc_dt::sc_fxval_fast, int, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,int,const sc_dt::sc_fxnum_fast &", Scalars::integer(), SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::uint64, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::uint64,const sc_dt::sc_fxnum_fast &", Scalars::unsigned_64(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,long", SourceView::value_fast(sources),
    Scalars::long_integer());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum &", SourceView::value_fast(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval_fast, float, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,float,const sc_dt::sc_fxnum_fast &", Scalars::real_float(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,int", SourceView::value_fast(sources), Scalars::integer());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval &", SourceView::value_fast(sources),
    SourceView::value(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_uint_base &", SourceView::number_fast(sources),
    SourceView::uint(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, const char*>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const char *", SourceView::value_fast(sources),
    Scalars::text());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxval_fast &", SourceView::uint(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxnum_fast &",
    SourceView::unsigned_value(sources), SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_int_base &", SourceView::value_fast(sources),
    SourceView::sint(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, unsigned int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,unsigned int", SourceView::value_fast(sources),
    Scalars::unsigned_integer());
  emit_binary<sc_dt::sc_fxval_fast, double, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,double,const sc_dt::sc_fxval_fast &", Scalars::real_double(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, double, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,double,const sc_dt::sc_fxnum_fast &", Scalars::real_double(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxnum_fast &",
    SourceView::number_fast(sources), SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxnum &", SourceView::number_fast(sources),
    SourceView::number(sources));
  emit_binary<sc_dt::sc_fxval_fast, const char*, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const char *,const sc_dt::sc_fxval_fast &", Scalars::text(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, unsigned long, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,unsigned long,const sc_dt::sc_fxval_fast &", Scalars::unsigned_long_integer(),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_signed, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_signed &,const sc_dt::sc_fxval_fast &", SourceView::signed_value(sources),
    SourceView::value_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, const char*, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const char *,const sc_dt::sc_fxnum_fast &", Scalars::text(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, long, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,long,const sc_dt::sc_fxnum_fast &", Scalars::long_integer(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_int_base &", SourceView::number_fast(sources),
    SourceView::sint(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_signed>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_signed &", SourceView::number_fast(sources),
    SourceView::signed_value(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, float>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,float", SourceView::number_fast(sources),
    Scalars::real_float());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, unsigned int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,unsigned int", SourceView::number_fast(sources),
    Scalars::unsigned_integer());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,long", SourceView::number_fast(sources),
    Scalars::long_integer());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, unsigned long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,unsigned long", SourceView::number_fast(sources),
    Scalars::unsigned_long_integer());
  emit_binary<sc_dt::sc_fxval_fast, unsigned long, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,unsigned long,const sc_dt::sc_fxnum_fast &", Scalars::unsigned_long_integer(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_int_base &,const sc_dt::sc_fxnum_fast &", SourceView::sint(sources),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_signed>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_signed &", SourceView::value_fast(sources),
    SourceView::signed_value(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::int64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,sc_dt::int64", SourceView::value_fast(sources),
    Scalars::signed_64());
  emit_binary<sc_dt::sc_fxval_fast, unsigned int, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,unsigned int,const sc_dt::sc_fxnum_fast &", Scalars::unsigned_integer(),
    SourceView::number_fast(sources));
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, double>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,double", SourceView::number_fast(sources),
    Scalars::real_double());
  emit_binary<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::int64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,sc_dt::int64", SourceView::number_fast(sources),
    Scalars::signed_64());
}

void run_operator_matrix()
{
  Sources sources;
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxval &", SourceView::value(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxnum &", SourceView::number(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_int_base &", SourceView::number(sources),
    SourceView::sint(sources));
  emit_operators<sc_dt::sc_fxval, int, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,int,const sc_dt::sc_fxnum &",
                                                        Scalars::integer(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_signed, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_signed &,const sc_dt::sc_fxnum &", SourceView::signed_value(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, float>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,float",
                                                          SourceView::number(sources), Scalars::real_float());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, int>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,int",
                                                        SourceView::number(sources), Scalars::integer());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxnum &", SourceView::value(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, unsigned int>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,unsigned int", SourceView::number(sources), Scalars::unsigned_integer());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_signed>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_signed &", SourceView::number(sources),
    SourceView::signed_value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval &", SourceView::value_fast(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &", SourceView::value(sources),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxnum &", SourceView::number_fast(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, double, sc_dt::sc_fxval>("sc_dt::sc_fxval &,double,const sc_dt::sc_fxval &",
                                                           Scalars::real_double(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, int>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,int",
                                                        SourceView::value(sources), Scalars::integer());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_unsigned &", SourceView::value(sources),
    SourceView::unsigned_value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_signed>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_signed &", SourceView::value(sources),
    SourceView::signed_value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_uint_base &", SourceView::number(sources),
    SourceView::uint(sources));
  emit_operators<sc_dt::sc_fxval, float, sc_dt::sc_fxval>("sc_dt::sc_fxval &,float,const sc_dt::sc_fxval &",
                                                          Scalars::real_float(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, double>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,double",
                                                           SourceView::value(sources), Scalars::real_double());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, const char*>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const char *", SourceView::value(sources), Scalars::text());
  emit_operators<sc_dt::sc_fxval, unsigned long, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,unsigned long,const sc_dt::sc_fxnum &", Scalars::unsigned_long_integer(),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, float, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,float,const sc_dt::sc_fxnum &",
                                                          Scalars::real_float(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &", SourceView::value(sources),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval, unsigned int, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,unsigned int,const sc_dt::sc_fxval &", Scalars::unsigned_integer(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, float>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,float",
                                                          SourceView::value(sources), Scalars::real_float());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxnum_fast &", SourceView::number(sources),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, long>("sc_dt::sc_fxval &,const sc_dt::sc_fxval &,long",
                                                         SourceView::value(sources), Scalars::long_integer());
  emit_operators<sc_dt::sc_fxval, long, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,long,const sc_dt::sc_fxnum &",
                                                         Scalars::long_integer(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::uint64, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,sc_dt::uint64,const sc_dt::sc_fxval &", Scalars::unsigned_64(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxval &", SourceView::number(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, unsigned long>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,unsigned long", SourceView::value(sources),
    Scalars::unsigned_long_integer());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, unsigned long>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,unsigned long", SourceView::number(sources),
    Scalars::unsigned_long_integer());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_unsigned, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxval &", SourceView::unsigned_value(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval_fast, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum &", SourceView::value_fast(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::int64, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,sc_dt::int64,const sc_dt::sc_fxval &", Scalars::signed_64(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::int64, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,sc_dt::int64,const sc_dt::sc_fxnum &", Scalars::signed_64(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_int_base, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_int_base &,const sc_dt::sc_fxval &", SourceView::sint(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, const char*>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const char *", SourceView::number(sources), Scalars::text());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, unsigned int>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,unsigned int", SourceView::value(sources), Scalars::unsigned_integer());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_uint_base &", SourceView::value(sources),
    SourceView::uint(sources));
  emit_operators<sc_dt::sc_fxval, unsigned int, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,unsigned int,const sc_dt::sc_fxnum &", Scalars::unsigned_integer(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_unsigned &", SourceView::number(sources),
    SourceView::unsigned_value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_uint_base, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxval &", SourceView::uint(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum_fast, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxval &", SourceView::number_fast(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_signed, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const sc_dt::sc_signed &,const sc_dt::sc_fxval &", SourceView::signed_value(sources),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, const char*, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,const char *,const sc_dt::sc_fxval &", Scalars::text(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::uint64>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,sc_dt::uint64", SourceView::value(sources), Scalars::unsigned_64());
  emit_operators<sc_dt::sc_fxval, sc_dt::uint64, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,sc_dt::uint64,const sc_dt::sc_fxnum &", Scalars::unsigned_64(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_int_base, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_int_base &,const sc_dt::sc_fxnum &", SourceView::sint(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,const sc_dt::sc_fxval_fast &", SourceView::number(sources),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_unsigned, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxnum &", SourceView::unsigned_value(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::int64>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,sc_dt::int64", SourceView::value(sources), Scalars::signed_64());
  emit_operators<sc_dt::sc_fxval, long, sc_dt::sc_fxval>("sc_dt::sc_fxval &,long,const sc_dt::sc_fxval &",
                                                         Scalars::long_integer(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxval, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxval &,const sc_dt::sc_int_base &", SourceView::value(sources),
    SourceView::sint(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_uint_base, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxnum &", SourceView::uint(sources),
    SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, unsigned long, sc_dt::sc_fxval>(
    "sc_dt::sc_fxval &,unsigned long,const sc_dt::sc_fxval &", Scalars::unsigned_long_integer(),
    SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, const char*, sc_dt::sc_fxnum>(
    "sc_dt::sc_fxval &,const char *,const sc_dt::sc_fxnum &", Scalars::text(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, double>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,double",
                                                           SourceView::number(sources), Scalars::real_double());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::int64>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,sc_dt::int64", SourceView::number(sources), Scalars::signed_64());
  emit_operators<sc_dt::sc_fxval, int, sc_dt::sc_fxval>("sc_dt::sc_fxval &,int,const sc_dt::sc_fxval &",
                                                        Scalars::integer(), SourceView::value(sources));
  emit_operators<sc_dt::sc_fxval, double, sc_dt::sc_fxnum>("sc_dt::sc_fxval &,double,const sc_dt::sc_fxnum &",
                                                           Scalars::real_double(), SourceView::number(sources));
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, sc_dt::uint64>(
    "sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,sc_dt::uint64", SourceView::number(sources), Scalars::unsigned_64());
  emit_operators<sc_dt::sc_fxval, sc_dt::sc_fxnum, long>("sc_dt::sc_fxval &,const sc_dt::sc_fxnum &,long",
                                                         SourceView::number(sources), Scalars::long_integer());
  emit_operators<sc_dt::sc_fxval_fast, long, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,long,const sc_dt::sc_fxnum_fast &", Scalars::long_integer(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, double>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,double", SourceView::number_fast(sources),
    Scalars::real_double());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::int64, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::int64,const sc_dt::sc_fxnum_fast &", Scalars::signed_64(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, float>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,float", SourceView::value_fast(sources),
    Scalars::real_float());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_signed, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_signed &,const sc_dt::sc_fxval_fast &", SourceView::signed_value(sources),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_uint_base &", SourceView::value_fast(sources),
    SourceView::uint(sources));
  emit_operators<sc_dt::sc_fxval_fast, double, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,double,const sc_dt::sc_fxval_fast &", Scalars::real_double(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_int_base &,const sc_dt::sc_fxnum_fast &", SourceView::sint(sources),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, float, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,float,const sc_dt::sc_fxnum_fast &", Scalars::real_float(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_unsigned &", SourceView::number_fast(sources),
    SourceView::unsigned_value(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, const char*>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const char *", SourceView::value_fast(sources),
    Scalars::text());
  emit_operators<sc_dt::sc_fxval_fast, float, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,float,const sc_dt::sc_fxval_fast &", Scalars::real_float(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, unsigned long, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,unsigned long,const sc_dt::sc_fxval_fast &", Scalars::unsigned_long_integer(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,int", SourceView::number_fast(sources), Scalars::integer());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_signed>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_signed &", SourceView::number_fast(sources),
    SourceView::signed_value(sources));
  emit_operators<sc_dt::sc_fxval_fast, const char*, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const char *,const sc_dt::sc_fxnum_fast &", Scalars::text(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::uint64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,sc_dt::uint64", SourceView::value_fast(sources),
    Scalars::unsigned_64());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::int64, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::int64,const sc_dt::sc_fxval_fast &", Scalars::signed_64(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, unsigned long, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,unsigned long,const sc_dt::sc_fxnum_fast &", Scalars::unsigned_long_integer(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &", SourceView::value_fast(sources),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::uint64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,sc_dt::uint64", SourceView::number_fast(sources),
    Scalars::unsigned_64());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxnum_fast &",
    SourceView::unsigned_value(sources), SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &", SourceView::value_fast(sources),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,long", SourceView::value_fast(sources),
    Scalars::long_integer());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxnum_fast &",
    SourceView::number_fast(sources), SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, float>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,float", SourceView::number_fast(sources),
    Scalars::real_float());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_int_base &", SourceView::number_fast(sources),
    SourceView::sint(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_unsigned>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_unsigned &", SourceView::value_fast(sources),
    SourceView::unsigned_value(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, double>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,double", SourceView::value_fast(sources),
    Scalars::real_double());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::int64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,sc_dt::int64", SourceView::number_fast(sources),
    Scalars::signed_64());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_uint_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_uint_base &", SourceView::number_fast(sources),
    SourceView::uint(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, unsigned long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,unsigned long", SourceView::number_fast(sources),
    Scalars::unsigned_long_integer());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxnum_fast &", SourceView::uint(sources),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_uint_base, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_uint_base &,const sc_dt::sc_fxval_fast &", SourceView::uint(sources),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, unsigned long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,unsigned long", SourceView::value_fast(sources),
    Scalars::unsigned_long_integer());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_unsigned, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_unsigned &,const sc_dt::sc_fxval_fast &",
    SourceView::unsigned_value(sources), SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_signed>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_signed &", SourceView::value_fast(sources),
    SourceView::signed_value(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_signed, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_signed &,const sc_dt::sc_fxnum_fast &", SourceView::signed_value(sources),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, long, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,long,const sc_dt::sc_fxval_fast &", Scalars::long_integer(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, unsigned int, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,unsigned int,const sc_dt::sc_fxval_fast &", Scalars::unsigned_integer(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::uint64, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::uint64,const sc_dt::sc_fxnum_fast &", Scalars::unsigned_64(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::uint64, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,sc_dt::uint64,const sc_dt::sc_fxval_fast &", Scalars::unsigned_64(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, const char*>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const char *", SourceView::number_fast(sources),
    Scalars::text());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,const sc_dt::sc_fxval_fast &",
    SourceView::number_fast(sources), SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_int_base, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_int_base &,const sc_dt::sc_fxval_fast &", SourceView::sint(sources),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, const char*, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,const char *,const sc_dt::sc_fxval_fast &", Scalars::text(),
    SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::sc_int_base>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,const sc_dt::sc_int_base &", SourceView::value_fast(sources),
    SourceView::sint(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, unsigned int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,unsigned int", SourceView::number_fast(sources),
    Scalars::unsigned_integer());
  emit_operators<sc_dt::sc_fxval_fast, int, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,int,const sc_dt::sc_fxnum_fast &", Scalars::integer(), SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, int, sc_dt::sc_fxval_fast>(
    "sc_dt::sc_fxval_fast &,int,const sc_dt::sc_fxval_fast &", Scalars::integer(), SourceView::value_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, unsigned int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,unsigned int", SourceView::value_fast(sources),
    Scalars::unsigned_integer());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, long>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxnum_fast &,long", SourceView::number_fast(sources),
    Scalars::long_integer());
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, int>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,int", SourceView::value_fast(sources), Scalars::integer());
  emit_operators<sc_dt::sc_fxval_fast, double, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,double,const sc_dt::sc_fxnum_fast &", Scalars::real_double(),
    SourceView::number_fast(sources));
  emit_operators<sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, sc_dt::int64>(
    "sc_dt::sc_fxval_fast &,const sc_dt::sc_fxval_fast &,sc_dt::int64", SourceView::value_fast(sources),
    Scalars::signed_64());
  emit_operators<sc_dt::sc_fxval_fast, unsigned int, sc_dt::sc_fxnum_fast>(
    "sc_dt::sc_fxval_fast &,unsigned int,const sc_dt::sc_fxnum_fast &", Scalars::unsigned_integer(),
    SourceView::number_fast(sources));
}
}  // namespace sc_dt_fxvalues_probe

int sc_main(int, char**)
{
  using namespace sc_dt_fxvalues_probe;
  Sources sources;
  sc_dt::sc_fxval value(3.5);
  sc_dt::sc_fxval_fast fast(3.5);
  std::cout << "FXOBS\tvalue\t" << observe(value) << '\n';
  std::cout << "FXOBS\tfast\t" << observe(fast) << '\n';
  const auto flatten = [](std::string text)
  {
    std::replace(text.begin(), text.end(), '\n', '~');
    return text;
  };
  std::cout << "FXFMT\tvalue\t" << flatten(Formatting::stream_value(value)) << '\n';
  std::cout << "FXFMT\tfast\t" << flatten(Formatting::stream_fast(fast)) << '\n';
  std::cout << "FXDUMP\tvalue\t" << flatten(Formatting::dump_value(value)) << '\n';
  std::cout << "FXDUMP\tfast\t" << flatten(Formatting::dump_fast(fast)) << '\n';
  const char* precise_text = "1.23456789012345678901234567890123456789";
  sc_dt::sc_fxval exact(precise_text);
  sc_dt::sc_fxval_fast rounded(precise_text);
  std::cout.precision(17);
  std::cout << "FXPREC\tnormal_vs_fast\t" << decimal(exact) << '|' << decimal(rounded) << '|'
            << Formatting::precision_value(exact) << '|' << Formatting::precision_fast(rounded) << '\n';
  emit_python_expression<sc_dt::sc_fxval>("value|python_int|arithmetic", 2);
  emit_python_expression<sc_dt::sc_fxval>("value|python_float|arithmetic", 1.25);
  emit_python_expression<sc_dt::sc_fxval>("value|fixed_value|arithmetic", sc_dt::sc_fxval_fast(0.75));
  emit_python_expression<sc_dt::sc_fxval_fast>("fast|python_int|arithmetic", 2);
  emit_python_expression<sc_dt::sc_fxval_fast>("fast|python_float|arithmetic", 1.25);
  emit_python_expression<sc_dt::sc_fxval_fast>("fast|fixed_value|arithmetic", sc_dt::sc_fxval_fast(0.75));
  run_constructor_matrix();
  {
    sc_dt::sc_fxval copy(0);
    {
      sc_dt::sc_fxval source("7.125");
      copy = source;
    }
    std::cout << "FXLIFE\tcopy_after_source_release\t" << observe(copy) << '\n';
  }
  run_assignment_matrix();
  run_compound_matrix();
  run_binary_matrix();
  run_operator_matrix();
  std::cout << "SC_DT_FXVALUES_PUBLIC_OK\n";
  return 0;
}
