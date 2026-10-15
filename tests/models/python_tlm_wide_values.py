"""Verify structural TLM adapters preserve native arbitrary-width value storage."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt, tlm

Value = sc_dt.sc_biguint[73]


def sample(low):
    """Create a native73bit sample with a high bit and supplied low value."""
    value = Value(low)
    value[72] = True
    return value


class Queue:
    """Store native copies behind structural blocking/nonblocking callbacks."""

    def __init__(self):
        """Initialize empty sample storage and callback history."""
        self.values = []
        self.calls = []

    def put(self, value):
        """Retain an independent copy after a blocking service wait."""
        sc.wait(1, sc.SC_NS)
        self.values.append(Value(value))
        self.calls.append("put")

    def nb_put(self, value):
        """Store a copied sample immediately."""
        self.values.append(Value(value))
        self.calls.append("nb_put")
        return True

    def nb_can_put(self):
        """Report unrestricted capacity."""
        return True

    def get(self):
        """Consume a native sample after a blocking service wait."""
        sc.wait(1, sc.SC_NS)
        self.calls.append("get")
        return self.values.pop(0)

    def peek(self):
        """Return a copy without consuming storage."""
        self.calls.append("peek")
        return Value(self.values[0])

    def nb_get(self):
        """Consume only available samples with explicit success."""
        self.calls.append("nb_get")
        return (True, self.values.pop(0)) if self.values else (False, Value())

    def nb_peek(self):
        """Return an available sample copy without consumption."""
        self.calls.append("nb_peek")
        return (True, Value(self.values[0])) if self.values else (False, Value())

    def nb_can_get(self):
        """Report sample availability."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report sample availability without changing state."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
queue = Queue()
put = sc.sc_port[tlm.tlm_put_if[Value]]("put")
get = sc.sc_port[tlm.tlm_get_peek_if[Value]]("get")
put(queue)
get(queue)
del module_name
records = []


def process():
    """Assert wide copies, high-bit preservation and conditional native assignment."""
    source = sample(17)
    expected = source.to_string()
    put.get_interface().put(source)
    source[72] = False
    assert queue.values[0].to_string() == expected
    assert get.get_interface().peek().to_string() == expected
    output = Value(99)
    get.get_interface().peek(output)
    assert output.to_string() == expected
    assert get.get_interface().nb_peek(output)
    assert output.to_string() == expected and len(queue.values) == 1
    get.get_interface().get(output)
    assert output.to_string() == expected and not queue.values
    assert not get.get_interface().nb_get(output)
    assert output.to_string() == expected
    second = sample(23)
    assert put.get_interface().nb_put(second)
    second[72] = False
    assert get.get_interface().nb_get(output)
    assert output.to_string() == sample(23).to_string()
    assert not queue.values
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(process, "process")
sc.sc_start(3, sc.SC_NS)
assert records == ["2 ns"]
assert queue.calls == ["put", "peek", "peek", "nb_peek", "get", "nb_get", "nb_put", "nb_get"]
assert handle.terminated()
sc.sc_stop()
print("PYTHON_TLM_WIDE_VALUES_OK")
