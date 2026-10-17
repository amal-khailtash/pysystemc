"""Validate five FIFO configuration/debug callback result contracts."""

from amal.eda.systemc import sc_core as sc, tlm


class Owner:
    """Supply configurable callback results and preserve dispatch arguments."""

    def __init__(self):
        """Initialize results and call history."""
        self.result = None
        self.calls = []

    def __getattr__(self, name):
        """Expose all selected FIFO callbacks as ordinary methods."""

        def callback(*args):
            """Record native default/explicit arguments and return owner state."""
            self.calls.append((name, args))
            return self.result

        return callback


name = sc.sc_module_name("top")
top = sc.sc_module(name)
config = Owner()
debug = Owner()
config_port = sc.sc_port[tlm.tlm_fifo_config_size_if]()
debug_port = sc.sc_port[tlm.tlm_fifo_debug_if[int]]()
config_port(config)
debug_port(debug)
del name
native = config_port.get_interface()
checks = (
    (config, native.nb_expand, "nb_expand", (1,)),
    (config, native.nb_unbound, "nb_unbound", (16,)),
    (config, native.nb_reduce, "nb_reduce", (1,)),
    (config, lambda: native.nb_bound(3), "nb_bound", (3,)),
    (debug, lambda: debug_port.get_interface().nb_poke(17), "nb_poke", (17, 0)),
)
for owner, invoke, method, arguments in checks:
    void = method in ("nb_expand", "nb_unbound")
    invalid = (False, 0, "bad") if void else (None, 0, 1, "bad")
    for result in invalid:
        owner.result = result
        try:
            invoke()
        except TypeError as error:
            assert method in str(error)
        else:
            raise AssertionError(f"invalid {method} result accepted")
        assert owner.calls[-1] == (method, arguments)
    for result in (None,) if void else (False, True, False):
        owner.result = result
        assert invoke() == result
        assert owner.calls[-1] == (method, arguments)
for method in ("used", "size"):
    invoke = getattr(debug_port.get_interface(), method)
    for result in (None, True, 1.5, 2**31, -(2**31) - 1):
        debug.result = result
        try:
            invoke()
        except TypeError as error:
            assert method in str(error)
        else:
            raise AssertionError("invalid count accepted")
    for result in (-(2**31), -1, 0, 2**31 - 1):
        debug.result = result
        assert invoke() == result
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("TLM_FIFO_CALLBACK_CONTRACTS_OK")
