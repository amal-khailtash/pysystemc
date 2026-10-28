/** @file Non-default-constructible FIFO value and structural reader fixture. */
#pragma once

#include <sysc/communication/sc_fifo_ifs.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace sc_core_fifo_nondefault
{
/** @brief A FIFO value with copy/assignment but no default constructor. */
struct Value
{
  int value;

  Value() = delete;
  explicit Value(int input) : value(input)
  {
  }
  Value(const Value& other) : value(other.value)
  {
  }
  Value& operator=(const Value& other)
  {
    value = other.value;
    return *this;
  }
  Value& __assign__(const Value& other)
  {
    return *this = other;
  }
};

static_assert(!std::is_default_constructible_v<Value>);
static_assert(std::is_copy_constructible_v<Value>);
static_assert(std::is_copy_assignable_v<Value>);

/** @brief Structural native blocking reader with value and output overloads. */
class Reader : public sc_core::sc_fifo_blocking_in_if<Value>
{
 public:
  Reader() : values_{Value(17), Value(23)}
  {
  }

  void read(Value& output) override
  {
    ++output_read_calls_;
    output = values_.at(next_++);
  }

  Value read() override
  {
    ++value_read_calls_;
    return values_.at(next_++);
  }

  /** @brief Return the number of native value-return reads. */
  std::size_t value_read_calls() const
  {
    return value_read_calls_;
  }

  /** @brief Return the number of native output-parameter reads. */
  std::size_t output_read_calls() const
  {
    return output_read_calls_;
  }

 private:
  std::vector<Value> values_;
  std::size_t next_ = 0;
  std::size_t value_read_calls_ = 0;
  std::size_t output_read_calls_ = 0;
};

/** @brief Return the adjusted borrowed blocking-input interface address. */
inline std::uintptr_t blocking_input_address(Reader& reader)
{
  auto* interface = static_cast<sc_core::sc_fifo_blocking_in_if<Value>*>(&reader);
  return reinterpret_cast<std::uintptr_t>(interface);
}
}  // namespace sc_core_fifo_nondefault
