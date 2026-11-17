/** @file event_finder_error_edges.cpp
 * @brief Reference event finder specialization, cache, and callback behavior.
 *
 * The finder borrows its port and the port borrows the interface. The module
 * owns both until after simulation and deletes the cached finder on teardown.
 */
#include <iostream>
#include <systemc>

namespace event_finder_error_edges
{
/** @brief Interface exposing an inherited event and an observable scalar. */
class ready_base : public virtual sc_core::sc_interface
{
 public:
  /** @brief Return the event used by the inherited finder member pointer. */
  const sc_core::sc_event& ready() const
  {
    return ready_event_;
  }

  /** @brief Return a stable scalar value for the callback reference. */
  int status() const
  {
    return 73;
  }

  /** @brief Notify the event after the requested native delay. */
  void notify_ready(int delay, sc_core::sc_time_unit unit)
  {
    ready_event_.notify(delay, unit);
  }

 private:
  sc_core::sc_event ready_event_;
};

/** @brief Derive the selected port type while inheriting the event member. */
class ready_derived : public ready_base
{
};

using finder_type = sc_core::sc_event_finder_t<ready_derived>;

/** @brief Own a bound interface and observe its event through both finders. */
struct finder_model : sc_core::sc_module
{
  ready_derived source;
  sc_core::sc_port<ready_derived> port;
  finder_type finder;
  sc_core::sc_event_finder* cache;
  sc_core::sc_event_finder* cached;
  bool cache_same;
  bool cache_port;
  bool finder_lookup;
  unsigned callbacks;
  int event_value;
  sc_dt::uint64 time_value;

  SC_HAS_PROCESS(finder_model);

  /** @brief Bind the interface, create direct/cached finders, and register sensitivity. */
  explicit finder_model(sc_core::sc_module_name name)
    : sc_module(name)
    , source()
    , port("edge_port")
    , finder(port, &ready_base::ready)
    , cache(nullptr)
    , cached(nullptr)
    , cache_same(false)
    , cache_port(false)
    , finder_lookup(false)
    , callbacks(0)
    , event_value(0)
    , time_value(0)
  {
    port.bind(source);
    sc_core::sc_event_finder& first = sc_core::sc_event_finder::cached_create(cache, port, &ready_base::ready);
    sc_core::sc_event_finder& second = sc_core::sc_event_finder::cached_create(cache, port, &ready_base::ready);
    cached = &first;
    cache_same = &first == &second;
    cache_port = &first.port() == &port;
    finder_lookup = &finder.find_event() == &source.ready() && &first.find_event() == &source.ready();

    SC_METHOD(observe);
    sensitive << finder;
    dont_initialize();
  }

  /** @brief Delete the cache-owned finder while the port is still alive. */
  ~finder_model() override
  {
    delete cache;
  }

  /** @brief Record native callback count, value, and simulation timestamp. */
  void observe()
  {
    ++callbacks;
    event_value = source.status();
    time_value = sc_core::sc_time_stamp().value() / sc_core::sc_time(1, sc_core::SC_NS).value();
  }
};
}  // namespace event_finder_error_edges

/** @brief Run a bounded event and print the result shared with the Python model. */
int sc_main(int, char**)
{
  event_finder_error_edges::finder_model model("event_finder_error_edges_model");
  model.source.notify_ready(3, sc_core::SC_NS);
  sc_core::sc_start(4, sc_core::SC_NS);
  std::cout << "FINDER_EDGE_REFERENCE finder_lookup=" << static_cast<int>(model.finder_lookup)
            << " cache_same=" << static_cast<int>(model.cache_same)
            << " cache_port=" << static_cast<int>(model.cache_port) << " callbacks=" << model.callbacks
            << " event_value=" << model.event_value << " time_ns=" << model.time_value << "\n";
  return 0;
}
