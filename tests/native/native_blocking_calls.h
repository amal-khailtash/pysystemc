/** @file native_blocking_calls.h
 * @brief Cached native wait and foreign channel reference fixtures.
 * @details Functors borrow events/channels/output storage through suspension. All calls
 * execute in C++ without Python/GIL state. Interrupted writes preserve caller storage.
 */
#pragma once
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <memory>
#include <stdexcept>
#include <systemc>
#include <vector>
namespace blocking_fixture
{
using namespace sc_core;
/** @brief Capture the original native static overload before Python guard installation. */
inline auto cached_static = static_cast<void (*)(sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native count overload before Python guard installation. */
inline auto cached_count = static_cast<void (*)(int, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native time overload before Python guard installation. */
inline auto cached_time = static_cast<void (*)(const sc_time&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native units overload before Python guard installation. */
inline auto cached_units = static_cast<void (*)(double, sc_time_unit, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native event overload before Python guard installation. */
inline auto cached_event = static_cast<void (*)(const sc_event&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native or overload before Python guard installation. */
inline auto cached_or = static_cast<void (*)(const sc_event_or_list&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native and overload before Python guard installation. */
inline auto cached_and = static_cast<void (*)(const sc_event_and_list&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native time_event overload before Python guard installation. */
inline auto cached_time_event = static_cast<void (*)(const sc_time&, const sc_event&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native time_or overload before Python guard installation. */
inline auto cached_time_or =
  static_cast<void (*)(const sc_time&, const sc_event_or_list&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native time_and overload before Python guard installation. */
inline auto cached_time_and =
  static_cast<void (*)(const sc_time&, const sc_event_and_list&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native units_event overload before Python guard installation. */
inline auto cached_units_event =
  static_cast<void (*)(double, sc_time_unit, const sc_event&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native units_or overload before Python guard installation. */
inline auto cached_units_or =
  static_cast<void (*)(double, sc_time_unit, const sc_event_or_list&, sc_simcontext*)>(&sc_core::wait);
/** @brief Capture the original native units_and overload before Python guard installation. */
inline auto cached_units_and =
  static_cast<void (*)(double, sc_time_unit, const sc_event_and_list&, sc_simcontext*)>(&sc_core::wait);
/** @brief Borrow targets and invoke one captured native function pointer. */
struct CachedWait
{
  int kind;
  bool normal;
  sc_event& first;
  sc_event& second;
  sc_simcontext* context;
  /** @brief Retain borrowed native targets for a single callable's lifetime. */
  CachedWait(int kind, bool normal, sc_event& first, sc_event& second, sc_simcontext* context)
    : kind(kind), normal(normal), first(first), second(second), context(context)
  {
  }
  /** @brief Execute one wait, allowing native scheduler unwind to escape to the caller's guard. */
  void operator()() const
  {
    sc_time duration(normal ? 3 : 100, SC_NS);
    sc_event_or_list either = first | second;
    sc_event_and_list both = first & second;
    switch (kind)
    {
    case 0:
      cached_static(context);
      break;
    case 1:
      cached_count(2, context);
      break;
    case 2:
      cached_time(duration, context);
      break;
    case 3:
      cached_units(normal ? 3 : 100, SC_NS, context);
      break;
    case 4:
      cached_event(first, context);
      break;
    case 5:
      cached_or(either, context);
      break;
    case 6:
      cached_and(both, context);
      break;
    case 7:
      cached_time_event(duration, first, context);
      break;
    case 8:
      cached_time_or(duration, either, context);
      break;
    case 9:
      cached_time_and(duration, both, context);
      break;
    case 10:
      cached_units_event(100, SC_NS, first, context);
      break;
    case 11:
      cached_units_or(100, SC_NS, either, context);
      break;
    case 12:
      cached_units_and(100, SC_NS, both, context);
      break;
    default:
      throw std::invalid_argument("wait kind");
    }
  }
};
/** @brief Native third-party blocking method, deliberately outside installed channel pythonizations. */
struct ForeignBase : sc_prim_channel
{
  sc_event ready;
  explicit ForeignBase(const char* name) : sc_prim_channel(name)
  {
  }
  /** @brief Block on a native event, retaining caller output until completion. */
  int acquire(int& output, bool fail)
  {
    sc_core::wait(ready);
    if (fail) throw std::runtime_error("foreign operation failure");
    output = 73;
    return 41;
  }
};
/** @brief Inherit an arbitrary native blocking method without any Python method guard. */
struct ForeignChannel : ForeignBase
{
  explicit ForeignChannel(const char* name) : ForeignBase(name)
  {
  }
};
/** @brief Call the borrowed foreign method entirely in C++. */
struct ForeignOperation
{
  ForeignBase& channel;
  int& output;
  bool fail;
  /** @brief Borrow channel/output storage; the Python caller owns both through invocation. */
  ForeignOperation(ForeignBase& channel, int& output, bool fail) : channel(channel), output(output), fail(fail)
  {
  }
  /** @brief Borrow a stable element in caller-owned native output storage. */
  ForeignOperation(ForeignBase& channel, std::vector<int>& output, bool fail)
    : channel(channel), output(output.at(0)), fail(fail)
  {
  }
  int operator()() const
  {
    return channel.acquire(output, fail);
  }
};
/** @brief Copyable native result that explicitly forbids moving and assignment. */
struct CopyOnlyResult
{
  int value;
  explicit CopyOnlyResult(int value) : value(value)
  {
  }
  CopyOnlyResult(const CopyOnlyResult&) = default;
  CopyOnlyResult(CopyOnlyResult&&) = delete;
  CopyOnlyResult& operator=(const CopyOnlyResult&) = delete;
};
/** @brief Return a value after blocking; C++17 initializes the result without moving. */
struct CopyOnlyOperation
{
  ForeignBase& channel;
  explicit CopyOnlyOperation(ForeignBase& channel) : channel(channel)
  {
  }
  CopyOnlyResult operator()() const
  {
    sc_core::wait(channel.ready);
    return CopyOnlyResult(73);
  }
};
/** @brief Count invocation of invalid native return contracts; rejection must precede work. */
inline int rejected_calls = 0;
/** @brief Deliberately omit a const call operator for signature rejection. */
struct NonConstOperation
{
  void operator()()
  {
    ++rejected_calls;
  }
};
/** @brief Deliberately return borrowed pointer storage for a precondition test. */
struct PointerOperation
{
  int* operator()() const
  {
    ++rejected_calls;
    return nullptr;
  }
};
/** @brief Deliberately return borrowed reference storage for a precondition test. */
struct ReferenceOperation
{
  int& operator()() const
  {
    ++rejected_calls;
    return rejected_calls;
  }
};
/** @brief Deliberately return move-only storage for a precondition test. */
struct MoveOnlyOperation
{
  std::unique_ptr<int> operator()() const
  {
    ++rejected_calls;
    return std::make_unique<int>(17);
  }
};
}  // namespace blocking_fixture
