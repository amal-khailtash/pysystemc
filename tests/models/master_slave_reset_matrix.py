"""Check master/slave blocking callback cleanup during scheduler reset and kill."""

import sys

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


TYPES = (
    ("int", int),
    ("long_long", "long long"),
    ("long_double", "long double"),
    ("biguint137", sc_dt.sc_biguint[137]),
)
CPP_TYPE_NAMES = {
    "int": "int",
    "long_long": "long long",
    "long_double": "long double",
    "biguint137": "sc_dt::sc_biguint<137>",
}
WAIT_KINDS = ("time", "event", "or", "and", "timed-event", "timed-or", "timed-and", "cycle", "count")
PATHS = ("put", "get", "peek", "get-output", "peek-output")
FAMILIES = (
    ("blocking-master", "master", False),
    ("full-master", "master", True),
    ("blocking-slave", "slave", False),
    ("full-slave", "slave", True),
)


def sample(native_type):
    """Return a stable nonzero value, including a high bit for the wide case."""
    value = native_type(73)
    if native_type == sc_dt.sc_biguint[137]:
        value[136] = True
    return value


def normalized(value):
    """Convert native proxies to values suitable for Python assertions."""
    if hasattr(value, "to_string"):
        return value.to_string()
    return value.value if hasattr(value, "value") else value


def expected_sample(type_label):
    """Return the exact callback sample for a public matrix type."""
    if type_label == "biguint137":
        return sample(sc_dt.sc_biguint[137])
    return 73.0 if type_label == "long_double" else 73


def wait_for(kind, first, second):
    """Suspend through the requested SystemC wait overload."""
    if kind == "time":
        sc.wait(sc.sc_time(100, sc.SC_NS))
    elif kind == "event":
        sc.wait(first)
    elif kind == "or":
        sc.wait(first | second)
    elif kind == "and":
        sc.wait(first & second)
    elif kind == "timed-event":
        sc.wait(sc.sc_time(100, sc.SC_NS), first)
    elif kind == "timed-or":
        sc.wait(sc.sc_time(100, sc.SC_NS), first | second)
    elif kind == "timed-and":
        sc.wait(sc.sc_time(100, sc.SC_NS), first & second)
    elif kind == "cycle":
        sc.wait()
    else:
        sc.wait(100)


def install_output_helper(interface_name, type_name, helper_name):
    """Compile a GIL-releasing native caller that checks output before rethrow."""
    cppyy.cppdef(
        f"void {helper_name}({interface_name}& channel, bool do_get) {{ "
        f"{type_name} output = static_cast<{type_name}>(73); const {type_name} sentinel = output; "
        "try { "
        "if (do_get) channel.get(output); else channel.peek(output); "
        'throw std::runtime_error("blocked master/slave output returned"); '
        "} catch (const sc_core::sc_unwind_exception& unwind) { "
        "const bool reset = sc_core::sc_time_stamp() == sc_core::sc_time(2, sc_core::SC_NS); "
        "if (output != sentinel || unwind.is_reset() != reset || "
        "(reset && sc_core::sc_time_stamp() != sc_core::sc_time(2, sc_core::SC_NS)) || "
        "(!reset && sc_core::sc_time_stamp() != sc_core::sc_time(3, sc_core::SC_NS))) "
        'throw std::runtime_error("master/slave output changed or unwind kind/time mismatch"); '
        "throw; } }"
    )
    helper = getattr(cppyy.gbl, helper_name)
    helper.__release_gil__ = True
    return helper


if len(sys.argv) == 3:
    wait_start, wait_end = map(int, sys.argv[1:])
else:
    wait_start, wait_end = 0, len(WAIT_KINDS)
assert 0 <= wait_start < wait_end <= len(WAIT_KINDS)
selected_waits = WAIT_KINDS[wait_start:wait_end]

name = sc.sc_module_name("master_slave_reset_top")
top = sc.sc_module(name)
clock = sc.sc_clock("matrix_clock", sc.sc_time(100, sc.SC_NS), 0.5, sc.sc_time(50, sc.SC_NS), True)
first_event = sc.sc_event("first_event")
second_event = sc.sc_event("second_event")
records = {}
handles = []
cases = []
output_helpers = {}
compiled_output_helpers = {}


class Owner:
    """Implement blocking callbacks that record cleanup around a native wait."""

    def __init__(self, key, kind, expected):
        """Retain case identity, wait selection, and the copied put value."""
        self.key = key
        self.kind = kind
        self.expected = expected
        self.calls = 0
        self.calls_log = records[key]

    def block(self, operation, value=None):
        """Validate callback arguments and record wait cleanup in scheduler order."""
        self.calls += 1
        self.calls_log.append(("callback-start", str(sc.sc_time_stamp()), operation))
        if operation == "put":
            assert normalized(value) == normalized(self.expected), (self.key, value, self.expected)
        try:
            wait_for(self.kind, first_event, second_event)
        finally:
            self.calls_log.append(("callback-cleanup", str(sc.sc_time_stamp()), operation))

    def put(self, value):
        """Block while accepting the native master/slave request value."""
        self.block("put", value)

    def get(self):
        """Block and return the response sample after normal completion."""
        self.block("get")
        return self.expected

    def peek(self):
        """Block and return the peek sample after normal completion."""
        self.block("peek")
        return self.expected

    def nb_put(self, value):
        """Reject unused nonblocking puts in full interfaces."""
        return False

    def nb_get(self):
        """Report an empty unused nonblocking get."""
        return False, self.expected

    def nb_peek(self):
        """Report an empty unused nonblocking peek."""
        return False, self.expected

    def nb_can_put(self):
        """Report no unused nonblocking put capacity."""
        return False

    def nb_can_get(self):
        """Report no unused nonblocking get value."""
        return False

    def nb_can_peek(self):
        """Report no unused nonblocking peek value."""
        return False


def worker(index, key, interface, path, expected):
    """Invoke one blocking path and preserve the outer process cleanup record."""
    log = records[key]
    log.append(("worker-start", str(sc.sc_time_stamp()), path))
    try:
        if path == "put":
            interface.put(expected)
        elif path == "get":
            interface.get()
        elif path == "peek":
            interface.peek()
        else:
            output_helpers[index](interface, path == "get-output")
        raise AssertionError(("interrupted blocking operation returned", key))
    finally:
        log.append(("worker-cleanup", str(sc.sc_time_stamp()), path))


for type_label, type_spec in TYPES:
    expected = expected_sample(type_label)
    for family_label, direction, full in FAMILIES:
        family = "" if full else "blocking_"
        interface_type = getattr(tlm, f"tlm_{family}{direction}_if")[type_spec, type_spec]
        interface_name = interface_type.__cpp_name__
        for path in PATHS:
            for wait_index, kind in enumerate(selected_waits, wait_start):
                key = (type_label, family_label, path, kind)
                case_index = len(cases)
                records[key] = []
                port = sc.sc_port[interface_type]("channel_" + "_".join(key))
                owner = Owner(key, kind, expected)
                port(owner)
                channel = port.get_interface()
                if path in ("get-output", "peek-output"):
                    helper_key = (type_label, family_label)
                    if helper_key not in compiled_output_helpers:
                        helper_name = f"ms_reset_output_{type_label}_{family_label.replace('-', '_')}"
                        compiled_output_helpers[helper_key] = install_output_helper(
                            interface_name, CPP_TYPE_NAMES[type_label], helper_name
                        )
                    output_helpers[case_index] = compiled_output_helpers[helper_key]
                options = sc.sc_spawn_options()
                options.set_sensitivity(clock.posedge_event())
                handle = sc.sc_spawn(
                    lambda index=case_index, key=key, channel=channel, path=path, expected=expected: worker(
                        index, key, channel, path, expected
                    ),
                    "worker_" + "_".join(key),
                    options,
                )
                handles.append(handle)
                cases.append((case_index, key, owner))
del name


def controller():
    """Reset all blocked callbacks at 2 ns, then kill their restarted workers at 3 ns."""
    sc.wait(2, sc.SC_NS)
    for handle in handles:
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles:
        handle.kill()


controller_handle = sc.sc_spawn(controller, "controller")
sc.sc_start(4, sc.SC_NS)
expected_events = [
    ("worker-start", "0 s"),
    ("callback-start", "0 s"),
    ("callback-cleanup", "2 ns"),
    ("worker-cleanup", "2 ns"),
    ("worker-start", "2 ns"),
    ("callback-start", "2 ns"),
    ("callback-cleanup", "3 ns"),
    ("worker-cleanup", "3 ns"),
]
case_records = []
for case_index, key, owner in cases:
    log = records[key]
    assert [(event, timestamp) for event, timestamp, _ in log] == expected_events, (key, log)
    assert owner.calls == 2, (key, owner.calls)
    case_records.append(f"MASTER_SLAVE_RESET_CASE {' '.join(key)} {owner.calls} 2 ns 3 ns")
assert sc.sc_time_stamp() == sc.sc_time(4, sc.SC_NS)
assert all(handle.terminated() for handle in handles + [controller_handle])
for record in sorted(case_records):
    print(record)
print(f"MASTER_SLAVE_RESET_MATRIX_OK {len(case_records)} {selected_waits[0]}..{selected_waits[-1]}")
