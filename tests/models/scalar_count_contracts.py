"""Validate FIFO count and bool/signed/unsigned scalar result contracts."""

from amal.eda.systemc import sc_core as sc
import cppyy


class Owner:
    """Return configured count/value while retaining native event storage."""

    def __init__(self):
        """Initialize scalar state and a live native event."""
        self.result = 0
        self.changed = sc.sc_event()

    def __getattr__(self, method):
        """Provide the selected family callbacks with valid auxiliary results."""
        if method.endswith("event"):
            return lambda: self.changed
        if method in ("event", "posedge", "negedge", "nb_write"):
            return lambda *args: False
        if method == "nb_read":
            return lambda: (False, 0)
        if method == "write":
            return lambda value: None
        return lambda: self.result


name = sc.sc_module_name("top")
top = sc.sc_module(name)
checks = []
for family, method in ((sc.sc_fifo_in_if[int], "num_available"), (sc.sc_fifo_out_if[int], "num_free")):
    owner = Owner()
    port = sc.sc_port[family]()
    port(owner)
    checks.append(
        (
            owner,
            getattr(port.get_interface(), method),
            (None, True, 1.5, "1", 2**31, -(2**31) - 1),
            (-(2**31), -1, 0, 2**31 - 1),
        )
    )
for scalar in (
    "bool",
    "signed char",
    "short",
    "int",
    "long",
    "long long",
    "unsigned char",
    "unsigned short",
    "unsigned int",
    "unsigned long",
    "unsigned long long",
):
    owner = Owner()
    port = sc.sc_port[sc.sc_signal_in_if[scalar]]()
    port(owner)
    native = port.get_interface()
    if scalar == "bool":
        bad, good = (None, 0, 1, "true"), (False, True)
    else:
        bits = cppyy.sizeof(scalar) * 8
        unsigned = scalar.startswith("unsigned")
        lower, upper = (0, 2**bits) if unsigned else (-(2 ** (bits - 1)), 2 ** (bits - 1))
        bad, good = (None, True, 1.5, "1", lower - 1, upper), (lower, 0, upper - 1)
    for method in (native.read, native.get_data_ref):
        checks.append((owner, method, bad, good))
del name
for owner, invoke, bad, good in checks:
    for value in bad:
        owner.result = value
        try:
            invoke()
        except TypeError:
            pass
        else:
            raise AssertionError(f"invalid scalar/count accepted: {value!r}")
    for value in good:
        owner.result = value
        actual = invoke()
        if isinstance(actual, str):
            # cppyy exposes signed/unsigned char native results as one character.
            expected = chr(value % 256)
        else:
            expected = value
        assert actual == expected, (str(invoke), value, actual)
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("SCALAR_COUNT_CONTRACTS_OK")
