"""Exercise standalone get/peek adapters without full-interface auto-upgrade."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


class Owner:
    """Expose only the requested operation while retaining callback state."""

    def __init__(self, operation, wide):
        """Install one blocking/nonblocking callback family."""
        self.calls = 0
        self.failure = False
        self.invalid = False
        self.available = False
        self.nb_failure = None
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
        if self.invalid:
            return object()
        sc.wait(1, sc.SC_NS)
        self.calls += 1
        return self.value()

    def try_sample(self):
        """Copy a ready value without waiting."""
        if self.failure:
            raise RuntimeError("partial nonblocking failure")
        if self.nb_failure == "flag":
            return 1, self.value()
        if self.nb_failure == "value":
            return True, object()
        if self.nb_failure == "shape":
            return (True,)
        return (True, self.value()) if self.available else (False, None)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
cases = []
ready_seen = []
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
/** Reject value-returning callbacks; clear Python errors with GIL held. */
bool partial_value_reject_{index}(tlm::tlm_{operation}_if<{scalar}>& channel) {{
 try {{ channel.{operation}(); }}
 catch (...) {{ auto gil = PyGILState_Ensure(); PyErr_Clear(); PyGILState_Release(gil);
   return true; }}
 return false;
}}
/** Verify inherited interfaces borrow the adapter-owned readiness event. */
bool partial_event_identity_{index}(tlm::tlm_{operation}_if<{scalar}>& channel) {{
 tlm::tlm_nonblocking_{operation}_if<{scalar}>& base = channel;
 return &channel.ok_to_{operation}() == &base.ok_to_{operation}();
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
        assert getattr(cppyy.gbl, f"partial_event_identity_{index}")(native)
        event = getattr(native, "ok_to_" + operation)()

        def observe_ready(index=index):
            """Record exactly one delayed notification without initialization."""
            ready_seen.append((index, str(sc.sc_time_stamp())))

        top.sc_method(f"ready_{index}", observe_ready)
        top.sensitive(event)
        top.dont_initialize()
        native.notify_ready(sc.sc_time(1, sc.SC_NS))
        nb = getattr(cppyy.gbl, f"partial_nb_{index}")
        assert nb(native, False)
        owner.available = True
        assert nb(native, True)
        owner.available = False
        assert nb(native, False)
        owner.available = True
        label = ("long_long", "unsigned_long_long", "long_double", "biguint137")[index // 2]
        print(f"TLM_NB {label} {operation} 0 1 1")
        print(f"TLM_EVENT {label} {operation} 1")
        owner.failure = True
        reject = getattr(cppyy.gbl, f"partial_reject_{index}")
        assert reject(native, True) and reject(native, False)
        value_reject = getattr(cppyy.gbl, f"partial_value_reject_{index}")
        assert value_reject(native)
        owner.failure = False
        for failure in ("flag", "value", "shape"):
            owner.nb_failure = failure
            assert reject(native, False)
            owner.nb_failure = None
            assert nb(native, True)
        owner.invalid = True
        assert value_reject(native) and reject(native, True)
        owner.invalid = False
        helper = getattr(cppyy.gbl, f"partial_complete_{index}")
        helper.__release_gil__ = True
        cases.append((owner, port, helper))
del name
completed = []


def consume():
    """Assert both native overloads complete and each callback runs twice."""
    for index, (owner, port, helper) in enumerate(cases):
        assert helper(port.get_interface())
        assert owner.calls == 2
        label = ("long_long", "unsigned_long_long", "long_double", "biguint137")[index // 2]
        operation = "get" if index % 2 == 0 else "peek"
        print(f"TLM_VALUE {label} {operation} 1 1")
    completed.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(consume, "consumer")
sc.sc_start(17, sc.SC_NS)
assert completed == ["16 ns"], completed
assert sorted(ready_seen) == [(index, "1 ns") for index in range(len(cases))], ready_seen
assert handle.terminated()
sc.sc_stop()
print("PARTIAL_MULTIWORD_TLM_OK")
