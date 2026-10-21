"""Check FIFO debug indexing, output retention and native configuration defaults."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import ctypes
import cppyy
import sys

value_type, output_type = {
    "int": (int, ctypes.c_int),
    "long_long": ("long long", ctypes.c_longlong),
    "unsigned_long_long": ("unsigned long long", ctypes.c_ulonglong),
    "long_double": ("long double", ctypes.c_longdouble),
    "bigint128": (sc_dt.sc_bigint[128], sc_dt.sc_bigint[128]),
    "biguint128": (sc_dt.sc_biguint[128], sc_dt.sc_biguint[128]),
    "biguint137": (sc_dt.sc_biguint[137], sc_dt.sc_biguint[137]),
}[sys.argv[1] if len(sys.argv) > 1 else "int"]


def sample(value):
    """Copy callback arguments without truncating arbitrary-width native values."""
    if not isinstance(value_type, str) and value_type is not int:
        result = value_type()
        result.__assign__(value)
        return result
    return value


def output_value(value):
    """Read either borrowed scalar output or native arbitrary-width storage."""
    return value.value if isinstance(value, ctypes._SimpleCData) else value


def same_wide(actual, expected):
    """Compare exact native wide representations without cppyy equality coercion."""
    return actual.to_string() == expected.to_string()


cppyy.cppdef("""
/** @brief Inspect FIFO overloads through borrowed native base interfaces.
 * @details No waits or GIL release are needed. Python callbacks acquire GIL;
 * owners and output storage remain alive. Callback exceptions propagate.
 */
/** Borrow virtual interface addresses without invoking callbacks. */
std::uintptr_t fifo_compound_identity(sc_core::sc_interface* value) {
    return reinterpret_cast<std::uintptr_t>(value);
}
bool fifo_base_reference(tlm::tlm_get_peek_if<int>& ordinary,
                        tlm::tlm_fifo_debug_if<int>& debug)
{
    int oldest = -1, newest = -1;
    return ordinary.nb_peek(oldest) && debug.nb_peek(newest, 0)
        && oldest == 23 && newest == 29;
}
""")


class Debug:
    """Expose newest-first debug values without simulation notifications."""

    def __init__(self):
        """Retain samples and debug callback counts."""
        self.values = [41, 23]
        self.calls = 0

    def used(self):
        """Return occupied entries."""
        return len(self.values)

    def size(self):
        """Return native FIFO capacity."""
        return 4

    def debug(self):
        """Record a debug inspection without changing entries."""
        self.calls += 1

    def nb_peek(self, index=None):
        """Return an indexed sample or failure without changing values."""
        if index is None:
            return (True, self.values[-1]) if self.values else (False, 0)
        return (True, self.values[index]) if 0 <= index < len(self.values) else (False, 0)

    def put(self, value):
        """Append after one native wait, sharing state with debug access."""
        sc.wait(1, sc.SC_NS)
        self.values.insert(0, sample(value))

    def nb_put(self, value):
        """Append without waiting if capacity allows."""
        if len(self.values) == 4:
            return False
        self.values.insert(0, sample(value))
        return True

    def nb_can_put(self):
        """Report capacity."""
        return len(self.values) < 4

    def get(self):
        """Consume oldest entry."""
        return self.values.pop()

    def peek(self):
        """Read oldest entry."""
        return self.values[-1]

    def nb_get(self):
        """Consume only if available."""
        return (True, self.values.pop()) if self.values else (False, 0)

    def nb_can_get(self):
        """Report available entries."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report peek availability."""
        return bool(self.values)

    def nb_poke(self, value, index):
        """Replace an indexed entry, rejecting out-of-range writes."""
        if not 0 <= index < len(self.values):
            return False
        self.values[index] = sample(value)
        return True


class Config:
    """Record configuration calls and their native default arguments."""

    def __init__(self):
        """Create an ordered call record."""
        self.calls = []

    def nb_expand(self, amount):
        """Record expansion."""
        self.calls.append(("expand", amount))

    def nb_unbound(self, amount):
        """Record unbounded capacity increment."""
        self.calls.append(("unbound", amount))

    def nb_reduce(self, amount):
        """Record a successful reduction."""
        self.calls.append(("reduce", amount))
        return True

    def nb_bound(self, amount):
        """Record a rejected bound."""
        self.calls.append(("bound", amount))
        return False


name = sc.sc_module_name("top")
top = sc.sc_module(name)
debug, config = Debug(), Config()
debug_port = sc.sc_port[tlm.tlm_fifo_debug_if[value_type]]("debug")
config_port = sc.sc_port[tlm.tlm_fifo_config_size_if]("config")
debug_port(debug)
config_port(config)
put_port = sc.sc_port[tlm.tlm_fifo_put_if[value_type]]("put")
get_port = sc.sc_port[tlm.tlm_fifo_get_if[value_type]]("get")
put_port(debug)
get_port(debug)
assert len({cppyy.gbl.fifo_compound_identity(port.get_interface()) for port in (debug_port, put_port, get_port)}) == 1
# Both compound-first orders must retain the same virtual debug base.
identity_owners, identity_ports = [], []
for order in (
    (tlm.tlm_fifo_get_if, tlm.tlm_fifo_put_if, tlm.tlm_fifo_debug_if),
    (tlm.tlm_fifo_put_if, tlm.tlm_fifo_debug_if, tlm.tlm_fifo_get_if),
):
    identity_owner = Debug()
    ports = []
    for contract in order:
        port = sc.sc_port[contract[value_type]]()
        port(identity_owner)
        ports.append(port)
    assert len({cppyy.gbl.fifo_compound_identity(port.get_interface()) for port in ports}) == 1
    ordinary = sc.sc_port[tlm.tlm_get_peek_if[value_type]]()
    ordinary(identity_owner)
    assert cppyy.gbl.fifo_compound_identity(ordinary.get_interface()) == cppyy.gbl.fifo_compound_identity(
        ports[0].get_interface()
    )
    ports.append(ordinary)
    identity_owners.append(identity_owner)
    identity_ports.extend(ports)
del name
interface = debug_port.get_interface()
assert interface.used() == 2 and interface.size() == 4
interface.debug()
assert debug.calls == 1
value = output_type(71)
assert interface.nb_peek(value, 0) and output_value(value) == 41
assert interface.nb_peek(value, 1) and output_value(value) == 23
assert not interface.nb_peek(value, 2) and output_value(value) == 23
assert interface.nb_poke(17)
assert debug.values == [17, 23]
assert not interface.nb_poke(99, 2) and debug.values == [17, 23]
interface = config_port.get_interface()
interface.nb_expand()
interface.nb_unbound()
assert interface.nb_reduce()
assert not interface.nb_bound(3)
assert config.calls == [("expand", 1), ("unbound", 16), ("reduce", 1), ("bound", 3)]
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
records = []

if value_type in (int, "long long", "unsigned long long"):
    bits = ctypes.sizeof(output_type) * 8
    lower, upper = (0, 2**bits - 1) if value_type == "unsigned long long" else (-(2 ** (bits - 1)), 2 ** (bits - 1) - 1)
    native = get_port.get_interface()
    for method in ("get", "peek", "nb_get", "nb_peek"):
        original = getattr(debug, method)
        for invalid in (True, 1.5, object(), lower - 1, upper + 1):
            setattr(
                debug,
                method,
                lambda invalid=invalid, method=method: (True, invalid) if method.startswith("nb_") else invalid,
            )
            output = output_type(71)
            try:
                getattr(native, method)(output)
            except TypeError, ValueError, OverflowError:
                pass
            else:
                raise AssertionError(f"invalid {method} callback accepted")
            assert output_value(output) == 71
        for endpoint in (lower, upper):
            setattr(
                debug,
                method,
                lambda endpoint=endpoint, method=method: (True, endpoint) if method.startswith("nb_") else endpoint,
            )
            output = output_type(71)
            result = getattr(native, method)(output)
            assert output_value(output) == endpoint
            if method.startswith("nb_"):
                assert result
        setattr(debug, method, original)


def exercise():
    """Validate compound blocking and nonblocking overloads on shared samples."""
    put = put_port.get_interface()
    get = get_port.get_interface()
    put.put(19)
    assert put.nb_can_put() and put.nb_put(29)
    assert debug.values == [29, 19, 17, 23]
    if value_type is int:
        assert cppyy.gbl.fifo_base_reference(get, get)
    assert not put.nb_put(99)
    assert get.peek() == 23
    value = output_type(71)
    get.peek(value)
    assert output_value(value) == 23
    assert get.nb_peek(value, 0) and output_value(value) == 29
    assert get.nb_peek(value) and output_value(value) == 23
    assert get.get() == 23
    get.get(value)
    assert output_value(value) == 17
    assert get.nb_get(value) and output_value(value) == 19
    assert get.nb_get(value) and output_value(value) == 29
    assert not get.nb_get(value) and output_value(value) == 29
    if value_type in (sc_dt.sc_bigint[128], sc_dt.sc_biguint[128], sc_dt.sc_biguint[137]):
        expected = value_type(17)
        expected[136 if value_type == sc_dt.sc_biguint[137] else 127] = True
        assert put.nb_put(expected)
        actual = get.peek()
        assert same_wide(actual, expected)
        get.peek(value)
        assert same_wide(output_value(value), expected)
        assert get.nb_peek(value, 0) and same_wide(output_value(value), expected)
        assert get.nb_poke(expected, 0)
        get.get(value)
        assert same_wide(output_value(value), expected)
        assert not get.nb_get(value) and same_wide(output_value(value), expected)
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(exercise, "exercise")
sc.sc_start(2, sc.SC_NS)
assert handle.terminated() and records == ["1 ns"], records
sc.sc_stop()
print("STRUCTURAL_TLM_FIFO_COMPOUND_OK")
