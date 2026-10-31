"""Exercise native TLM fallback rejection and recovery through an independent consumer.

The original multi-socket registration overload cannot currently be called by
cppyy with its reflected native member pointer, so the test records that
limitation and then registers the same native callback through the exact
public pointer holder. The native initiator and callback owner remain strongly
referenced through ``sc_start``; the callback itself runs on the kernel thread
without entering Python or acquiring the GIL.
"""

import gc
import weakref

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy


cppyy.cppdef(
    r"""


#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <systemc>
#include <tlm>

/** @brief Hold callback state and record native callback timing. */
struct SocketFallbackOwner : sc_core::sc_module
{
  using Target = tlm_utils::multi_passthrough_target_socket<SocketFallbackOwner, 32>;

  int calls = 0;
  sc_core::sc_time callback_time = sc_core::SC_ZERO_TIME;

  /** @brief Construct the native callback owner.
   *  @param name SystemC module name.
   */
  explicit SocketFallbackOwner(sc_core::sc_module_name name) : sc_module(name)
  {
  }

  /** @brief Mutate the borrowed transaction during native forward dispatch.
   *  @param index Native multi-socket connection index.
   *  @param payload Borrowed transaction, valid for the callback duration.
   *  @param phase Borrowed phase updated to BEGIN_RESP.
   *  @param delay Borrowed annotation updated to three nanoseconds.
   *  @return TLM_UPDATED after the in-place mutation.
   *  @details Called on the SystemC kernel thread. No Python callback or GIL
   *  transition is involved. The multi socket borrows this module pointer;
   *  Python must retain the owner through simulation.
   */
  tlm::tlm_sync_enum forward(int index, tlm::tlm_generic_payload &payload, tlm::tlm_phase &phase,
                             sc_core::sc_time &delay)
  {
    calls += index + 1;
    callback_time = sc_core::sc_time_stamp();
    payload.set_address(payload.get_address() + 16);
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(3, sc_core::SC_NS);
    return tlm::TLM_UPDATED;
  }

  /** @brief Return a genuine native member-pointer callback value.
   *  @return The upstream callback typedef for cppyy's direct registration
   * path.
   *  @details C++ owns only the pointer value; it does not own this module.
   */
  static Target::nb_cb native_callback()
  {
    return &SocketFallbackOwner::forward;
  }
};

/** @brief Own parent and child multi sockets for native hierarchy recovery. */
struct SocketFallbackConsumer : sc_core::sc_module
{
  using Target = tlm_utils::multi_passthrough_target_socket<SocketFallbackOwner, 32>;

  Target outer;
  Target inner;

  /** @brief Construct both native hierarchy endpoints.
   *  @param name SystemC module name.
   */
  explicit SocketFallbackConsumer(sc_core::sc_module_name name) : sc_module(name), outer("outer"), inner("inner")
  {
  }
};

/** @brief Issue one transaction independently from Python callback adapters. */
struct SocketFallbackInitiator : sc_core::sc_module
{
  tlm_utils::simple_initiator_socket<SocketFallbackInitiator, 32> socket;
  tlm::tlm_generic_payload payload;
  int calls = 0;
  sc_core::sc_time issue_time = sc_core::SC_ZERO_TIME;
  sc_core::sc_time annotation = sc_core::SC_ZERO_TIME;
  tlm::tlm_phase result_phase = tlm::UNINITIALIZED_PHASE;
  tlm::tlm_sync_enum result = tlm::TLM_ACCEPTED;

  SC_HAS_PROCESS(SocketFallbackInitiator);

  /** @brief Construct the initiator and register its native process.
   *  @param name SystemC module name.
   */
  explicit SocketFallbackInitiator(sc_core::sc_module_name name) : sc_module(name), socket("socket")
  {
    SC_THREAD(issue);
  }

  /** @brief Send one request at two nanoseconds and retain native results. */
  void issue()
  {
    wait(2, sc_core::SC_NS);
    issue_time = sc_core::sc_time_stamp();
    payload.set_address(100);
    result_phase = tlm::BEGIN_REQ;
    result = socket->nb_transport_fw(payload, result_phase, annotation);
    ++calls;
  }
};
"""
)

OWNER_TYPE = cppyy.gbl.SocketFallbackOwner
CONSUMER_TYPE = cppyy.gbl.SocketFallbackConsumer
INITIATOR_TYPE = cppyy.gbl.SocketFallbackInitiator
TARGET_TYPE = tlm_utils.multi_passthrough_target_socket[OWNER_TYPE, 32]

owner = OWNER_TYPE(sc.sc_module_name("callback_owner"))
owner_ref = weakref.ref(owner)
retained_owners = [owner]
del owner
gc.collect()
assert owner_ref() is retained_owners[0], "Python must retain the borrowed native callback owner"

consumer = CONSUMER_TYPE(sc.sc_module_name("consumer"))
initiator = INITIATOR_TYPE(sc.sc_module_name("initiator"))

# A non-native argument takes the public bind fallback and is rejected by the
# native overload resolver. The same socket remains usable for valid binding.
try:
    consumer.outer.bind(object())
except TypeError as error:
    assert "bind" in str(error)
else:
    raise AssertionError("native multi socket accepted a non-socket argument")

# cppyy exposes the exact upstream member pointer as its native callback proxy,
# outside the Python holder class. Registration reaches the original native
# overload, which cppyy currently cannot invoke; confirm this limitation and
# recover through the public exact-pointer holder adapter.
native_callback = OWNER_TYPE.native_callback()
assert not isinstance(native_callback, TARGET_TYPE.nb_cb)
assert "std::function<" in type(native_callback).__cpp_name__
try:
    TARGET_TYPE.register_nb_transport_fw(consumer.inner, owner_ref(), native_callback)
except NotImplementedError as error:
    assert "cannot (yet) be called" in str(error)
else:
    raise AssertionError("cppyy unexpectedly bypassed the documented callback holder")

callback_value = TARGET_TYPE.nb_cb.from_member("SocketFallbackOwner::forward")
consumer.inner.register_nb_transport_fw(owner_ref(), callback_value)
consumer.outer.bind(consumer.inner)
initiator.socket.bind(consumer.outer)

# Keep the native MODULE owner and both socket-owning modules alive until all
# kernel callbacks finish. sc_start releases the GIL while the native process
# and callback execute; neither callback calls back into Python.
sc.sc_start(sc.sc_time(5, sc.SC_NS))
assert initiator.calls == 1
assert initiator.issue_time == sc.sc_time(2, sc.SC_NS)
assert initiator.result == tlm.TLM_UPDATED
assert initiator.result_phase == tlm.BEGIN_RESP
assert initiator.annotation == sc.sc_time(3, sc.SC_NS)
assert initiator.payload.get_address() == 116
assert owner_ref().calls == 1
assert owner_ref().callback_time == sc.sc_time(2, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(5, sc.SC_NS)

print("SOCKET_NATIVE_FALLBACK_BIND_REJECTED_RECOVERED")
print("SOCKET_NATIVE_FALLBACK_CALLBACK_REJECTED_RECOVERED")
print("SOCKET_NATIVE_FALLBACK_RESULT 2 ns 2 ns 1 BEGIN_RESP 3 ns 116")
print("SOCKET_NATIVE_FALLBACK_OWNER_RETAINED")
sc.sc_stop()
