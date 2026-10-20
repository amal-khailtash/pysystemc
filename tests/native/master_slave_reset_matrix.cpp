/** @file master_slave_reset_matrix.cpp
 *  @brief Independent native master/slave callback unwind reference matrix.
 *  @details Each process owns its empty callback endpoint and wait events through
 *  simulation. Caller outputs are local native values; output wrappers verify the
 *  sentinel before rethrowing SystemC unwind. No Python callbacks or GIL operations
 *  are involved.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <memory>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>
#include <type_traits>
#include <vector>

/** @brief Assert native reference behavior regardless of NDEBUG. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

enum class wait_kind
{
  time,
  event,
  event_or,
  event_and,
  timed_event,
  timed_or,
  timed_and,
  cycle,
  count
};

enum class operation
{
  put,
  get,
  peek,
  get_output,
  peek_output
};

enum class family
{
  blocking_master,
  full_master,
  blocking_slave,
  full_slave
};

const char* name(wait_kind value)
{
  switch (value)
  {
  case wait_kind::time:
    return "time";
  case wait_kind::event:
    return "event";
  case wait_kind::event_or:
    return "or";
  case wait_kind::event_and:
    return "and";
  case wait_kind::timed_event:
    return "timed-event";
  case wait_kind::timed_or:
    return "timed-or";
  case wait_kind::timed_and:
    return "timed-and";
  case wait_kind::cycle:
    return "cycle";
  case wait_kind::count:
    return "count";
  }
  throw std::runtime_error("unknown wait kind");
}

const char* name(operation value)
{
  switch (value)
  {
  case operation::put:
    return "put";
  case operation::get:
    return "get";
  case operation::peek:
    return "peek";
  case operation::get_output:
    return "get-output";
  case operation::peek_output:
    return "peek-output";
  }
  throw std::runtime_error("unknown operation");
}

const char* name(family value)
{
  switch (value)
  {
  case family::blocking_master:
    return "blocking-master";
  case family::full_master:
    return "full-master";
  case family::blocking_slave:
    return "blocking-slave";
  case family::full_slave:
    return "full-slave";
  }
  throw std::runtime_error("unknown family");
}

/** @brief Retain ordered lifecycle events for exact simulation-time checks. */
struct record
{
  std::string event;
  sc_core::sc_time time;
};

/** @brief Produce the exact request value used by the matrix. */
template <class T>
T sample()
{
  T value = 73;
  if constexpr (std::is_same_v<T, sc_dt::sc_biguint<137>>) value[136] = true;
  return value;
}

/** @brief Own an empty native compound endpoint and its wait events.
 *  @details The owner is retained by its case until all spawned processes terminate.
 *  Callback code runs only on the SystemC simulation thread and calls native wait;
 *  no Python references, GIL acquisition, or cross-language exceptions are used.
 */
template <class T>
class native_endpoint : public tlm::tlm_master_if<T, T>, public tlm::tlm_slave_if<T, T>
{
 public:
  using value_pair = std::pair<T, bool>;
  sc_core::sc_event first_event;
  sc_core::sc_event second_event;
  mutable int callback_calls = 0;
  int output_checks = 0;
  std::vector<record>* lifecycle = nullptr;
  wait_kind kind;

  /** @brief Bind this endpoint to one selected wait form. */
  explicit native_endpoint(wait_kind selected) : kind(selected)
  {
  }

  /** @brief Record an event at the current simulation timestamp. */
  void log(const char* event)
  {
    if (!lifecycle) throw std::runtime_error("native lifecycle record owner is missing");
    lifecycle->push_back({event, sc_core::sc_time_stamp()});
  }

  /** @brief Execute one selected wait and validate scheduler unwind kind and time. */
  void block(const char* operation_name)
  {
    ++callback_calls;
    log("callback-start");
    try
    {
      switch (kind)
      {
      case wait_kind::time:
        sc_core::wait(100, sc_core::SC_NS);
        break;
      case wait_kind::event:
        sc_core::wait(first_event);
        break;
      case wait_kind::event_or:
        sc_core::wait(first_event | second_event);
        break;
      case wait_kind::event_and:
        sc_core::wait(first_event & second_event);
        break;
      case wait_kind::timed_event:
        sc_core::wait(100, sc_core::SC_NS, first_event);
        break;
      case wait_kind::timed_or:
        sc_core::wait(100, sc_core::SC_NS, first_event | second_event);
        break;
      case wait_kind::timed_and:
        sc_core::wait(100, sc_core::SC_NS, first_event & second_event);
        break;
      case wait_kind::cycle:
        sc_core::wait();
        break;
      case wait_kind::count:
        sc_core::wait(100);
        break;
      }
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      const bool resetting = callback_calls == 1;
      verify(unwind.is_reset() == resetting, "callback unwind kind mismatch");
      verify(sc_core::sc_time_stamp() == sc_core::sc_time(resetting ? 2 : 3, sc_core::SC_NS),
             "callback unwind time mismatch");
      log("callback-cleanup");
      throw;
    }
    throw std::runtime_error(operation_name);
  }

  /** @brief Block in the native request callback after checking the copied value. */
  void put(const T& value) override
  {
    verify(value == sample<T>(), "native put value mismatch");
    block("put returned");
  }

  /** @brief Block in a native response get and return a stable sample on success. */
  T get(tlm::tlm_tag<T>* = nullptr) override
  {
    block("get returned");
    return sample<T>();
  }

  /** @brief Block in a native response peek and return a stable sample on success. */
  T peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    const_cast<native_endpoint*>(this)->block("peek returned");
    return sample<T>();
  }

  /** @brief Reject unused full-interface nonblocking puts. */
  bool nb_put(const T&) override
  {
    return false;
  }

  /** @brief Report no unused put capacity. */
  bool nb_can_put(tlm::tlm_tag<T>* = nullptr) const override
  {
    return false;
  }

  /** @brief Return the endpoint-owned put readiness event. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<T>* = nullptr) const override
  {
    return first_event;
  }

  /** @brief Reject unused full-interface nonblocking gets. */
  bool nb_get(T&) override
  {
    return false;
  }

  /** @brief Report no unused get value. */
  bool nb_can_get(tlm::tlm_tag<T>* = nullptr) const override
  {
    return false;
  }

  /** @brief Return the endpoint-owned get readiness event. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<T>* = nullptr) const override
  {
    return first_event;
  }

  /** @brief Reject unused full-interface nonblocking peeks. */
  bool nb_peek(T&) const override
  {
    return false;
  }

  /** @brief Report no unused peek value. */
  bool nb_can_peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    return false;
  }

  /** @brief Return the endpoint-owned peek readiness event. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    return second_event;
  }
};

/** @brief Compare interface caller-output behavior before propagating unwind. */
template <class T, class Channel>
void call_output(Channel& channel, operation selected, native_endpoint<T>& endpoint)
{
  T output = 73;
  const T sentinel = output;
  try
  {
    if (selected == operation::get_output)
      channel.get(output);
    else
      channel.peek(output);
    throw std::runtime_error("empty native blocking output returned");
  }
  catch (const sc_core::sc_unwind_exception& unwind)
  {
    const bool resetting = endpoint.callback_calls == 1;
    verify(output == sentinel, "native scheduler unwind changed caller output");
    verify(unwind.is_reset() == resetting, "native output unwind kind mismatch");
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(resetting ? 2 : 3, sc_core::SC_NS),
           "native output unwind time mismatch");
    ++endpoint.output_checks;
    throw;
  }
}

/** @brief Shared type-erased lifecycle controls for a matrix case. */
class case_control
{
 public:
  virtual ~case_control() = default;
  virtual sc_core::sc_process_handle& handle() = 0;
  virtual void verify() const = 0;
  virtual void print() const = 0;
};

/** @brief Own one native type/family/path/wait case and its dynamic process. */
template <class T>
class matrix_case final : public case_control
{
 public:
  std::string type_label;
  family interface_family;
  operation path;
  wait_kind wait_form;
  native_endpoint<T> endpoint;
  sc_core::sc_process_handle process;
  std::vector<record> lifecycle;

  /** @brief Register a dynamically spawned worker sensitive to the matrix clock. */
  matrix_case(const char* type, family selected_family, operation selected_path, wait_kind selected_wait,
              const sc_core::sc_event& clock_event)
    : type_label(type)
    , interface_family(selected_family)
    , path(selected_path)
    , wait_form(selected_wait)
    , endpoint(selected_wait)
  {
    endpoint.lifecycle = &lifecycle;
    sc_core::sc_spawn_options options;
    options.set_sensitivity(&clock_event);
    const std::string process_name =
      type_label + "_" + name(interface_family) + "_" + name(path) + "_" + name(wait_form);
    process = sc_core::sc_spawn([this] { worker(); }, process_name.c_str(), &options);
  }

  /** @brief Expose this process handle to the controller. */
  sc_core::sc_process_handle& handle() override
  {
    return process;
  }

  /** @brief Invoke the selected blocking method and record outer cleanup on unwind. */
  void worker()
  {
    lifecycle.push_back({"worker-start", sc_core::sc_time_stamp()});
    try
    {
      const bool master = interface_family == family::blocking_master || interface_family == family::full_master;
      const bool combined = interface_family == family::full_master || interface_family == family::full_slave;
      if (master && combined)
      {
        invoke(static_cast<tlm::tlm_master_if<T, T>&>(endpoint));
      }
      else if (!master && combined)
      {
        invoke(static_cast<tlm::tlm_slave_if<T, T>&>(endpoint));
      }
      else if (master)
      {
        invoke(static_cast<tlm::tlm_blocking_master_if<T, T>&>(endpoint));
      }
      else
        invoke(static_cast<tlm::tlm_blocking_slave_if<T, T>&>(endpoint));
      throw std::runtime_error("blocked native method returned");
    }
    catch (const sc_core::sc_unwind_exception&)
    {
      lifecycle.push_back({"worker-cleanup", sc_core::sc_time_stamp()});
      throw;
    }
  }

  /** @brief Invoke one blocking operation through the requested native interface view. */
  template <class Channel>
  void invoke(Channel& channel)
  {
    if (path == operation::put)
      channel.put(sample<T>());
    else if (path == operation::get)
      static_cast<void>(channel.get());
    else if (path == operation::peek)
      static_cast<void>(channel.peek());
    else
      call_output<T>(channel, path, endpoint);
  }

  /** @brief Assert exact reset/kill callback and process ordering, time, and counts. */
  void verify() const override
  {
    const std::vector<record> expected = {
      {"worker-start", sc_core::sc_time(0, sc_core::SC_NS)},
      {"callback-start", sc_core::sc_time(0, sc_core::SC_NS)},
      {"callback-cleanup", sc_core::sc_time(2, sc_core::SC_NS)},
      {"worker-cleanup", sc_core::sc_time(2, sc_core::SC_NS)},
      {"worker-start", sc_core::sc_time(2, sc_core::SC_NS)},
      {"callback-start", sc_core::sc_time(2, sc_core::SC_NS)},
      {"callback-cleanup", sc_core::sc_time(3, sc_core::SC_NS)},
      {"worker-cleanup", sc_core::sc_time(3, sc_core::SC_NS)},
    };
    ::verify(lifecycle.size() == expected.size(), "native lifecycle record count mismatch");
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
      ::verify(lifecycle[i].event == expected[i].event && lifecycle[i].time == expected[i].time,
               "native lifecycle order/time mismatch");
    }
    ::verify(endpoint.callback_calls == 2, "native callback count mismatch");
    const int output_expected = path == operation::get_output || path == operation::peek_output ? 2 : 0;
    ::verify(endpoint.output_checks == output_expected, "native output unwind check count mismatch");
    ::verify(process.terminated(), "native worker remains alive after kill");
  }

  /** @brief Print the normalized case record compared with the Python matrix. */
  void print() const override
  {
    std::cout << "MASTER_SLAVE_RESET_CASE " << type_label << ' ' << name(interface_family) << ' ' << name(path) << ' '
              << name(wait_form) << " 2 2 ns 3 ns\n";
  }
};

/** @brief Parse one wait-kind slice while retaining every type/family/path case. */
struct matrix : sc_core::sc_module
{
  SC_HAS_PROCESS(matrix);
  sc_core::sc_clock clock;
  std::vector<std::unique_ptr<case_control>> cases;

  /** @brief Construct the selected independent cases and start the scheduler controller. */
  explicit matrix(sc_core::sc_module_name module_name)
    : sc_module(module_name)
    , clock("matrix_clock", sc_core::sc_time(100, sc_core::SC_NS), 0.5, sc_core::sc_time(50, sc_core::SC_NS), true)
  {
    build_type<int>("int");
    build_type<long long>("long_long");
    build_type<long double>("long_double");
    build_type<sc_dt::sc_biguint<137>>("biguint137");
    SC_THREAD(control);
  }

  /** @brief Add output get/peek event waits for every interface family of one type. */
  template <class T>
  void build_type(const char* type_label)
  {
    const family families[] = {family::blocking_master, family::full_master, family::blocking_slave,
                               family::full_slave};
    const operation paths[] = {operation::get_output, operation::peek_output};
    for (family selected_family : families)
      for (operation selected_path : paths)
        cases.emplace_back(std::make_unique<matrix_case<T>>(type_label, selected_family, selected_path,
                                                            wait_kind::event, clock.posedge_event()));
  }

  /** @brief Reset each blocked process at 2 ns, kill each restart at 3 ns, and verify. */
  void control()
  {
    sc_core::wait(2, sc_core::SC_NS);
    for (auto& item : cases) item->handle().reset();
    sc_core::wait(1, sc_core::SC_NS);
    for (auto& item : cases) item->handle().kill();
    sc_core::wait(1, sc_core::SC_NS);
    for (const auto& item : cases) item->verify();
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(4, sc_core::SC_NS), "native final time mismatch");
    for (const auto& item : cases) item->print();
    std::cout << "MASTER_SLAVE_RESET_MATRIX_NATIVE_OK " << cases.size() << '\n';
    sc_core::sc_stop();
  }
};

/** @brief Run one bounded wait-form slice in an independent native kernel. */
int sc_main(int, char**)
{
  matrix model("matrix");
  sc_core::sc_start(5, sc_core::SC_NS);
  verify(sc_core::sc_time_stamp() == sc_core::sc_time(4, sc_core::SC_NS), "native matrix stopped at wrong time");
  return 0;
}
