"""Verify multiword blocking TLM returns and caller-output overload completion."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


class Owner:
    """Return deterministic values after a native service wait."""

    def __init__(self, wide=False):
        """Initialize callback execution count."""
        self.nb_failure = None
        self.available = False
        self.wide = wide
        self.calls = 0
        self.invalid = False
        self.raise_error = False

    def get(self):
        """Wait one nanosecond and return one sample."""
        if self.raise_error:
            raise RuntimeError("configured TLM failure")
        if self.invalid:
            return object()
        sc.wait(1, sc.SC_NS)
        self.calls += 1
        if self.wide:
            value = sc_dt.sc_biguint[137](17)
            value[136] = True
            return value
        return 17

    def peek(self):
        """Wait one nanosecond and return a peek sample."""
        return self.get()

    def nb_get(self):
        """Return the configured availability and a copied sample."""
        if self.nb_failure == "raise":
            raise RuntimeError("configured nonblocking failure")
        if self.nb_failure == "tuple":
            return 1, 17
        if self.nb_failure == "value":
            return True, object()
        if self.available:
            value = sc_dt.sc_biguint[137](17) if self.wide else 17
            if self.wide:
                value[136] = True
            return True, value
        return False, None

    def nb_peek(self):
        """Return the configured nonblocking peek sample."""
        return self.nb_get()

    def nb_can_get(self):
        """Report configured immediate get readiness."""
        return self.available

    def nb_can_peek(self):
        """Report configured immediate peek readiness."""
        return self.available


name = sc.sc_module_name("top")
top = sc.sc_module(name)
cases = []
for scalar in ("long long", "unsigned long long", "long double", "sc_dt::sc_biguint<137>"):
    for contract in (
        tlm.tlm_blocking_get_if,
        tlm.tlm_blocking_peek_if,
        tlm.tlm_blocking_get_peek_if,
        tlm.tlm_get_peek_if,
    ):
        owner = Owner(scalar == "sc_dt::sc_biguint<137>")
        port = sc.sc_port[contract[scalar]]()
        port(owner)
        cases.append((scalar, contract, owner, port))
del name
cppyy.cppdef("""
/** Check blocking native overloads while retaining caller-owned output.
 * Ports retain Python owners. Callers release GIL around waits; callback
 * adapters reacquire it. Exceptions propagate and no output reference escapes.
 */
namespace multiword_completion {
 /** Preserve borrowed output on conversion or owner exceptions; clear errors under GIL. */
 template<class T> bool rejected_get(tlm::tlm_blocking_get_if<T>& channel) {
   T output = 71;
   try { channel.get(output); } catch (...) {
     auto gil = PyGILState_Ensure(); PyErr_Clear(); PyGILState_Release(gil);
     return output == T(71);
   }
   return false;
 }
 template<class T> bool rejected_peek(tlm::tlm_blocking_peek_if<T>& channel) {
   T output = 71;
   try { channel.peek(output); } catch (...) {
     auto gil = PyGILState_Ensure(); PyErr_Clear(); PyGILState_Release(gil);
     return output == T(71);
   }
   return false;
 }
 /** Check false readiness and unchanged caller output without waiting. */
 template<class T> bool unavailable(tlm::tlm_get_peek_if<T>& channel) {
   T get_output = 71, peek_output = 73;
   return !channel.nb_can_get() && !channel.nb_can_peek()
     && !channel.nb_get(get_output) && !channel.nb_peek(peek_output)
     && get_output == T(71) && peek_output == T(73);
 }
 /** Preserve both borrowed outputs when nonblocking callbacks reject. */
 template<class T> bool rejected_nonblocking(tlm::tlm_get_peek_if<T>& channel) {
   T get_output = 71, peek_output = 73;
   bool get_failed = false, peek_failed = false;
   try { channel.nb_get(get_output); } catch (...) {
     auto gil = PyGILState_Ensure(); PyErr_Clear(); PyGILState_Release(gil); get_failed = true;
   }
   try { channel.nb_peek(peek_output); } catch (...) {
     auto gil = PyGILState_Ensure(); PyErr_Clear(); PyGILState_Release(gil); peek_failed = true;
   }
   return get_failed && peek_failed && get_output == T(71) && peek_output == T(73);
 }
 template<class T> T expected() { return T(17); }
 template<> sc_dt::sc_biguint<137> expected<sc_dt::sc_biguint<137>>() {
   sc_dt::sc_biguint<137> value = 17; value[136] = true; return value;
 }
 /** Check successful nonblocking copies without scheduler waits. */
 template<class T> bool available(tlm::tlm_get_peek_if<T>& channel) {
   T get_output = 71, peek_output = 73;
   return channel.nb_can_get() && channel.nb_can_peek()
     && channel.nb_get(get_output) && channel.nb_peek(peek_output)
     && get_output == expected<T>() && peek_output == expected<T>();
 }
 template<class T> bool get(tlm::tlm_blocking_get_if<T>& channel) {
   T output = 0;
   if (channel.get() != expected<T>()) return false;
   channel.get(output);
   return output == expected<T>();
 }
 template<class T> bool peek(tlm::tlm_blocking_peek_if<T>& channel) {
   T output = 0;
   if (channel.peek() != expected<T>()) return false;
   channel.peek(output);
   return output == expected<T>();
 }
}
""")
helpers = {}
for index, (scalar, contract, owner, port) in enumerate(cases):
    method = "peek" if contract == tlm.tlm_blocking_peek_if else "get"
    cppyy.cppdef(
        f"/** Borrow interface during blocking service; caller releases GIL, adapter reacquires it. */ "
        f"bool multiword_helper_{index}(tlm::{contract.__cpp_name__.split('::')[-1]}<{scalar}>& channel) "
        f"{{ return multiword_completion::{method}<{scalar}>(channel)"
        + (
            f" && multiword_completion::peek<{scalar}>(channel)"
            if contract in (tlm.tlm_blocking_get_peek_if, tlm.tlm_get_peek_if)
            else ""
        )
        + "; }"
    )
    cppyy.cppdef(
        f"/** Inspect callback failure with borrowed output; clear Python error under GIL. */ "
        f"bool multiword_reject_{index}(tlm::{contract.__cpp_name__.split('::')[-1]}<{scalar}>& channel) "
        f"{{ return multiword_completion::rejected_{method}<{scalar}>(channel)"
        + (
            f" && multiword_completion::rejected_peek<{scalar}>(channel)"
            if contract in (tlm.tlm_blocking_get_peek_if, tlm.tlm_get_peek_if)
            else ""
        )
        + "; }"
    )
    cppyy.cppdef(
        f"/** Check borrowed nonblocking outputs; callbacks do not wait. */ "
        f"bool multiword_unavailable_{index}(tlm::tlm_get_peek_if<{scalar}>& channel) "
        f"{{ return multiword_completion::unavailable<{scalar}>(channel); }}"
    )
    assert getattr(cppyy.gbl, f"multiword_unavailable_{index}")(port.get_interface())
    cppyy.cppdef(
        f"/** Check nonblocking output copies while owners remain retained. */ "
        f"bool multiword_available_{index}(tlm::tlm_get_peek_if<{scalar}>& channel) "
        f"{{ return multiword_completion::available<{scalar}>(channel); }}"
    )
    cppyy.cppdef(
        f"/** Inspect nonblocking errors under GIL without retaining caller outputs. */ "
        f"bool multiword_nb_reject_{index}(tlm::tlm_get_peek_if<{scalar}>& channel) "
        f"{{ return multiword_completion::rejected_nonblocking<{scalar}>(channel); }}"
    )
    for failure in ("raise", "tuple", "value"):
        owner.nb_failure = failure
        assert getattr(cppyy.gbl, f"multiword_nb_reject_{index}")(port.get_interface())
    owner.nb_failure = None
    owner.available = True
    before = sc.sc_time_stamp().value()
    assert getattr(cppyy.gbl, f"multiword_available_{index}")(port.get_interface())
    assert sc.sc_time_stamp().value() == before
    owner.available = False
    for failure in ("invalid", "raise_error"):
        setattr(owner, failure, True)
        assert getattr(cppyy.gbl, f"multiword_reject_{index}")(port.get_interface())
        setattr(owner, failure, False)
    helper = getattr(cppyy.gbl, f"multiword_helper_{index}")
    helper.__release_gil__ = True
    helpers[id(port)] = helper
records = []


def consumer():
    """Check callback count and elapsed service time for every native type."""
    for scalar, contract, owner, port in cases:
        assert helpers[id(port)](port.get_interface())
        assert owner.calls == (4 if contract in (tlm.tlm_blocking_get_peek_if, tlm.tlm_get_peek_if) else 2)
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(consumer, "consumer")
sc.sc_start(49, sc.SC_NS)
assert records == ["48 ns"], records
assert handle.terminated()
sc.sc_stop()
print("MULTIWORD_TLM_COMPLETION_OK")
