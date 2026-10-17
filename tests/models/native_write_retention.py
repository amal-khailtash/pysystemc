"""Verify retained signal/FIFO write arguments own independent native storage."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy
import gc


class Owner:
    """Retain callback arguments without making user-side copies."""

    def __init__(self):
        """Initialize retained values and a notification event."""
        self.values = []
        self.changed = sc.sc_event()

    def write(self, value):
        """Keep the exact callback object beyond native return."""
        self.values.append(value)

    def nb_write(self, value):
        """Keep the nonblocking callback object and accept the write."""
        self.values.append(value)
        return True

    def num_free(self):
        """Report available capacity for combined output interface."""
        return 10

    def data_read_event(self):
        """Borrow the native output event."""
        return self.changed

    def get_writer_policy(self):
        """Report native one-writer policy."""
        return sc.SC_ONE_WRITER


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for family in (sc.sc_signal_write_if, sc.sc_fifo_out_if, sc.sc_fifo_blocking_out_if, sc.sc_fifo_nonblocking_out_if):
    owner = Owner()
    port = sc.sc_port[family[sc_dt.sc_biguint[137]]]()
    port(owner)
    entries.append((owner, port))
del name
for owner, port in entries:
    native = port.get_interface()
    for method in ("write", "nb_write"):
        if not hasattr(native, method):
            continue
        source = sc_dt.sc_biguint[137](17)
        source[136] = True
        getattr(native, method)(source)
        retained = owner.values[-1]
        assert cppyy.addressof(retained) != cppyy.addressof(source)
        expected = source.to_string()
        source.__assign__(29)
        gc.collect()
        assert retained.to_string() == expected
        retained.__assign__(73)
        assert source.to_uint() == 29
        del source
        gc.collect()
        assert retained.to_uint() == 73
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("NATIVE_WRITE_RETENTION_OK")
