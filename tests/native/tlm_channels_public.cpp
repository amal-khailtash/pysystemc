/** @file Native reference for TLM 1 FIFOs and request/response channels. */
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "tlm_channels_probe.h"

using namespace pysystemc_tlm_channels;

/** @brief Run native blocking traffic and expose every public channel member. */
class NativeChannelTraffic : public sc_core::sc_module
{
 public:
  tlm::tlm_fifo<Value> fifo;
  tlm::tlm_fifo<Value> default_fifo;
  tlm::tlm_fifo<Value> unbounded_fifo;
  ExposedFifo<Value> exposed_fifo;
  tlm::tlm_req_rsp_channel<Request, Response> req_rsp_transport;
  tlm::tlm_req_rsp_channel<Request, Response> req_rsp_default;
  tlm::tlm_req_rsp_channel<Request, Response> req_rsp_idle_named;
  tlm::tlm_transport_channel<Request, Response> transport_channel;
  tlm::tlm_transport_channel<Request, Response> transport_channel_default;
  tlm::tlm_transport_to_master<Request, Response> transport_adapter;
  tlm::tlm_transport_to_master<Request, Response> transport_adapter_default;
  DummyTransport dummy_transport;
  tlm::tlm_slave_to_transport<Request, Response> slave_adapter;
  tlm::tlm_slave_to_transport<Request, Response> slave_adapter_default;
  std::vector<std::string> events;

  SC_HAS_PROCESS(NativeChannelTraffic);

  /** @brief Construct named/default channels and bind all public adapter ports. */
  explicit NativeChannelTraffic(sc_core::sc_module_name name)
    : sc_core::sc_module(name)
    , fifo("matrix_fifo", 2)
    , default_fifo()
    , unbounded_fifo("unbounded_fifo", -1)
    , exposed_fifo("exposed_fifo", 1)
    , req_rsp_transport("req_rsp_transport", 1, 1)
    , req_rsp_default(1, 1)
    , req_rsp_idle_named("req_rsp_idle_named", 1, 1)
    , transport_channel("transport_channel")
    , transport_channel_default()
    , transport_adapter("transport_adapter")
    , transport_adapter_default()
    , dummy_transport("dummy_transport")
    , slave_adapter("slave_adapter")
    , slave_adapter_default()
  {
    transport_adapter.master_port(req_rsp_transport.master_export);
    transport_adapter_default.master_port(req_rsp_default.master_export);
    slave_adapter.slave_port(req_rsp_idle_named.slave_export);
    slave_adapter.initiator_port(dummy_transport.target_export);
    slave_adapter_default.slave_port(req_rsp_default.slave_export);
    slave_adapter_default.initiator_port(dummy_transport.target_export);
    SC_THREAD(write_fifo);
    SC_THREAD(read_fifo);
    SC_THREAD(serve_adapter);
    SC_THREAD(call_adapter);
    SC_THREAD(serve_channel);
    SC_THREAD(call_channel);
  }

  /** @brief Seed both ports that the upstream unregistered adapter leaves idle. */
  void seed_idle_requests()
  {
    if (!req_rsp_default.put_request_export->nb_put(make_request(71, 0x171)) ||
        !req_rsp_idle_named.put_request_export->nb_put(make_request(72, 0x172)))
      throw std::runtime_error("could not seed dormant adapter input");
  }

  /** @brief Count both requests still queued after bounded simulation. */
  int idle_requests() const
  {
    return static_cast<int>(req_rsp_default.get_request_export->nb_can_get()) +
           static_cast<int>(req_rsp_idle_named.get_request_export->nb_can_get());
  }

  /** @brief Format native kernel time for parity records. */
  std::string now() const
  {
    return sc_core::sc_time_stamp().to_string();
  }

  /** @brief Write two values, blocking on the second while capacity is full. */
  void write_fifo()
  {
    wait(1, sc_core::SC_NS);
    const Value first(101);
    fifo_blocking.put(first);
    events.push_back("FIFO_WRITE " + first.text() + " " + now());
    const Value second(202);
    fifo_blocking.put(second);
    events.push_back("FIFO_WRITE " + second.text() + " " + now());
  }

  /** @brief Read an initially empty FIFO, then read after a timed wait. */
  void read_fifo()
  {
    const auto first = fifo_blocking.get();
    events.push_back("FIFO_READ " + first.text() + " " + now());
    wait(2, sc_core::SC_NS);
    const auto second = fifo_blocking.get();
    events.push_back("FIFO_READ " + second.text() + " " + now());
  }

  /** @brief Receive an adapter request and return its delayed response. */
  void serve_adapter()
  {
    Request request;
    req_rsp_transport.get_request_export->get(request);
    events.push_back("T2M_REQUEST " + request.text() + " " + now());
    wait(1, sc_core::SC_NS);
    const auto response = make_response(request);
    req_rsp_transport.put_response_export->put(response);
    events.push_back("T2M_RESPONSE " + response.text() + " " + now());
  }

  /** @brief Call the standalone native transport-to-master adapter. */
  void call_adapter()
  {
    wait(1, sc_core::SC_NS);
    const auto request = make_request(31, 0x131);
    const auto response = transport_adapter.target_export->transport(request);
    events.push_back("T2M_RETURN " + response.text() + " " + now());
  }

  /** @brief Receive a composite channel request and return its response. */
  void serve_channel()
  {
    Request request;
    transport_channel.get_request_export->get(request);
    events.push_back("CHANNEL_REQUEST " + request.text() + " " + now());
    wait(1, sc_core::SC_NS);
    const auto response = make_response(request);
    transport_channel.put_response_export->put(response);
    events.push_back("CHANNEL_RESPONSE " + response.text() + " " + now());
  }

  /** @brief Call the composite request/response channel transport interface. */
  void call_channel()
  {
    wait(1, sc_core::SC_NS);
    const auto request = make_request(41, 0x141);
    const auto response = transport_channel.target_export->transport(request);
    events.push_back("CHANNEL_RETURN " + response.text() + " " + now());
  }

 private:
  tlm::tlm_fifo<Value> fifo_blocking{"blocking_fifo", 1};
};

/** @brief Enforce conformance checks in builds where assert may be disabled. */
static void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Return the position of an event with the requested prefix. */
static std::size_t event_position(const std::vector<std::string>& events, const std::string& prefix)
{
  const auto found =
    std::find_if(events.begin(), events.end(), [&](const std::string& event) { return event.find(prefix) == 0; });
  if (found == events.end()) throw std::runtime_error("missing event: " + prefix);
  return static_cast<std::size_t>(found - events.begin());
}

int sc_main(int, char**)
{
  static_assert(std::is_same_v<typename tlm::tlm_slave_to_transport<Request, Response>::SC_CURRENT_USER_MODULE,
                               tlm::tlm_slave_to_transport<Request, Response>>);
  reset_exposed_fifo_destructor_count();
  {
    ExposedFifo<Value> destructor_probe("fifo_destructor_probe", 1);
  }
  verify(exposed_fifo_destructor_count() == 1, "exposed FIFO destructor");
  std::cout << "TLM_CHANNEL_EVENT FIFO_DTOR 1\n";
  reset_exposed_fifo_destructor_count();
  {
    NativeChannelTraffic traffic("top");
    verify(traffic.fifo.kind() == std::string("tlm_fifo") && fifo_kind_constant() == std::string("tlm_fifo"),
           "FIFO kind");
    verify(traffic.fifo.size() == 2 && traffic.default_fifo.size() == 1 && traffic.unbounded_fifo.size() == -1,
           "FIFO constructor sizes");
    std::cout << "TLM_CHANNEL_EVENT FIFO_KIND tlm_fifo tlm_fifo 2 1 -1\n";
    verify(fifo_ok_get_event_address(traffic.fifo) == fifo_ok_peek_event_address(traffic.fifo) &&
             fifo_ok_put_event_address(traffic.fifo) != fifo_ok_get_event_address(traffic.fifo) &&
             fifo_read_event_address(traffic.exposed_fifo),
           "FIFO event identities");
    traffic.exposed_fifo.update_ref();

    Value first(11), second(22), third(33), output;
    tlm::tlm_tag<Value> fifo_tag;
    const bool initial_tag_put = traffic.fifo.nb_can_put(&fifo_tag);
    const bool initial_tag_get = traffic.fifo.nb_can_get(&fifo_tag);
    const bool initial_tag_peek = traffic.fifo.nb_can_peek(&fifo_tag);
    const bool tagged_events_match =
      reinterpret_cast<std::uintptr_t>(&traffic.fifo.ok_to_get()) == fifo_ok_get_event_address(traffic.fifo) &&
      reinterpret_cast<std::uintptr_t>(&traffic.fifo.ok_to_get(&fifo_tag)) == fifo_ok_get_event_address(traffic.fifo) &&
      reinterpret_cast<std::uintptr_t>(&traffic.fifo.ok_to_peek()) == fifo_ok_peek_event_address(traffic.fifo) &&
      reinterpret_cast<std::uintptr_t>(&traffic.fifo.ok_to_peek(&fifo_tag)) ==
        fifo_ok_peek_event_address(traffic.fifo) &&
      reinterpret_cast<std::uintptr_t>(&traffic.fifo.ok_to_put()) == fifo_ok_put_event_address(traffic.fifo) &&
      reinterpret_cast<std::uintptr_t>(&traffic.fifo.ok_to_put(&fifo_tag)) == fifo_ok_put_event_address(traffic.fifo);
    verify(traffic.fifo.nb_can_put() && !traffic.fifo.nb_can_get() && !traffic.fifo.nb_can_peek(),
           "initial FIFO flags");
    verify(traffic.fifo.nb_put(first) && traffic.fifo.nb_put(second) && !traffic.fifo.nb_put(third), "nonblocking put");
    verify(!traffic.fifo.nb_can_put(), "full FIFO put status");
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    verify(traffic.fifo.used() == 2 && traffic.fifo.nb_can_get() && traffic.fifo.nb_can_peek(), "FIFO used state");
    const bool full_tag_put = traffic.fifo.nb_can_put(&fifo_tag);
    const bool full_tag_get = traffic.fifo.nb_can_get(&fifo_tag);
    const bool full_tag_peek = traffic.fifo.nb_can_peek(&fifo_tag);
    verify(initial_tag_put && !initial_tag_get && !initial_tag_peek && tagged_events_match && !full_tag_put &&
             full_tag_get && full_tag_peek,
           "tagged FIFO event and status methods");
    std::cout << "TLM_CHANNEL_EVENT FIFO_TAGS 1 0 0 1 1 1 1 1 1 0 1 1\n";
    verify(!traffic.fifo.nb_bound(0) && traffic.fifo.size() == 2 && !traffic.fifo.nb_reduce(1),
           "FIFO bounded reduction");
    verify(traffic.fifo.nb_peek(output) && output.text() == first.text(), "FIFO peek output");
    verify(traffic.fifo.nb_peek(output, 0) && output.text() == first.text() && traffic.fifo.nb_peek(output, 1) &&
             output.text() == second.text() && traffic.fifo.nb_peek(output, -1) && output.text() == second.text() &&
             !traffic.fifo.nb_peek(output, 2),
           "FIFO indexed peek");
    verify(traffic.fifo.nb_poke(third, 1) && traffic.fifo.nb_peek(output, 1) && output.text() == third.text() &&
             traffic.fifo.nb_poke(first) && !traffic.fifo.nb_poke(first, 2),
           "FIFO poke overloads");
    verify(traffic.fifo.peek().text() == first.text(), "FIFO blocking peek");
    verify(traffic.fifo.nb_get(output) && output.text() == first.text(), "FIFO nonblocking get");
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    verify(traffic.fifo.get().text() == third.text(), "FIFO blocking get");
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    verify(traffic.fifo.used() == 0 && traffic.fifo.nb_bound(0), "FIFO empty state and bound");
    traffic.fifo.nb_expand(3);
    verify(traffic.fifo.size() == 3 && traffic.fifo.nb_bound(4), "FIFO expansion");
    traffic.fifo.nb_unbound(5);
    verify(traffic.fifo.size() == -5 && !traffic.fifo.nb_reduce(1) && traffic.fifo.nb_bound(2),
           "FIFO unbounded resize");
    traffic.unbounded_fifo.nb_unbound(7);
    verify(traffic.unbounded_fifo.size() == -7, "FIFO default unbounded amount");
    traffic.fifo.debug();
    std::cout << "TLM_CHANNEL_EVENT FIFO_NONBLOCK 11 22 33 2 3 -5 2\n";

    verify(traffic.req_rsp_transport.master_export.get_interface() != nullptr &&
             traffic.req_rsp_transport.slave_export.get_interface() != nullptr &&
             traffic.req_rsp_transport.get_request_export.get_interface() != nullptr &&
             traffic.req_rsp_transport.get_response_export.get_interface() != nullptr &&
             traffic.req_rsp_transport.put_request_export.get_interface() != nullptr &&
             traffic.req_rsp_transport.put_response_export.get_interface() != nullptr,
           "request response exports");
    verify(traffic.transport_channel.target_export.get_interface() != nullptr &&
             traffic.transport_channel.slave_export.get_interface() != nullptr &&
             traffic.transport_channel.get_request_export.get_interface() != nullptr &&
             traffic.transport_channel.put_response_export.get_interface() != nullptr,
           "transport channel exports");
    verify(traffic.transport_adapter.target_export.get_interface() != nullptr &&
             traffic.transport_adapter.master_port.get_interface() != nullptr &&
             traffic.slave_adapter.slave_port.get_interface() != nullptr &&
             traffic.slave_adapter.initiator_port.get_interface() != nullptr,
           "adapter ports and exports");
    std::cout << "TLM_CHANNEL_EVENT CHANNEL_FIELDS 6 4 2 2\n";
    traffic.seed_idle_requests();
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    verify(traffic.idle_requests() == 2, "seeded idle slave adapter requests");
    sc_core::sc_start(4, sc_core::SC_NS);
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(4, sc_core::SC_NS), "channel final time");
    verify(traffic.idle_requests() == 2 && traffic.dummy_transport.calls == 0, "upstream slave adapter is dormant");
    verify(event_position(traffic.events, "FIFO_WRITE 101") < event_position(traffic.events, "FIFO_READ 101") &&
             event_position(traffic.events, "FIFO_WRITE 202") < event_position(traffic.events, "FIFO_READ 202"),
           "blocking FIFO order");
    verify(event_position(traffic.events, "T2M_REQUEST") < event_position(traffic.events, "T2M_RESPONSE") &&
             event_position(traffic.events, "T2M_RESPONSE") < event_position(traffic.events, "T2M_RETURN"),
           "transport-to-master response order");
    verify(event_position(traffic.events, "CHANNEL_REQUEST") < event_position(traffic.events, "CHANNEL_RESPONSE") &&
             event_position(traffic.events, "CHANNEL_RESPONSE") < event_position(traffic.events, "CHANNEL_RETURN"),
           "transport channel response order");
    for (const auto& event : traffic.events) std::cout << "TLM_CHANNEL_EVENT " << event << '\n';
    std::cout << "TLM_CHANNEL_EVENT SLAVE_ADAPTER_IDLE " << traffic.idle_requests() << ' '
              << traffic.dummy_transport.calls << '\n';
    sc_core::sc_stop();
  }
  std::cout << "TLM_CHANNEL_EVENT TIME " << sc_core::sc_time_stamp() << '\n';
  std::cout << "TLM_CHANNELS_PUBLIC_OK\n";
  return 0;
}
