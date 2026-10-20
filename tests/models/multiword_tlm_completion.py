"""Verify multiword blocking TLM returns and caller-output overload completion."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy


class Owner:
    """Return deterministic values after a native service wait."""

    def __init__(self):
        """Initialize callback execution count."""
        self.calls = 0

    def get(self):
        """Wait one nanosecond and return one sample."""
        sc.wait(1, sc.SC_NS)
        self.calls += 1
        return 17

    def peek(self):
        """Wait one nanosecond and return a peek sample."""
        return self.get()


name = sc.sc_module_name("top")
top = sc.sc_module(name)
cases = []
for scalar in ("long long", "unsigned long long", "long double"):
    for contract in (tlm.tlm_blocking_get_if, tlm.tlm_blocking_peek_if, tlm.tlm_blocking_get_peek_if):
        owner = Owner()
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
 template<class T> bool get(tlm::tlm_blocking_get_if<T>& channel) {
   T output = 0;
   if (channel.get() != T(17)) return false;
   channel.get(output);
   return output == T(17);
 }
 template<class T> bool peek(tlm::tlm_blocking_peek_if<T>& channel) {
   T output = 0;
   if (channel.peek() != T(17)) return false;
   channel.peek(output);
   return output == T(17);
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
        + (f" && multiword_completion::peek<{scalar}>(channel)" if contract == tlm.tlm_blocking_get_peek_if else "")
        + "; }"
    )
    helper = getattr(cppyy.gbl, f"multiword_helper_{index}")
    helper.__release_gil__ = True
    helpers[id(port)] = helper
records = []


def consumer():
    """Check callback count and elapsed service time for every native type."""
    for scalar, contract, owner, port in cases:
        assert helpers[id(port)](port.get_interface())
        assert owner.calls == (4 if contract == tlm.tlm_blocking_get_peek_if else 2)
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(consumer, "consumer")
sc.sc_start(25, sc.SC_NS)
assert records == ["24 ns"], records
assert handle.terminated()
sc.sc_stop()
print("MULTIWORD_TLM_COMPLETION_OK")
