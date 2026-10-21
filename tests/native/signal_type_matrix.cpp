/** @file signal_type_matrix.cpp
 * @brief Compare scalar native signal endpoints and floating value properties.
 * @details Native signals own all values and events. No Python callbacks or GIL
 * operations are involved, and every channel and observer is constructed before simulation.
 */
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

/** @brief Return a stable decimal token for a native integral sample. */
template <class T>
std::string integer_token(T value)
{
  std::ostringstream stream;
  if constexpr (std::is_same_v<T, bool>)
    stream << (value ? 1 : 0);
  else if constexpr (std::is_signed_v<T>)
    stream << static_cast<long long>(value);
  else
    stream << static_cast<unsigned long long>(value);
  return stream.str();
}

/** @brief Classify floating samples without platform-specific formatting. */
template <class T>
std::string float_property(T value)
{
  if (std::isnan(value)) return "nan";
  if (std::isinf(value)) return std::signbit(value) ? "-inf" : "inf";
  if (value == T{0}) return std::signbit(value) ? "-zero" : "+zero";
  return std::signbit(value) ? "finite-" : "finite+";
}

/** @brief Observe and retain each committed native signal value. */
template <class T>
struct Observer : sc_core::sc_module
{
  sc_core::sc_signal<T>* channel;
  std::vector<T> samples;
  std::vector<bool> events;
  SC_HAS_PROCESS(Observer);

  /** @brief Bind a native signal and register a noninitializing observer. */
  Observer(sc_core::sc_module_name name, sc_core::sc_signal<T>& signal) : sc_module(name), channel(&signal)
  {
    SC_METHOD(sample);
    sensitive << signal.value_changed_event();
    dont_initialize();
  }

  /** @brief Copy the committed signal value after a native value event. */
  void sample()
  {
    samples.push_back(channel->read());
    events.push_back(channel->event());
  }
};

/** @brief Retain an integer signal, observer and exact endpoint stimuli. */
template <class T>
struct IntegerCase
{
  sc_core::sc_signal<T> signal;
  Observer<T> observer;
  T low;
  T high;
  std::string name;

  /** @brief Construct native storage before the simulation starts. */
  IntegerCase(const char* label, T low_value, T high_value)
    : signal(label, low_value)
    , observer((std::string(label) + "_observer").c_str(), signal)
    , low(low_value)
    , high(high_value)
    , name(label)
  {
  }

  /** @brief Schedule the high endpoint update. */
  void write_high()
  {
    signal.write(high);
  }

  /** @brief Schedule the low endpoint update. */
  void write_low()
  {
    signal.write(low);
  }

  /** @brief Assert both callbacks and emit the normalized reference record. */
  void emit() const
  {
    if (observer.samples.size() != 2 || observer.events.size() != 2 ||
        !std::all_of(observer.events.begin(), observer.events.end(), [](bool event) { return event; }))
      throw std::runtime_error("integer signal transition or event callback count");
    std::cout << "SIGNAL_MATRIX " << name << "|" << observer.samples.size() << "|" << integer_token(observer.samples[0])
              << "," << integer_token(observer.samples[1]) << "|" << integer_token(signal.read()) << "|"
              << sc_core::sc_time_stamp() << '\n';
  }
};

/** @brief Retain floating signals for signed-zero and special-value comparisons. */
template <class T>
struct FloatingCase
{
  sc_core::sc_signal<T> signal;
  sc_core::sc_signal<T> negative_zero;
  Observer<T> observer;
  std::string name;

  /** @brief Construct both native channels and observer before simulation. */
  explicit FloatingCase(const char* label)
    : signal(label, T{0})
    , negative_zero((std::string(label) + "_negative_zero").c_str(), -T{0})
    , observer((std::string(label) + "_observer").c_str(), signal)
    , name(label)
  {
  }

  /** @brief Try a signed-zero update, which native equality suppresses. */
  void write_negative_zero()
  {
    signal.write(-T{0});
  }

  /** @brief Schedule positive infinity. */
  void write_infinity()
  {
    signal.write(std::numeric_limits<T>::infinity());
  }

  /** @brief Schedule quiet NaN. */
  void write_nan()
  {
    signal.write(std::numeric_limits<T>::quiet_NaN());
  }

  /** @brief Schedule a finite exactly representable sample. */
  void write_finite()
  {
    signal.write(T{1.25});
  }

  /** @brief Assert signed-zero equality is suppressed by native signal storage. */
  void check_zero() const
  {
    if (!observer.samples.empty() || !observer.events.empty() || std::signbit(signal.read()) ||
        !std::signbit(negative_zero.read()))
      throw std::runtime_error("signed-zero native signal semantics");
  }

  /** @brief Assert infinity, NaN and finite samples were observed in order. */
  void emit() const
  {
    if (observer.samples.size() != 3 || observer.events.size() != 3 ||
        !std::all_of(observer.events.begin(), observer.events.end(), [](bool event) { return event; }) ||
        !std::isinf(observer.samples[0]) || !std::isnan(observer.samples[1]) || observer.samples[2] != T{1.25} ||
        signal.read() != T{1.25})
    {
      std::ostringstream details;
      details << "floating signal value properties: count=" << observer.samples.size()
              << " final=" << float_property(signal.read());
      for (const auto& sample : observer.samples) details << " sample=" << float_property(sample);
      throw std::runtime_error(details.str());
    }
  }

  /** @brief Emit the floating reference line after value assertions. */
  void print() const
  {
    std::cout << "SIGNAL_FLOAT " << name << "|" << float_property(observer.samples[0]) << ","
              << float_property(observer.samples[1]) << "," << float_property(observer.samples[2]) << "|"
              << float_property(signal.read()) << "|" << float_property(T{0}) << "|"
              << float_property(negative_zero.read()) << "|" << sc_core::sc_time_stamp() << '\n';
  }
};

/** @brief Run the native reference for the assigned scalar type cases. */
int sc_main(int, char**)
{
  IntegerCase<bool> boolean("bool", false, true);
  IntegerCase<short> signed_short("short", std::numeric_limits<short>::lowest(), std::numeric_limits<short>::max());
  IntegerCase<unsigned short> unsigned_short("unsigned_short", 0, std::numeric_limits<unsigned short>::max());
  IntegerCase<long> signed_long("long", std::numeric_limits<long>::lowest(), std::numeric_limits<long>::max());
  IntegerCase<unsigned long> unsigned_long("unsigned_long", 0, std::numeric_limits<unsigned long>::max());
  IntegerCase<int> signed_int("int", std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
  IntegerCase<unsigned int> unsigned_int("unsigned_int", std::numeric_limits<unsigned int>::lowest(),
                                         std::numeric_limits<unsigned int>::max());
  IntegerCase<long long> signed_long_long("long_long", std::numeric_limits<long long>::lowest(),
                                          std::numeric_limits<long long>::max());
  IntegerCase<unsigned long long> unsigned_long_long("unsigned_long_long",
                                                     std::numeric_limits<unsigned long long>::lowest(),
                                                     std::numeric_limits<unsigned long long>::max());
  FloatingCase<float> single("float");
  FloatingCase<double> real("double");
  FloatingCase<long double> extended("long_double");

  boolean.write_high();
  signed_short.write_high();
  unsigned_short.write_high();
  signed_long.write_high();
  unsigned_long.write_high();
  signed_int.write_high();
  unsigned_int.write_high();
  signed_long_long.write_high();
  unsigned_long_long.write_high();
  single.write_negative_zero();
  real.write_negative_zero();
  extended.write_negative_zero();
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  single.check_zero();
  real.check_zero();
  extended.check_zero();
  boolean.write_low();
  signed_short.write_low();
  unsigned_short.write_low();
  signed_long.write_low();
  unsigned_long.write_low();
  signed_int.write_low();
  unsigned_int.write_low();
  signed_long_long.write_low();
  unsigned_long_long.write_low();
  single.write_infinity();
  real.write_infinity();
  extended.write_infinity();
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  single.write_nan();
  real.write_nan();
  extended.write_nan();
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  single.write_finite();
  real.write_finite();
  extended.write_finite();
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  sc_core::sc_start(sc_core::SC_ZERO_TIME);

  boolean.emit();
  signed_short.emit();
  unsigned_short.emit();
  signed_long.emit();
  unsigned_long.emit();
  signed_int.emit();
  unsigned_int.emit();
  signed_long_long.emit();
  unsigned_long_long.emit();
  single.emit();
  single.print();
  real.emit();
  real.print();
  extended.emit();
  extended.print();
  sc_core::sc_stop();
  return 0;
}
