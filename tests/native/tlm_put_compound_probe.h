/** @file tlm_put_compound_probe.h
 *  @brief Native reference types for TLM put, compound and FIFO interfaces.
 *  @details Interface references borrow their adapter. Adapters own wide
 *  values, readiness events and callback counters; no Python callback or GIL
 *  state is retained by this standalone reference.
 */
#ifndef PYSYSTEMC_TLM_PUT_COMPOUND_PROBE_H
#define PYSYSTEMC_TLM_PUT_COMPOUND_PROBE_H

#include <deque>
#include <systemc>
#include <tlm>
#include <vector>

namespace pysystemc_tlm_put_compound
{

using Wide = sc_dt::sc_biguint<137>;

/** @brief Return a 137-bit value with both its low word and top bit set. */
inline Wide wide(unsigned int low)
{
  Wide value = low;
  value[136] = true;
  return value;
}

/** @brief A custom request/response type for transport interface templates. */
struct Record
{
  int code = 0;
};

/** @brief Implement a put contract with native event storage and exact counts. */
class PutAdapter : public tlm::tlm_put_if<Wide>
{
 public:
  /** @brief Copy a blocking put sample into adapter-owned storage. */
  void put(const Wide& value) override
  {
    values_.push_back(value);
    ++put_calls;
  }
  /** @brief Accept one nonblocking sample. */
  bool nb_put(const Wide& value) override
  {
    values_.push_back(value);
    ++nb_put_calls;
    return true;
  }
  /** @brief Report capacity and count each query. */
  bool nb_can_put(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_put_calls;
    return true;
  }
  /** @brief Return adapter-owned readiness storage. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++event_calls;
    return ready_;
  }
  /** @brief Count the number of samples accepted by either put method. */
  int sample_count() const
  {
    return static_cast<int>(values_.size());
  }

  int put_calls = 0;
  int nb_put_calls = 0;
  mutable int can_put_calls = 0;
  mutable int event_calls = 0;

 private:
  std::vector<Wide> values_;
  mutable sc_core::sc_event ready_;
};

/** @brief Implement either request/response direction with wide values.
 *  @tparam Interface One of the public combined, blocking or nonblocking
 *  master/slave interfaces. Requests and responses intentionally share a
 *  width here so every inherited operation has a visible native overload.
 */
template <class Interface>
class CompoundAdapter : public Interface
{
 public:
  /** @brief Start with two response samples in arrival order. */
  CompoundAdapter() : responses_{wide(7), wide(9)}
  {
  }

  /** @brief Retain a copied request. */
  void put(const Wide& value) override
  {
    sent_.push_back(value);
    ++put_calls;
  }
  /** @brief Return and consume one response. */
  Wide get(tlm::tlm_tag<Wide>* = nullptr) override
  {
    ++get_calls;
    const Wide value = responses_.front();
    responses_.pop_front();
    return value;
  }
  /** @brief Return the next response without consuming it. */
  Wide peek(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++peek_calls;
    return responses_.front();
  }
  /** @brief Accept and retain a nonblocking request. */
  bool nb_put(const Wide& value) override
  {
    sent_.push_back(value);
    ++nb_put_calls;
    return true;
  }
  /** @brief Report request readiness and count each query. */
  bool nb_can_put(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_put_calls;
    return true;
  }
  /** @brief Return the event borrowed by put readiness consumers. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    return put_ready_;
  }
  /** @brief Consume a response only when one is queued. */
  bool nb_get(Wide& output) override
  {
    ++nb_get_calls;
    if (responses_.empty()) return false;
    output = responses_.front();
    responses_.pop_front();
    return true;
  }
  /** @brief Report whether a response can be consumed. */
  bool nb_can_get(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_get_calls;
    return !responses_.empty();
  }
  /** @brief Return the event borrowed by get readiness consumers. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    return get_ready_;
  }
  /** @brief Copy the next response without consuming it. */
  bool nb_peek(Wide& output) const override
  {
    ++nb_peek_calls;
    if (responses_.empty()) return false;
    output = responses_.front();
    return true;
  }
  /** @brief Report whether a response can be inspected. */
  bool nb_can_peek(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_peek_calls;
    return !responses_.empty();
  }
  /** @brief Return the event borrowed by peek readiness consumers. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    return peek_ready_;
  }

  /** @brief Replace pending response samples for output-retention checks. */
  void set_responses(std::initializer_list<Wide> values)
  {
    responses_ = std::deque<Wide>(values);
  }
  /** @brief Return the retained request count. */
  int sent_count() const
  {
    return static_cast<int>(sent_.size());
  }

  int put_calls = 0;
  int get_calls = 0;
  mutable int peek_calls = 0;
  int nb_put_calls = 0;
  mutable int can_put_calls = 0;
  int nb_get_calls = 0;
  mutable int can_get_calls = 0;
  mutable int nb_peek_calls = 0;
  mutable int can_peek_calls = 0;

 private:
  std::deque<Wide> responses_;
  std::vector<Wide> sent_;
  sc_core::sc_event put_ready_;
  sc_core::sc_event get_ready_;
  sc_core::sc_event peek_ready_;
};

/** @brief Implement FIFO put, get/peek, debug and indexing contracts. */
class FifoAdapter : public tlm::tlm_fifo_put_if<Wide>, public tlm::tlm_fifo_get_if<Wide>
{
 public:
  /** @brief Start with newest and oldest wide values. */
  FifoAdapter() : values_{wide(17), wide(23)}
  {
  }
  /** @brief Append a blocking put at the newest end. */
  void put(const Wide& value) override
  {
    values_.push_front(value);
    ++put_calls;
  }
  /** @brief Append a nonblocking put at the newest end. */
  bool nb_put(const Wide& value) override
  {
    values_.push_front(value);
    ++nb_put_calls;
    return true;
  }
  /** @brief Report that the test FIFO has space. */
  bool nb_can_put(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_put_calls;
    return true;
  }
  /** @brief Return put readiness storage. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    return put_ready_;
  }
  /** @brief Consume the oldest FIFO value. */
  Wide get(tlm::tlm_tag<Wide>* = nullptr) override
  {
    ++get_calls;
    const Wide result = values_.back();
    values_.pop_back();
    return result;
  }
  /** @brief Return the oldest FIFO value without consuming it. */
  Wide peek(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++peek_calls;
    return values_.back();
  }
  /** @brief Consume an available oldest value into caller storage. */
  bool nb_get(Wide& output) override
  {
    ++nb_get_calls;
    if (values_.empty()) return false;
    output = values_.back();
    values_.pop_back();
    return true;
  }
  /** @brief Report whether the FIFO contains values. */
  bool nb_can_get(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_get_calls;
    return !values_.empty();
  }
  /** @brief Return get readiness storage. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    return get_ready_;
  }
  /** @brief Copy the oldest value into caller storage. */
  bool nb_peek(Wide& output) const override
  {
    ++nb_peek_calls;
    if (values_.empty()) return false;
    output = values_.back();
    return true;
  }
  /** @brief Report whether a FIFO value can be inspected. */
  bool nb_can_peek(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    ++can_peek_calls;
    return !values_.empty();
  }
  /** @brief Return peek readiness storage. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<Wide>* = nullptr) const override
  {
    return peek_ready_;
  }
  /** @brief Return FIFO occupancy. */
  int used() const override
  {
    return static_cast<int>(values_.size());
  }
  /** @brief Return fixed FIFO capacity. */
  int size() const override
  {
    return 4;
  }
  /** @brief Count a debug inspection. */
  void debug() const override
  {
    ++debug_calls;
  }
  /** @brief Read a newest-first FIFO entry without consuming it. */
  bool nb_peek(Wide& output, int index) const override
  {
    ++indexed_peek_calls;
    if (index < 0 || static_cast<std::size_t>(index) >= values_.size()) return false;
    output = values_[static_cast<std::size_t>(index)];
    return true;
  }
  /** @brief Replace a newest-first FIFO entry. */
  bool nb_poke(const Wide& value, int index = 0) override
  {
    ++poke_calls;
    if (index < 0 || static_cast<std::size_t>(index) >= values_.size()) return false;
    values_[static_cast<std::size_t>(index)] = value;
    return true;
  }

  int put_calls = 0;
  int nb_put_calls = 0;
  mutable int can_put_calls = 0;
  int get_calls = 0;
  mutable int peek_calls = 0;
  mutable int nb_peek_calls = 0;
  int nb_get_calls = 0;
  mutable int can_get_calls = 0;
  mutable int can_peek_calls = 0;
  mutable int debug_calls = 0;
  mutable int indexed_peek_calls = 0;
  int poke_calls = 0;

 private:
  std::deque<Wide> values_;
  sc_core::sc_event put_ready_;
  sc_core::sc_event get_ready_;
  sc_core::sc_event peek_ready_;
};

/** @brief Implement custom-value transport's pure value overload only.
 *  @details The native base output-reference overload remains inherited so its
 *  forwarding behavior can be tested directly.
 */
class TransportAdapter : public tlm::tlm_transport_if<Record, Record>
{
 public:
  /** @brief Return the custom response for one custom request. */
  Record transport(const Record& request) override
  {
    ++calls;
    return Record{request.code + 9};
  }

  int calls = 0;
};

}  // namespace pysystemc_tlm_put_compound

#endif  // PYSYSTEMC_TLM_PUT_COMPOUND_PROBE_H
