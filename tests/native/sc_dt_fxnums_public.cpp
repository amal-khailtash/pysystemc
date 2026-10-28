/** @file Independent native reference for the sc_fxnum base families. */
#include <iostream>
#include <systemc>

#include "sc_dt_fxnums_probe.h"

namespace probe = sc_dt_fxnums_probe;

namespace
{
/** @brief Write one stable native observation without retaining its source. */
void emit(const std::string& kind, const std::string& family, const std::string& label, const std::string& value)
{
  std::cout << "FXN\t" << kind << '\t' << family << '\t' << label << '\t' << value << '\n';
}

/** @brief Verify returned format and cast references borrow stable owner storage. */
template <class Base>
std::string borrow_state(const Base& value)
{
  return std::to_string(&value.type_params() == &value.type_params()) + "|" +
         std::to_string(&value.cast_switch() == &value.cast_switch());
}

/** @brief Exercise the protected zero constructor through its public test facade. */
template <class Base, class Facade, class ObserverType>
void emit_ctor(const std::string& family, const std::string& label, const sc_dt::sc_fxtype_params& params,
               const sc_dt::sc_fxcast_switch& cast, ObserverType& observer)
{
  Facade value(params, sc_dt::SC_TC_, cast, &observer);
  emit("CTOR", family, label,
       probe::observe(value) + "|" + std::to_string(probe::observer_matches(value, &observer)) + "|" +
         borrow_state<Base>(value));
}

/** @brief Construct, assign, and compound with one exact native source type. */
template <class Base, class Facade, class ObserverType, class Source>
void emit_source(const std::string& family, const std::string& label, probe::parameter_t<Source> source,
                 const sc_dt::sc_fxtype_params& params, const sc_dt::sc_fxcast_switch& cast, ObserverType& observer)
{
  Facade constructed(source, params, sc_dt::SC_TC_, cast, &observer);
  emit("CTOR", family, label,
       probe::observe(constructed) + "|" + std::to_string(probe::observer_matches(constructed, &observer)) + "|" +
         borrow_state<Base>(constructed));

  Facade number(3.125, params, sc_dt::SC_TC_, cast, &observer);
  using Assign = Base& (Base::*)(probe::parameter_t<Source>);
  const Assign assign = static_cast<Assign>(&Base::operator=);
  Base& assigned = (number.*assign)(source);
  emit("ASSIGN", family, label, probe::observe(number) + "|" + std::to_string(&assigned == &number));

  using Add = Base& (Base::*)(probe::parameter_t<Source>);
  using Sub = Add;
  using Mult = Add;
  using Div = Add;
  const Add add = static_cast<Add>(&Base::operator+=);
  const Sub sub = static_cast<Sub>(&Base::operator-=);
  const Mult mult = static_cast<Mult>(&Base::operator*=);
  const Div div = static_cast<Div>(&Base::operator/=);
  const std::pair<const char*, Add> operations[] = {
    {"add", add}, {"subtract", sub}, {"multiply", mult}, {"divide", div}};
  for (const auto& operation : operations)
  {
    number = 3.125;
    Base& result = (number.*operation.second)(source);
    emit(operation.first, family, label, probe::observe(number) + "|" + std::to_string(&result == &number));
  }
}

/** @brief Run constructor, mutation, proxy, query, lifetime, and precision cases. */
template <class Base, class Facade, class ObserverType>
void exercise(const std::string& family, const sc_dt::sc_fxtype_params& params, const sc_dt::sc_fxcast_switch& cast,
              ObserverType& observer, probe::Sources& sources)
{
  emit_ctor<Base, Facade>(family, "zero", params, cast, observer);
  emit_source<Base, Facade, ObserverType, int>(family, "int", probe::Sources::integer(), params, cast, observer);
  emit_source<Base, Facade, ObserverType, unsigned int>(family, "unsigned int", probe::Sources::unsigned_integer(),
                                                        params, cast, observer);
  emit_source<Base, Facade, ObserverType, long>(family, "long", probe::Sources::long_integer(), params, cast, observer);
  emit_source<Base, Facade, ObserverType, unsigned long>(
    family, "unsigned long", probe::Sources::unsigned_long_integer(), params, cast, observer);
  emit_source<Base, Facade, ObserverType, float>(family, "float", probe::Sources::real_float(), params, cast, observer);
  emit_source<Base, Facade, ObserverType, double>(family, "double", probe::Sources::real_double(), params, cast,
                                                  observer);
  emit_source<Base, Facade, ObserverType, const char*>(family, "const char *", probe::Sources::text(), params, cast,
                                                       observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_fxval&>(family, "const sc_dt::sc_fxval &", sources.value,
                                                                  params, cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_fxval_fast&>(family, "const sc_dt::sc_fxval_fast &",
                                                                       sources.value_fast, params, cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_fxnum&>(family, "const sc_dt::sc_fxnum &", sources.number,
                                                                  params, cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_fxnum_fast&>(family, "const sc_dt::sc_fxnum_fast &",
                                                                       sources.number_fast, params, cast, observer);
  emit_source<Base, Facade, ObserverType, sc_dt::int64>(family, "sc_dt::int64", probe::Sources::signed_64(), params,
                                                        cast, observer);
  emit_source<Base, Facade, ObserverType, sc_dt::uint64>(family, "sc_dt::uint64", probe::Sources::unsigned_64(), params,
                                                         cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_int_base&>(family, "const sc_dt::sc_int_base &", sources.sint,
                                                                     params, cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_uint_base&>(family, "const sc_dt::sc_uint_base &",
                                                                      sources.uint, params, cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_signed&>(family, "const sc_dt::sc_signed &",
                                                                   sources.signed_value, params, cast, observer);
  emit_source<Base, Facade, ObserverType, const sc_dt::sc_unsigned&>(family, "const sc_dt::sc_unsigned &",
                                                                     sources.unsigned_value, params, cast, observer);

  Facade number(3.125, params, sc_dt::SC_TC_, cast, &observer);
  number = 3.125;
  ++number;
  emit("prefix", family, "increment", probe::observe(number));
  const auto old_increment = number++;
  emit("postfix", family, "increment", old_increment.to_string(sc_dt::SC_DEC) + "|" + probe::observe(number));
  --number;
  emit("prefix", family, "decrement", probe::observe(number));
  const auto old_decrement = number--;
  emit("postfix", family, "decrement", old_decrement.to_string(sc_dt::SC_DEC) + "|" + probe::observe(number));

  emit("borrow", family, "type_params_cast", borrow_state<Base>(number));
  number = 5.25;
  number.bit(0) = true;
  emit("proxy", family, "bit", number.to_string(sc_dt::SC_BIN));
  number = 5.25;
  number.range(3, 0) = sc_dt::sc_bv<4>("1010");
  emit("proxy", family, "range", number.to_string(sc_dt::SC_BIN));
  emit("format", family, "representations",
       number.to_string() + "|" + number.to_string(sc_dt::SC_BIN) + "|" + number.to_string(sc_dt::SC_OCT) + "|" +
         number.to_string(sc_dt::SC_HEX) + "|" + number.to_dec());
  std::string value_snapshot;
  {
    Facade source(7.875, params, sc_dt::SC_TC_, cast, &observer);
    const auto returned = source.value();
    value_snapshot = returned.to_string(sc_dt::SC_DEC) + "|" + returned.to_string(sc_dt::SC_BIN);
  }
  emit("value", family, "copy_after_owner", value_snapshot + "|1");
  emit("precision", family, "wide137",
       probe::observe_precision(
         Facade("9007199254740993.25", sc_dt::sc_fxtype_params(137, 69), sc_dt::SC_TC_, cast, &observer)));
  emit("precision", family, "wide191",
       probe::observe_precision(
         Facade("9007199254740995.5", sc_dt::sc_fxtype_params(191, 100), sc_dt::SC_TC_, cast, &observer)));
}
}  // namespace

/** @brief Emit independent normal and fast SystemC observations. */
int sc_main(int, char**)
{
  const sc_dt::sc_fxtype_params params(13, 5, sc_dt::SC_TRN, sc_dt::SC_WRAP);
  const sc_dt::sc_fxcast_switch cast(sc_dt::SC_OFF);
  probe::Sources sources;
  {
    probe::Observer observer;
    exercise<sc_dt::sc_fxnum, probe::Fxnum>("sc_fxnum", params, cast, observer, sources);
  }
  emit("lifetime", "sc_fxnum", "observer", std::to_string(probe::Observer::destroyed));
  {
    probe::FastObserver observer;
    exercise<sc_dt::sc_fxnum_fast, probe::FxnumFast>("sc_fxnum_fast", params, cast, observer, sources);
  }
  emit("lifetime", "sc_fxnum_fast", "observer", std::to_string(probe::FastObserver::destroyed));
  std::cout << "SC_DT_FXNUMS_PUBLIC_OK\n";
  return 0;
}
