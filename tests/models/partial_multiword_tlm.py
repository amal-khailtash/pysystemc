"""Exercise standalone get/peek adapters without full-interface auto-upgrade."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


class Owner:
    """Expose only the requested operation while retaining callback state."""

    def __init__(self, operation, wide):
        """Install one blocking/nonblocking callback family."""
        self.calls = 0
        self.failure = False
        self.available = False
        self.wide = wide
        setattr(self, operation, self.sample)
        setattr(self, "nb_" + operation, self.try_sample)
        setattr(self, "nb_can_" + operation, lambda: self.available)

    def value(self):
        """Return a scalar or a sample with bit 136 set."""
        if not self.wide:
            return 17
        result = sc_dt.sc_biguint[137](17)
        result[136] = True
        return result

    def sample(self):
        """Wait one nanosecond for a sample, or reject before touching output."""
        if self.failure:
            raise RuntimeError("partial callback failure")
        sc.wait(1, sc.SC_NS)
        self.calls += 1
        return self.value()

    def try_sample(self):
        """Copy a ready value without waiting."""
        if self.failure:
            raise RuntimeError("partial nonblocking failure")
        return (True, self.value()) if self.available else (False, None)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
cases = []
for scalar in ("long long", "unsigned long long", "long double", "sc_dt::sc_biguint<137>"):
    for operation in ("get", "peek"):
        owner = Owner(operation, scalar.startswith("sc_dt"))
        interface = getattr(tlm, "tlm_" + operation + "_if")[scalar]
        port = sc.sc_port[interface]()
        port(owner)
        # A partial owner must not accidentally select the full get/peek bridge.
        index = len(cases)
        expected = "T(17)"
        if owner.wide:
            expected = "([] { T value = 17; value[136] = true; return value; }())"
        cppyy.cppdef(f"""
/** Borrow partial interface; blocking callers release GIL and callbacks reacquire it.
 * Errors leave caller output intact. No references to output survive this call.
 */
bool partial_complete_{index}(tlm::tlm_{operation}_if<{scalar}>& channel) {{
 using T = {scalar}; T output = 71; T expected = {expected};
 if (dynamic_cast<tlm::tlm_get_peek_if<T>*>(&channel)) return false;
 if (channel.{operation}() != expected) return false;
 channel.{operation}(output); return output == expected;
}}
/** Check readiness and copied output without scheduler activity. */
bool partial_nb_{index}(tlm::tlm_{operation}_if<{scalar}>& channel, bool ready) {{
 using T = {scalar}; T output = 71; T expected = {expected};
 return channel.nb_can_{operation}() == ready && channel.nb_{operation}(output) == ready
   && output == (ready ? expected : T(71));
}}
/** Catch owner failures and clear Python errors with GIL held. */
bool partial_reject_{index}(tlm::tlm_{operation}_if<{scalar}>& channel, bool blocking) {{
 using T = {scalar}; T output = 71;
 try {{ if (blocking) channel.{operation}(output); else channel.nb_{operation}(output); }}
 catch (...) {{ auto gil = PyGILState_Ensure(); PyErr_Clear(); PyGILState_Release(gil);
   return output == T(71); }}
 return false;
}}
""")
        native = port.get_interface()
        nb = getattr(cppyy.gbl, f"partial_nb_{index}")
        assert nb(native, False)
        owner.available = True
        assert nb(native, True)
        owner.failure = True
        reject = getattr(cppyy.gbl, f"partial_reject_{index}")
        assert reject(native, True) and reject(native, False)
        owner.failure = False
        helper = getattr(cppyy.gbl, f"partial_complete_{index}")
        helper.__release_gil__ = True
        cases.append((owner, port, helper))
del name
completed = []


def consume():
    """Assert both native overloads complete and each callback runs twice."""
    for owner, port, helper in cases:
        assert helper(port.get_interface())
        assert owner.calls == 2
    completed.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(consume, "consumer")
sc.sc_start(17, sc.SC_NS)
assert completed == ["16 ns"], completed
assert handle.terminated()
sc.sc_stop()
print("PARTIAL_MULTIWORD_TLM_OK")
