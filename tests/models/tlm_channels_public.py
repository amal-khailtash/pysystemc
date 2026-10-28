"""Exercise public TLM 1 FIFO, request/response, and transport channel APIs."""

import gc

from amal.eda.systemc import sc_core as sc, tlm

import cppyy


PROBE = __file__.replace("/models/tlm_channels_public.py", "/native/tlm_channels_probe.h")
cppyy.include(PROBE)
probe = cppyy.gbl.pysystemc_tlm_channels

cppyy.cppdef(
    r"""
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

/** @brief Native wait/transport worker for the Python-side channel probe.
 *  @details All blocking calls run from native SC_THREAD stacks so Python
 *  coroutine state is never suspended by the SystemC kernel. Channels own
 *  their internal queues; transaction objects remain alive for each call.
 */
class PythonChannelTraffic : public sc_core::sc_module
{
public:
  tlm::tlm_fifo<pysystemc_tlm_channels::Value> fifo;
  tlm::tlm_fifo<pysystemc_tlm_channels::Value> default_fifo;
  tlm::tlm_fifo<pysystemc_tlm_channels::Value> unbounded_fifo;
  pysystemc_tlm_channels::ExposedFifo<pysystemc_tlm_channels::Value> exposed_fifo;
  tlm::tlm_req_rsp_channel<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> req_rsp_transport;
  tlm::tlm_req_rsp_channel<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> req_rsp_default;
  tlm::tlm_req_rsp_channel<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> req_rsp_idle_named;
  tlm::tlm_transport_channel<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> transport_channel;
  tlm::tlm_transport_channel<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> transport_channel_default;
  tlm::tlm_transport_to_master<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> transport_adapter;
  tlm::tlm_transport_to_master<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> transport_adapter_default;
  pysystemc_tlm_channels::DummyTransport dummy_transport;
  tlm::tlm_slave_to_transport<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> slave_adapter;
  tlm::tlm_slave_to_transport<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response> slave_adapter_default;
  std::vector<std::string> events;

  SC_HAS_PROCESS(PythonChannelTraffic);

  /** @brief Construct named/default channels and bind every adapter port. */
  explicit PythonChannelTraffic(sc_core::sc_module_name name)
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

  /** @brief Queue one request on each dormant slave adapter input. */
  void seed_idle_requests()
  {
    using namespace pysystemc_tlm_channels;
    if (!req_rsp_default.put_request_export->nb_put(make_request(71, 0x171)) ||
        !req_rsp_idle_named.put_request_export->nb_put(make_request(72, 0x172)))
      throw std::runtime_error("could not seed dormant adapter input");
  }

  /** @brief Return the number of adapter requests left unconsumed. */
  int idle_requests() const
  {
    return static_cast<int>(req_rsp_default.get_request_export->nb_can_get()) +
           static_cast<int>(req_rsp_idle_named.get_request_export->nb_can_get());
  }

  /** @brief Format current simulation time for stable native records. */
  std::string now() const
  {
    return sc_core::sc_time_stamp().to_string();
  }

  /** @brief Exercise a blocking writer against a capacity-one FIFO. */
  void write_fifo()
  {
    using namespace pysystemc_tlm_channels;
    wait(1, sc_core::SC_NS);
    const Value first(101);
    fifo_blocking.put(first);
    events.push_back("FIFO_WRITE " + first.text() + " " + now());
    const Value second(202);
    fifo_blocking.put(second);
    events.push_back("FIFO_WRITE " + second.text() + " " + now());
  }

  /** @brief Exercise blocking reads, including an initially empty FIFO. */
  void read_fifo()
  {
    const auto first = fifo_blocking.get();
    events.push_back("FIFO_READ " + first.text() + " " + now());
    wait(2, sc_core::SC_NS);
    const auto second = fifo_blocking.get();
    events.push_back("FIFO_READ " + second.text() + " " + now());
  }

  /** @brief Consume a request and publish its response for transport adapter. */
  void serve_adapter()
  {
    using namespace pysystemc_tlm_channels;
    Request request;
    req_rsp_transport.get_request_export->get(request);
    events.push_back("T2M_REQUEST " + request.text() + " " + now());
    wait(1, sc_core::SC_NS);
    const auto response = make_response(request);
    req_rsp_transport.put_response_export->put(response);
    events.push_back("T2M_RESPONSE " + response.text() + " " + now());
  }

  /** @brief Call the adapter's native blocking transport entry point. */
  void call_adapter()
  {
    using namespace pysystemc_tlm_channels;
    wait(1, sc_core::SC_NS);
    const auto request = make_request(31, 0x131);
    const auto response = transport_adapter.target_export->transport(request);
    events.push_back("T2M_RETURN " + response.text() + " " + now());
  }

  /** @brief Consume a channel request and publish its delayed response. */
  void serve_channel()
  {
    using namespace pysystemc_tlm_channels;
    Request request;
    transport_channel.get_request_export->get(request);
    events.push_back("CHANNEL_REQUEST " + request.text() + " " + now());
    wait(1, sc_core::SC_NS);
    const auto response = make_response(request);
    transport_channel.put_response_export->put(response);
    events.push_back("CHANNEL_RESPONSE " + response.text() + " " + now());
  }

  /** @brief Call the composite channel's blocking target transport interface. */
  void call_channel()
  {
    using namespace pysystemc_tlm_channels;
    wait(1, sc_core::SC_NS);
    const auto request = make_request(41, 0x141);
    const auto response = transport_channel.target_export->transport(request);
    events.push_back("CHANNEL_RETURN " + response.text() + " " + now());
  }

private:
  tlm::tlm_fifo<pysystemc_tlm_channels::Value> fifo_blocking{"blocking_fifo", 1};
};

static_assert(std::is_same_v<
              typename tlm::tlm_slave_to_transport<pysystemc_tlm_channels::Request,
                                                   pysystemc_tlm_channels::Response>::SC_CURRENT_USER_MODULE,
              tlm::tlm_slave_to_transport<pysystemc_tlm_channels::Request, pysystemc_tlm_channels::Response>>);
"""
)

value_type = probe.Value
fifo_type = tlm.tlm_fifo[value_type]
request_type = probe.Request
response_type = probe.Response
fifo_tag = tlm.tlm_tag[value_type]()


def record(text):
    """Emit one normalized event for the native reference comparison."""
    print("TLM_CHANNEL_EVENT " + text)


probe.reset_exposed_fifo_destructor_count()
destructor_probe = probe.ExposedFifo[value_type]("fifo_destructor_probe", 1)
del destructor_probe
gc.collect()
assert probe.exposed_fifo_destructor_count() == 1
record("FIFO_DTOR 1")
probe.reset_exposed_fifo_destructor_count()

traffic_type = cppyy.gbl.PythonChannelTraffic
traffic = traffic_type(sc.sc_module_name("top"))


# Verify native FIFO aliases, constructors, constants, event identity, and kind.
assert "pysystemc_tlm_channels::Value" in tlm.tlm_fifo[value_type].__cpp_name__
assert "tlm_fifo_get_if" in tlm.tlm_fifo_get_if[value_type].__cpp_name__
assert "tlm_fifo_put_if" in tlm.tlm_fifo_put_if[value_type].__cpp_name__
assert "tlm_fifo_debug_if" in tlm.tlm_fifo_debug_if[value_type].__cpp_name__
assert "tlm_fifo_config_size_if" in tlm.tlm_fifo_config_size_if.__cpp_name__
assert traffic.fifo.kind() == "tlm_fifo"
assert probe.fifo_kind_constant() == "tlm_fifo"
assert traffic.fifo.size() == 2 and traffic.fifo.used() == 0
assert traffic.default_fifo.size() == 1
assert traffic.unbounded_fifo.size() == -1
record(f"FIFO_KIND {traffic.fifo.kind()} {probe.fifo_kind_constant()} 2 1 -1")

get_event = probe.fifo_ok_get_event_address(traffic.fifo)
peek_event = probe.fifo_ok_peek_event_address(traffic.fifo)
put_event = probe.fifo_ok_put_event_address(traffic.fifo)
assert get_event == peek_event and put_event != get_event
python_events = (
    cppyy.addressof(traffic.fifo.ok_to_get()),
    cppyy.addressof(traffic.fifo.ok_to_get(fifo_tag)),
    cppyy.addressof(traffic.fifo.ok_to_peek()),
    cppyy.addressof(traffic.fifo.ok_to_peek(fifo_tag)),
    cppyy.addressof(traffic.fifo.ok_to_put()),
    cppyy.addressof(traffic.fifo.ok_to_put(fifo_tag)),
)
expected_events = (get_event, get_event, peek_event, peek_event, put_event, put_event)
assert python_events == expected_events
initial_tag_status = (
    traffic.fifo.nb_can_put(fifo_tag),
    traffic.fifo.nb_can_get(fifo_tag),
    traffic.fifo.nb_can_peek(fifo_tag),
)
assert initial_tag_status == (True, False, False)
assert probe.fifo_read_event_address(traffic.exposed_fifo) != 0
read_event_name = str(traffic.exposed_fifo.read_event_ref().name())
assert read_event_name
traffic.exposed_fifo.update_ref()

first = value_type(11)
second = value_type(22)
third = value_type(33)
output = value_type()
assert traffic.fifo.nb_can_put()
assert not traffic.fifo.nb_can_get() and not traffic.fifo.nb_can_peek()
assert traffic.fifo.nb_put(first) and traffic.fifo.nb_put(second)
assert not traffic.fifo.nb_put(third) and not traffic.fifo.nb_can_put()
sc.sc_start(sc.SC_ZERO_TIME)
assert traffic.fifo.used() == 2
assert traffic.fifo.nb_can_get() and traffic.fifo.nb_can_peek()
full_tag_status = (
    traffic.fifo.nb_can_put(fifo_tag),
    traffic.fifo.nb_can_get(fifo_tag),
    traffic.fifo.nb_can_peek(fifo_tag),
)
assert full_tag_status == (False, True, True)
tag_results = (*initial_tag_status, *(a == b for a, b in zip(python_events, expected_events)), *full_tag_status)
record("FIFO_TAGS " + " ".join(str(int(value)) for value in tag_results))
assert not traffic.fifo.nb_bound(0) and traffic.fifo.size() == 2
assert not traffic.fifo.nb_reduce(1) and traffic.fifo.size() == 2
assert traffic.fifo.nb_peek(output) and output.text() == first.text()
assert traffic.fifo.nb_peek(output, 0) and output.text() == first.text()
assert traffic.fifo.nb_peek(output, 1) and output.text() == second.text()
assert traffic.fifo.nb_peek(output, -1) and output.text() == second.text()
assert not traffic.fifo.nb_peek(output, 2)
assert traffic.fifo.nb_poke(third, 1)
assert traffic.fifo.nb_peek(output, 1) and output.text() == third.text()
assert traffic.fifo.nb_poke(first)
assert not traffic.fifo.nb_poke(first, 2)
assert traffic.fifo.peek().text() == first.text()
assert traffic.fifo.nb_get(output) and output.text() == first.text()
sc.sc_start(sc.SC_ZERO_TIME)
assert traffic.fifo.get().text() == third.text()
sc.sc_start(sc.SC_ZERO_TIME)
assert traffic.fifo.used() == 0

assert traffic.fifo.nb_bound(0) and traffic.fifo.size() == 0
assert traffic.fifo.nb_expand(3) is None and traffic.fifo.size() == 3
assert traffic.fifo.nb_bound(4) and traffic.fifo.size() == 4
traffic.fifo.nb_unbound(5)
assert traffic.fifo.size() == -5 and not traffic.fifo.nb_reduce(1)
assert traffic.fifo.nb_bound(2) and traffic.fifo.size() == 2
traffic.unbounded_fifo.nb_unbound(7)
assert traffic.unbounded_fifo.size() == -7
traffic.fifo.debug()
record("FIFO_NONBLOCK 11 22 33 2 3 -5 2")

# Public request/response fields and named/default channel constructors.
req_rsp = traffic.req_rsp_transport
req_rsp_default = traffic.req_rsp_default
for channel in (req_rsp, req_rsp_default, traffic.req_rsp_idle_named):
    for field in (
        "get_request_export",
        "get_response_export",
        "put_request_export",
        "put_response_export",
        "master_export",
        "slave_export",
    ):
        assert cppyy.addressof(getattr(channel, field)) != 0
transport = traffic.transport_channel
transport_default = traffic.transport_channel_default
for channel in (transport, transport_default):
    for field in ("get_request_export", "put_response_export", "slave_export", "target_export"):
        assert cppyy.addressof(getattr(channel, field)) != 0
for adapter in (traffic.transport_adapter, traffic.transport_adapter_default):
    assert cppyy.addressof(adapter.target_export) != 0
    assert cppyy.addressof(adapter.master_port) != 0
for adapter in (traffic.slave_adapter, traffic.slave_adapter_default):
    assert cppyy.addressof(adapter.slave_port) != 0
    assert cppyy.addressof(adapter.initiator_port) != 0
assert traffic.transport_adapter.target_export.get_interface().transport
assert "tlm_req_rsp_channel" in type(req_rsp).__cpp_name__
assert "tlm_transport_channel" in type(transport).__cpp_name__
assert "tlm_transport_to_master" in type(traffic.transport_adapter).__cpp_name__
assert "tlm_slave_to_transport" in type(traffic.slave_adapter).__cpp_name__
record("CHANNEL_FIELDS 6 4 2 2")

# Seed both unregistered slave adapters, then prove bounded simulation leaves
# their requests in place and never calls the bound dummy transport.
traffic.seed_idle_requests()
sc.sc_start(sc.SC_ZERO_TIME)
assert traffic.idle_requests() == 2
sc.sc_start(sc.sc_time(4, sc.SC_NS))
assert traffic.idle_requests() == 2 and traffic.dummy_transport.calls == 0
for event in traffic.events:
    record(str(event))
assert [str(event).split()[0] for event in traffic.events].count("FIFO_WRITE") == 2
assert [str(event).split()[0] for event in traffic.events].count("FIFO_READ") == 2
assert any(str(event).startswith("T2M_RETURN 1031:") for event in traffic.events)
assert any(str(event).startswith("CHANNEL_RETURN 1041:") for event in traffic.events)
assert sc.sc_time_stamp() == sc.sc_time(4, sc.SC_NS)
record(f"SLAVE_ADAPTER_IDLE {traffic.idle_requests()} {traffic.dummy_transport.calls}")
sc.sc_stop()
record(f"TIME {sc.sc_time_stamp()}")
print("TLM_CHANNELS_PUBLIC_OK")
