"""Reject unsupported readable trace values without sampling them."""

from amal.eda.systemc import sc_core as sc


class Readable:
    """Python object with a visible side effect in its read method."""

    def __init__(self):
        """Initialize a counter for read calls."""
        self.read_calls = 0

    def read(self):
        """Count an attempted sample and return a scalar value."""
        self.read_calls += 1
        return 23


readable = Readable()
filled_fifo = sc.sc_fifo[int]("filled_fifo", 1)
empty_fifo = sc.sc_fifo[int]("empty_fifo", 1)
filled_fifo.write(23)
sc.sc_start(sc.SC_ZERO_TIME)
assert filled_fifo.num_available() == 1

with sc.vcd_trace("trace") as trace:
    try:
        sc.sc_trace(trace, readable, "unsupported_readable")
    except TypeError:
        pass
    else:
        raise AssertionError("unsupported readable trace value was accepted")

    # Native SystemC warns and returns for untraceable sc_objects such as FIFO
    # channels. Tracing must not sample a queued item or wait on an empty FIFO.
    sc.sc_trace(trace, filled_fifo, "unsupported_filled_fifo")
    assert readable.read_calls == 0
    assert filled_fifo.num_available() == 1
    assert filled_fifo.read() == 23
    sc.sc_trace(trace, empty_fifo, "unsupported_empty_fifo")
    assert empty_fifo.num_available() == 0
    sc.sc_start(1, sc.SC_NS)
    print(f"TRACE_READ_SIDE_EFFECT {readable.read_calls} FILLED_FIFO 1 23 EMPTY_FIFO_EMPTY")
