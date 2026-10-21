"""Verify multiword TLM1 transport response copies and native output overloads."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import ctypes


class Owner:
    """Return configurable response samples for a copied integer request."""

    def __init__(self, value):
        """Retain response storage and callback count."""
        self.value = value
        self.calls = 0

    def transport(self, request):
        """Return a native response or propagate a configured callback failure."""
        assert request == 17
        self.calls += 1
        if self.value is None:
            raise RuntimeError("configured failure")
        return self.value


name = sc.sc_module_name("top")
top = sc.sc_module(name)
groups = []
for datatype, sample, output in (
    ("long long", -(2**63), ctypes.c_longlong(71)),
    ("unsigned long long", 2**64 - 1, ctypes.c_ulonglong(71)),
    ("long double", -2.5, ctypes.c_longdouble(71)),
    (sc_dt.sc_biguint[137], sc_dt.sc_biguint[137](17), sc_dt.sc_biguint[137](71)),
):
    if hasattr(sample, "to_uint64"):
        sample[136] = True
    owner = Owner(sample)
    port = sc.sc_port[tlm.tlm_transport_if["int", datatype]]()
    port(owner)
    groups.append((owner, port, sample, output))
del name
for owner, port, sample, output in groups:
    native = port.get_interface()
    result = native.transport(17)
    native.transport(17, output)
    if hasattr(sample, "to_uint64"):
        assert bool(result[136]) and bool(output[136])
        assert result.to_uint64() == output.to_uint64() == 17
    else:
        assert result == output.value == sample
    print("TRANSPORT_VALUE", sample.to_string() if hasattr(sample, "to_string") else sample)
    owner.value = None
    before = owner.calls
    try:
        native.transport(17, output)
    except Exception:
        pass
    else:
        raise AssertionError("missing callback error")
    assert owner.calls > before
    if hasattr(sample, "to_uint64"):
        assert bool(output[136]) and output.to_uint64() == 17
    else:
        assert output.value == sample
    owner.value = sample
    before = owner.calls
    native.transport(17, output)
    assert owner.calls == before + 1
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
print("TRANSPORT_MULTIWORD_OK 4 1 ns")
