"""Compare compound master/slave blocking callbacks for asymmetric TLM types."""

from dataclasses import dataclass
import ctypes

from amal.eda.systemc import sc_core as sc, sc_dt, tlm


@dataclass(frozen=True)
class TypePair:
    """Describe one request/response specialization and its exact samples."""

    label: str
    request_name: str
    response_name: str


TYPE_PAIRS = (
    TypePair("int_long_long", "int", "long long"),
    TypePair("unsigned_long_long_long_double", "unsigned long long", "long double"),
    TypePair("biguint137_bigint128", "sc_dt::sc_biguint<137>", "sc_dt::sc_bigint<128>"),
)


def wide_value(native_type, bit, low):
    """Build a native wide integer with an asserted high bit and low payload."""
    value = native_type(low)
    value[bit] = True
    return value


def signed_wide_endpoint(native_type, maximum):
    """Build a signed 128-bit endpoint by setting individual native bits."""
    value = native_type(0)
    if maximum:
        for bit in range(127):
            value[bit] = True
    else:
        value[127] = True
    return value


def pair_values(spec):
    """Return two request and response samples, including endpoints/high bits."""
    if spec.label == "int_long_long":
        return (-(2**31), 2**31 - 1), (-(2**63), 2**63 - 1)
    if spec.label == "unsigned_long_long_long_double":
        return (2**64 - 1, 2**64 - 2), (1.25, -2.5)
    request_type = sc_dt.sc_biguint[137]
    response_type = sc_dt.sc_bigint[128]
    return (
        (wide_value(request_type, 136, 17), wide_value(request_type, 136, 29)),
        (signed_wide_endpoint(response_type, False), signed_wide_endpoint(response_type, True)),
    )


def copy_value(value):
    """Copy a native proxy while leaving immutable Python scalar values alone."""
    if hasattr(type(value), "__cpp_name__"):
        result = type(value)()
        result.__assign__(value)
        return result
    return value


def normalized(value):
    """Normalize native values for exact assertions and reference records."""
    if hasattr(value, "to_string"):
        return value.to_string()
    if hasattr(value, "value"):
        return value.value
    return value


def output_value(type_name):
    """Allocate caller-owned storage accepted by cppyy's native output overload."""
    if type_name == "int":
        return ctypes.c_int(0)
    if type_name == "long long":
        return ctypes.c_longlong(0)
    if type_name == "unsigned long long":
        return ctypes.c_ulonglong(0)
    if type_name == "long double":
        return ctypes.c_longdouble(0)
    if type_name == "sc_dt::sc_biguint<137>":
        return sc_dt.sc_biguint[137](0)
    return sc_dt.sc_bigint[128](0)


def sentinel(type_name):
    """Create a distinctive output value for failed nonblocking calls."""
    if type_name == "int":
        return ctypes.c_int(91)
    if type_name == "long long":
        return ctypes.c_longlong(91)
    if type_name == "unsigned long long":
        return ctypes.c_ulonglong(91)
    if type_name == "long double":
        return ctypes.c_longdouble(91.5)
    if type_name == "sc_dt::sc_biguint<137>":
        return wide_value(sc_dt.sc_biguint[137], 136, 91)
    return signed_wide_endpoint(sc_dt.sc_bigint[128], True)


def owner_values(spec, direction):
    """Select put and get types according to native master/slave semantics."""
    requests, responses = pair_values(spec)
    if direction == "master":
        return spec.request_name, spec.response_name, requests, responses
    return spec.response_name, spec.request_name, responses, requests


class Endpoint:
    """Retain copied put samples and serve timed get/peek values."""

    def __init__(self, put_samples, get_samples):
        """Initialize request/response samples and an observable callback log."""
        self.put_samples = put_samples
        self.get_samples = [copy_value(value) for value in get_samples]
        self.sent = []
        self.calls = []

    def put(self, value):
        """Wait one nanosecond and retain an independent request/response copy."""
        sc.wait(1, sc.SC_NS)
        self.calls.append("put")
        self.sent.append(copy_value(value))

    def nb_put(self, value):
        """Retain a copied nonblocking request and report success."""
        self.calls.append("nb_put")
        self.sent.append(copy_value(value))
        return True

    def nb_can_put(self):
        """Report available nonblocking put capacity."""
        self.calls.append("nb_can_put")
        return True

    def get(self):
        """Wait one nanosecond, record the callback and consume one value."""
        sc.wait(1, sc.SC_NS)
        self.calls.append("get")
        return self.get_samples.pop(0)

    def peek(self):
        """Wait one nanosecond and return the next value without consuming it."""
        sc.wait(1, sc.SC_NS)
        self.calls.append("peek")
        return self.get_samples[0]

    def nb_get(self):
        """Return and consume the next sample, or report empty without a value."""
        self.calls.append("nb_get")
        return (True, self.get_samples.pop(0)) if self.get_samples else (False, None)

    def nb_peek(self):
        """Return the next sample without consuming it, or report an empty queue."""
        self.calls.append("nb_peek")
        return (True, self.get_samples[0]) if self.get_samples else (False, None)

    def nb_can_get(self):
        """Report whether a sample remains for a nonblocking get."""
        self.calls.append("nb_can_get")
        return bool(self.get_samples)

    def nb_can_peek(self):
        """Report whether a sample remains for a nonblocking peek."""
        self.calls.append("nb_can_peek")
        return bool(self.get_samples)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
cases = []
for spec in TYPE_PAIRS:
    for direction in ("master", "slave"):
        put_type_name, get_type_name, put_samples, get_samples = owner_values(spec, direction)
        for mode, family in (("combined", ""), ("blocking", "blocking_")):
            interface = getattr(tlm, f"tlm_{family}{direction}_if")[spec.request_name, spec.response_name]
            port = sc.sc_port[interface](f"{spec.label}_{direction}_{mode}")
            owner = Endpoint(put_samples, get_samples)
            port(owner)
            cases.append((spec, direction, mode, put_type_name, get_type_name, put_samples, get_samples, port, owner))
del name


def exercise_case(case):
    """Assert blocking values, output overloads, copies, and full-family NB paths."""
    spec, direction, mode, put_name, get_name, put_samples, get_samples, port, owner = case
    interface = port.get_interface()
    put_expected = normalized(put_samples[0])
    source = copy_value(put_samples[0])
    interface.put(source)
    if hasattr(source, "__cpp_name__"):
        if spec.label == "biguint137_bigint128":
            source[136] = False
        else:
            source.__assign__(type(source)(0))
    else:
        source = 0
    assert normalized(owner.sent[0]) == put_expected

    first, second = get_samples
    assert normalized(interface.peek()) == normalized(first)
    peek_output = output_value(get_name)
    interface.peek(peek_output)
    assert normalized(peek_output) == normalized(first)
    assert normalized(interface.get()) == normalized(first)
    get_output = output_value(get_name)
    interface.get(get_output)
    assert normalized(get_output) == normalized(second)

    expected_calls = ["put", "peek", "peek", "get", "get"]
    if mode == "combined":
        owner.get_samples[:] = [copy_value(first), copy_value(second)]
        assert interface.nb_can_get() and interface.nb_can_peek()
        nb_peek_output = output_value(get_name)
        assert interface.nb_peek(nb_peek_output)
        assert normalized(nb_peek_output) == normalized(first)
        nb_get_output = output_value(get_name)
        assert interface.nb_get(nb_get_output)
        assert normalized(nb_get_output) == normalized(first)
        nb_peek_output = output_value(get_name)
        assert interface.nb_peek(nb_peek_output)
        assert normalized(nb_peek_output) == normalized(second)
        nb_get_output = output_value(get_name)
        assert interface.nb_get(nb_get_output)
        assert normalized(nb_get_output) == normalized(second)
        assert not interface.nb_can_get() and not interface.nb_can_peek()
        for method in (interface.nb_get, interface.nb_peek):
            failed_output = sentinel(get_name)
            before = normalized(failed_output)
            assert not method(failed_output)
            assert normalized(failed_output) == before
        expected_calls.extend(
            [
                "nb_can_get",
                "nb_can_peek",
                "nb_peek",
                "nb_get",
                "nb_peek",
                "nb_get",
                "nb_can_get",
                "nb_can_peek",
                "nb_get",
                "nb_peek",
            ]
        )
    assert owner.calls == expected_calls, (spec.label, direction, mode, owner.calls)
    assert len(owner.sent) == 1 and normalized(owner.sent[0]) == put_expected
    return f"MASTER_SLAVE_CASE {spec.label} {direction} {mode} {len(owner.calls)} {sc.sc_time_stamp()}"


records = []


def exercise():
    """Run all twelve asymmetric interface specializations in scheduler order."""
    records.extend(exercise_case(case) for case in cases)


handle = sc.sc_spawn(exercise, "exercise")
sc.sc_start(61, sc.SC_NS)
assert handle.terminated()
assert sc.sc_time_stamp() == sc.sc_time(61, sc.SC_NS), sc.sc_time_stamp()
sc.sc_stop()
for record in records:
    print(record)
print("MASTER_SLAVE_TYPE_MATRIX_OK")
