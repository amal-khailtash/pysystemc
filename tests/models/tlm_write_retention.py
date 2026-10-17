"""Verify retained analysis/put/debug TLM callback arguments own native storage."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy
import gc


class Owner:
    """Retain native values supplied to ordinary TLM callbacks."""

    def __init__(self):
        """Initialize retained argument and index records."""
        self.values = []
        self.indices = []

    def write(self, value):
        """Keep an analysis sample beyond native return."""
        self.values.append(value)

    def put(self, value):
        """Keep a blocking put sample."""
        self.values.append(value)

    def nb_put(self, value):
        """Keep a nonblocking put sample."""
        self.values.append(value)
        return True

    def nb_can_put(self):
        """Report readiness."""
        return True

    def used(self):
        """Report one debug entry."""
        return 1

    def size(self):
        """Report capacity."""
        return 1

    def debug(self):
        """Leave debug state unchanged."""

    def nb_peek(self, index):
        """Return a valid debug output pair."""
        return True, sc_dt.sc_biguint[137](17)

    def nb_poke(self, value, index):
        """Retain debug data and the native index."""
        self.values.append(value)
        self.indices.append(index)
        return True


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for family in (
    tlm.tlm_analysis_if,
    tlm.tlm_blocking_put_if,
    tlm.tlm_nonblocking_put_if,
    tlm.tlm_put_if,
    tlm.tlm_fifo_debug_if,
):
    owner = Owner()
    port = sc.sc_port[family[sc_dt.sc_biguint[137]]]()
    port(owner)
    entries.append((owner, port))
del name
for owner, port in entries:
    native = port.get_interface()
    for method in ("write", "put", "nb_put", "nb_poke"):
        if not hasattr(native, method):
            continue
        source = sc_dt.sc_biguint[137](17)
        source[136] = True
        arguments = (source, 0) if method == "nb_poke" else (source,)
        getattr(native, method)(*arguments)
        retained = owner.values[-1]
        assert cppyy.addressof(retained) != cppyy.addressof(source)
        expected = source.to_string()
        source.__assign__(29)
        gc.collect()
        assert retained.to_string() == expected
        retained.__assign__(73)
        assert source.to_uint() == 29
        if method == "nb_poke":
            assert owner.indices == [0]
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("TLM_WRITE_RETENTION_OK")
