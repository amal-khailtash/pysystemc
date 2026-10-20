/** @file master_slave_type_matrix.cpp
 *  @brief Independent native reference for asymmetric master/slave value overloads.
 *  @details The model owns its native interfaces, queues, and events through simulation.
 *  It uses no Python callbacks or GIL operations; caller outputs remain local to the
 *  reference process, and every blocking callback waits for one nanosecond.
 */
#include <deque>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>
#include <type_traits>

/** @brief Enforce checks independently of NDEBUG. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Construct the first and second exact samples for one native type. */
template <class T>
T sample(int index)
{
  if constexpr (std::is_same_v<T, int>)
    return index == 0 ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
  else if constexpr (std::is_same_v<T, long long>)
    return index == 0 ? std::numeric_limits<long long>::min() : std::numeric_limits<long long>::max();
  else if constexpr (std::is_same_v<T, unsigned long long>)
    return std::numeric_limits<unsigned long long>::max() - static_cast<unsigned long long>(index);
  else if constexpr (std::is_same_v<T, long double>)
    return index == 0 ? 1.25L : -2.5L;
  else if constexpr (std::is_same_v<T, sc_dt::sc_biguint<137>>)
  {
    T value = 17 + 12 * index;
    value[136] = true;
    return value;
  }
  else if constexpr (std::is_same_v<T, sc_dt::sc_bigint<128>>)
  {
    T value = 0;
    if (index == 0)
      value[127] = true;
    else
      for (int bit = 0; bit < 127; ++bit) value[bit] = true;
    return value;
  }
}

/** @brief Own a compound TLM interface, response queue, and readiness events.
 *  @details All callback state belongs to this native object and outlives its process.
 *  Blocking callbacks use SystemC wait on the simulation thread. There are no Python
 *  references, callbacks, exceptions crossing a language boundary, or GIL requirements.
 */
template <class Base, class Put, class Get>
class NativeOwner : public Base
{
 public:
  std::deque<Get> values;
  Put sent{};
  bool has_sent = false;
  int calls = 0;
  sc_core::sc_event put_ready, get_ready, peek_ready;

  /** @brief Copy the request/response before the callback's one-nanosecond wait ends. */
  void put(const Put& value) override
  {
    ++calls;
    wait(1, sc_core::SC_NS);
    sent = value;
    has_sent = true;
  }

  /** @brief Consume and return a queued value after one nanosecond. */
  Get get(tlm::tlm_tag<Get>* = nullptr) override
  {
    ++calls;
    wait(1, sc_core::SC_NS);
    verify(!values.empty(), "blocking get from empty native response queue");
    Get result = values.front();
    values.pop_front();
    return result;
  }

  /** @brief Return the queued value without consuming it after one nanosecond. */
  Get peek(tlm::tlm_tag<Get>* = nullptr) const override
  {
    auto* self = const_cast<NativeOwner*>(this);
    ++self->calls;
    wait(1, sc_core::SC_NS);
    verify(!values.empty(), "blocking peek from empty native response queue");
    return values.front();
  }

  /** @brief Copy a nonblocking put request; unused by the selected assertion path. */
  bool nb_put(const Put& value) override
  {
    ++calls;
    sent = value;
    has_sent = true;
    return true;
  }

  /** @brief Report native nonblocking put capacity. */
  bool nb_can_put(tlm::tlm_tag<Put>* = nullptr) const override
  {
    ++const_cast<NativeOwner*>(this)->calls;
    return true;
  }

  /** @brief Return the owned native put readiness event. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<Put>* = nullptr) const override
  {
    return put_ready;
  }

  /** @brief Copy a successful sample to output without changing it on failure. */
  bool nb_get(Get& output) override
  {
    ++calls;
    if (values.empty()) return false;
    output = values.front();
    values.pop_front();
    return true;
  }

  /** @brief Report whether a nonblocking get can succeed. */
  bool nb_can_get(tlm::tlm_tag<Get>* = nullptr) const override
  {
    ++const_cast<NativeOwner*>(this)->calls;
    return !values.empty();
  }

  /** @brief Return the owned native get readiness event. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<Get>* = nullptr) const override
  {
    return get_ready;
  }

  /** @brief Copy a successful peek sample while preserving the queue. */
  bool nb_peek(Get& output) const override
  {
    ++const_cast<NativeOwner*>(this)->calls;
    if (values.empty()) return false;
    output = values.front();
    return true;
  }

  /** @brief Report whether a nonblocking peek can succeed. */
  bool nb_can_peek(tlm::tlm_tag<Get>* = nullptr) const override
  {
    ++const_cast<NativeOwner*>(this)->calls;
    return !values.empty();
  }

  /** @brief Return the owned native peek readiness event. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<Get>* = nullptr) const override
  {
    return peek_ready;
  }
};

/** @brief Check native blocking overloads and optionally the full-interface NB paths. */
template <bool Full, class Interface, class Owner, class Put, class Get>
void run_case(Interface& channel, Owner& owner, const std::string& label, const char* direction, const char* family)
{
  owner.values = {sample<Get>(0), sample<Get>(1)};
  Put source = sample<Put>(0);
  const Put expected_put = source;
  channel.put(source);
  source = Put{};
  verify(owner.has_sent && owner.sent == expected_put, "blocking put did not preserve its copied value");

  const Get first = sample<Get>(0);
  const Get second = sample<Get>(1);
  verify(channel.peek() == first, "blocking peek value overload mismatch");
  Get peek_output{};
  channel.peek(peek_output);
  verify(peek_output == first, "blocking peek output overload mismatch");
  verify(channel.get() == first, "blocking get value overload mismatch");
  Get get_output{};
  channel.get(get_output);
  verify(get_output == second, "blocking get output overload mismatch");

  if constexpr (Full)
  {
    owner.values = {first, second};
    verify(channel.nb_can_get() && channel.nb_can_peek(), "full interface missed ready samples");
    Get output{};
    verify(channel.nb_peek(output) && output == first, "successful nonblocking peek mismatch");
    verify(channel.nb_get(output) && output == first, "successful nonblocking get mismatch");
    verify(channel.nb_peek(output) && output == second, "second nonblocking peek mismatch");
    verify(channel.nb_get(output) && output == second, "second nonblocking get mismatch");
    verify(!channel.nb_can_get() && !channel.nb_can_peek(), "empty full interface still reports ready");
    output = sample<Get>(0);
    const Get sentinel = output;
    verify(!channel.nb_get(output) && output == sentinel, "failed nonblocking get changed caller output");
    verify(!channel.nb_peek(output) && output == sentinel, "failed nonblocking peek changed caller output");
  }

  const int expected_calls = Full ? 15 : 5;
  verify(owner.calls == expected_calls, "native callback count mismatch");
  std::cout << "MASTER_SLAVE_CASE " << label << ' ' << direction << ' ' << family << ' ' << owner.calls << ' '
            << sc_core::sc_time_stamp() << '\n';
}

/** @brief Run master-facing cases for the blocking and full native interface views. */
template <class Req, class Rsp>
void run_master(const char* type)
{
  using Full = tlm::tlm_master_if<Req, Rsp>;
  NativeOwner<Full, Req, Rsp> owner;
  run_case<true, tlm::tlm_master_if<Req, Rsp>, NativeOwner<Full, Req, Rsp>, Req, Rsp>(static_cast<Full&>(owner), owner,
                                                                                      type, "master", "combined");
  owner.calls = 0;
  owner.has_sent = false;
  run_case<false, tlm::tlm_blocking_master_if<Req, Rsp>, NativeOwner<Full, Req, Rsp>, Req, Rsp>(
    static_cast<tlm::tlm_blocking_master_if<Req, Rsp>&>(owner), owner, type, "master", "blocking");
}

/** @brief Run slave-facing cases with the native inverse request/response direction. */
template <class Req, class Rsp>
void run_slave(const char* type)
{
  using Full = tlm::tlm_slave_if<Req, Rsp>;
  NativeOwner<Full, Rsp, Req> owner;
  run_case<true, tlm::tlm_slave_if<Req, Rsp>, NativeOwner<Full, Rsp, Req>, Rsp, Req>(static_cast<Full&>(owner), owner,
                                                                                     type, "slave", "combined");
  owner.calls = 0;
  owner.has_sent = false;
  run_case<false, tlm::tlm_blocking_slave_if<Req, Rsp>, NativeOwner<Full, Rsp, Req>, Rsp, Req>(
    static_cast<tlm::tlm_blocking_slave_if<Req, Rsp>&>(owner), owner, type, "slave", "blocking");
}

/** @brief Schedule all native reference cases in one independent kernel. */
struct Matrix : sc_core::sc_module
{
  SC_HAS_PROCESS(Matrix);

  /** @brief Start the case matrix in one SystemC process. */
  explicit Matrix(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_THREAD(run);
  }

  /** @brief Exercise signed endpoints, unsigned endpoints, and nested wide types. */
  void run()
  {
    run_master<int, long long>("int_long_long");
    run_slave<int, long long>("int_long_long");
    run_master<unsigned long long, long double>("unsigned_long_long_long_double");
    run_slave<unsigned long long, long double>("unsigned_long_long_long_double");
    run_master<sc_dt::sc_biguint<137>, sc_dt::sc_bigint<128>>("biguint137_bigint128");
    run_slave<sc_dt::sc_biguint<137>, sc_dt::sc_bigint<128>>("biguint137_bigint128");
  }
};

/** @brief Run the reference and assert its exact accumulated simulation time. */
int sc_main(int, char**)
{
  Matrix matrix("matrix");
  sc_core::sc_start(61, sc_core::SC_NS);
  verify(sc_core::sc_time_stamp() == sc_core::sc_time(61, sc_core::SC_NS), "native matrix time mismatch");
  std::cout << "MASTER_SLAVE_TYPE_MATRIX_NATIVE_OK\n";
  return 0;
}
