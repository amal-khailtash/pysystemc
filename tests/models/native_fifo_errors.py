"""Verify FIFO callback validation, failed outputs, exceptions and recovery."""

from amal.eda.systemc import sc_core as sc, sc_dt


class Owner:
    """Provide configurable callback results for each FIFO interface."""

    def __init__(self):
        """Initialize native events and valid callbacks."""
        self.changed = sc.sc_event()
        self.result = (False, sc_dt.sc_biguint[137](0))
        self.write_result = False
        self.fail = False
        self.writes = []

    def nb_read(self):
        """Return the selected output-pair shape."""
        return self.result

    def nb_write(self, value):
        """Return the selected success value."""
        return self.write_result

    def read(self):
        """Raise or return a valid wide payload."""
        if self.fail:
            raise ValueError("read callback failed")
        return sc_dt.sc_biguint[137](29)

    def write(self, value):
        """Raise before recording the copied payload."""
        if self.fail:
            raise ValueError("write callback failed")
        self.writes.append(value.to_uint())

    def data_written_event(self):
        """Borrow the native notification event."""
        return self.changed

    def data_read_event(self):
        """Borrow the native notification event."""
        return self.changed


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Owner()
ports = []
for family in (
    sc.sc_fifo_nonblocking_in_if,
    sc.sc_fifo_nonblocking_out_if,
    sc.sc_fifo_blocking_in_if,
    sc.sc_fifo_blocking_out_if,
):
    port = sc.sc_port[family[sc_dt.sc_biguint[137]]]()
    port(owner)
    ports.append(port)
del name
nb_in, nb_out, blocking_in, blocking_out = [port.get_interface() for port in ports]
value = sc_dt.sc_biguint[137](73)
for invalid in (None, True, (True,), (1, value), [True, value], (True, object())):
    owner.result = invalid
    try:
        nb_in.nb_read(value)
    except TypeError, ValueError:
        pass
    else:
        raise AssertionError(f"invalid read result accepted: {invalid!r}")
    assert value.to_uint() == 73
for invalid in (None, 0, 1, "yes"):
    owner.write_result = invalid
    try:
        nb_out.nb_write(value)
    except TypeError as error:
        assert "nb_write must return bool" in str(error)
    else:
        raise AssertionError("invalid write result accepted")
owner.result = (False, sc_dt.sc_biguint[137](0))
assert not nb_in.nb_read(value) and value.to_uint() == 73
owner.result = (True, sc_dt.sc_biguint[137](17))
assert nb_in.nb_read(value) and value.to_uint() == 17
owner.write_result = True
assert nb_out.nb_write(value)
owner.fail = True
for invoke in (lambda: blocking_in.read(value), lambda: blocking_out.write(value)):
    try:
        invoke()
    except ValueError as error:
        assert "callback failed" in str(error)
    else:
        raise AssertionError("callback exception lost")
assert value.to_uint() == 17 and owner.writes == []
owner.fail = False
blocking_in.read(value)
blocking_out.write(value)
assert value.to_uint() == 29 and owner.writes == [29]
sc.sc_start(sc.SC_ZERO_TIME)
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_stop()
print("NATIVE_FIFO_ERRORS_OK")
