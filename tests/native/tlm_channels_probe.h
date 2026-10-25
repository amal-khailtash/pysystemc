/** @file tlm_channels_probe.h
 *  @brief Arbitrary-width data types for TLM 1 channel conformance probes.
 *  @details The native channels own their internal queues and module state.
 *  Request, response, and FIFO values are copied through native interfaces.
 *  Blocking interface calls suspend the SystemC process that invokes them;
 *  the probes keep every output object alive for the full call.
 */
#ifndef PYSYSTEMC_TLM_CHANNELS_PROBE_H
#define PYSYSTEMC_TLM_CHANNELS_PROBE_H

#include <cstdint>
#include <string>
#include <systemc>
#include <tlm>

namespace pysystemc_tlm_channels
{

/** @brief Shared test-local count of probe FIFO destruction events. */
inline unsigned int& exposed_fifo_destructor_count()
{
  static unsigned int count = 0;
  return count;
}

/** @brief Clear the probe FIFO destruction count before one native run. */
inline void reset_exposed_fifo_destructor_count()
{
  exposed_fifo_destructor_count() = 0;
}

using WideValue = sc_dt::sc_biguint<137>;

/** @brief FIFO value with a 137-bit field and a sequence number. */
struct Value
{
  WideValue bits = 0;
  int sequence = 0;

  /** @brief Construct one numbered value with its wide field initialized. */
  explicit Value(int number = 0) : bits(number), sequence(number)
  {
  }

  /** @brief Return a stable decimal view of both value fields. */
  std::string text() const
  {
    return std::to_string(sequence) + ":" + bits.to_string();
  }
};

/** @brief Request type distinct from response and FIFO values. */
struct Request
{
  WideValue payload = 0;
  int id = 0;

  /** @brief Construct a numbered request and encode its payload. */
  explicit Request(int number = 0) : payload(number), id(number)
  {
  }

  /** @brief Return a stable decimal view of the request. */
  std::string text() const
  {
    return std::to_string(id) + ":" + payload.to_string();
  }
};

/** @brief Response type with a wider payload than its request. */
struct Response
{
  sc_dt::sc_biguint<149> payload = 0;
  int id = 0;

  /** @brief Construct a numbered response and encode its payload. */
  explicit Response(int number = 0) : payload(number), id(number)
  {
  }

  /** @brief Return a stable decimal view of the response. */
  std::string text() const
  {
    return std::to_string(id) + ":" + payload.to_string();
  }
};

/** @brief Build a request with independent payload and transaction identities. */
inline Request make_request(int id, unsigned int payload)
{
  Request request(id);
  request.payload = payload;
  request.payload[136] = true;
  return request;
}

/** @brief Build the correlated response used by both adapter paths. */
inline Response make_response(const Request& request)
{
  Response response(request.id + 1000);
  response.payload = request.payload;
  response.payload[148] = true;
  return response;
}

/** @brief Expose protected FIFO hooks while preserving the native channel. */
template <class T>
class ExposedFifo : public tlm::tlm_fifo<T>
{
  using Base = tlm::tlm_fifo<T>;

 public:
  using Base::Base;

  /** @brief Count native teardown while preserving base destruction behavior. */
  ~ExposedFifo() override
  {
    ++exposed_fifo_destructor_count();
  }

  /** @brief Return the protected read event as a borrowed reference. */
  sc_core::sc_event& read_event_ref()
  {
    return Base::read_event();
  }

  /** @brief Run the native update hook for direct hook coverage. */
  void update_ref()
  {
    Base::update();
  }
};

/** @brief Return the 137-bit FIFO template's native kind constant. */
inline const char* fifo_kind_constant()
{
  return tlm::tlm_fifo<Value>::kind_string;
}

/** @brief Return a borrowed FIFO read event address through the exact base. */
inline std::uintptr_t fifo_read_event_address(ExposedFifo<Value>& fifo)
{
  return reinterpret_cast<std::uintptr_t>(&fifo.read_event_ref());
}

/** @brief Return borrowed event addresses through the public FIFO methods. */
inline std::uintptr_t fifo_ok_get_event_address(tlm::tlm_fifo<Value>& fifo)
{
  return reinterpret_cast<std::uintptr_t>(&fifo.ok_to_get());
}
inline std::uintptr_t fifo_ok_peek_event_address(tlm::tlm_fifo<Value>& fifo)
{
  return reinterpret_cast<std::uintptr_t>(&fifo.ok_to_peek());
}
inline std::uintptr_t fifo_ok_put_event_address(tlm::tlm_fifo<Value>& fifo)
{
  return reinterpret_cast<std::uintptr_t>(&fifo.ok_to_put());
}

/** @brief Minimal transport implementation used to bind the dormant adapter. */
class DummyTransport : public sc_core::sc_module, public virtual tlm::tlm_transport_if<Request, Response>
{
 public:
  sc_core::sc_export<tlm::tlm_transport_if<Request, Response>> target_export;
  unsigned int calls = 0;

  /** @brief Bind this transport implementation to its public export. */
  explicit DummyTransport(sc_core::sc_module_name name) : sc_core::sc_module(name), target_export("target_export")
  {
    target_export(*this);
  }

  /** @brief Count a transport and return a deterministic correlated response. */
  Response transport(const Request& request) override
  {
    ++calls;
    return make_response(request);
  }
};

}  // namespace pysystemc_tlm_channels

#endif  // PYSYSTEMC_TLM_CHANNELS_PROBE_H
