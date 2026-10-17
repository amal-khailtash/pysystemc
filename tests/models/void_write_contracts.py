"""Verify five void write/put callback contracts through native interfaces."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm


class Owner:
    """Supply required callbacks and configurable write results."""

    def __init__(self):
        """Own native event/value storage and recorded independent samples."""
        self.result = None
        self.event_storage = sc.sc_event()
        self.value = sc_dt.sc_biguint[137](17)
        self.samples = []

    def write(self, value):
        """Record the copied write argument and return the selected result."""
        self.samples.append(value)
        return self.result

    put = write

    def __getattr__(self, method):
        """Provide other family callbacks without altering write state."""
        if method.endswith("event"):
            return lambda: self.event_storage
        if method in ("read", "get_data_ref", "get", "peek"):
            return lambda: self.value
        if method == "get_writer_policy":
            return lambda: sc.SC_ONE_WRITER
        if method in ("event", "nb_can_put"):
            return lambda: False
        if method == "nb_put":
            return lambda value: False
        return lambda: 1


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for family, method in (
    (sc.sc_signal_inout_if, "write"),
    (sc.sc_signal_write_if, "write"),
    (sc.sc_fifo_blocking_out_if, "write"),
    (sc.sc_fifo_out_if, "write"),
    (tlm.tlm_analysis_if, "write"),
    (tlm.tlm_blocking_put_if, "put"),
    (tlm.tlm_put_if, "put"),
):
    owner = Owner()
    port = sc.sc_port[family[sc_dt.sc_biguint[137]]]()
    port(owner)
    entries.append((owner, port, method))
del name
source = sc_dt.sc_biguint[137](73)
for owner, port, method in entries:
    invoke = getattr(port.get_interface(), method)
    for invalid in (True, False, 0, 1, "value", source):
        owner.result = invalid
        try:
            invoke(source)
        except TypeError as error:
            assert f"{method} must return None" in str(error)
        else:
            raise AssertionError(f"void result accepted: {invalid!r}")
        assert source.to_uint() == 73
        assert owner.samples[-1].to_uint() == 73
    owner.result = None
    assert invoke(source) is None
    owner.samples[-1].__assign__(29)
    assert source.to_uint() == 73
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("VOID_WRITE_CONTRACTS_OK")
