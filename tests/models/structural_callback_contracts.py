"""Validate five native callback result contracts and recovery after rejection."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


class Owner:
    """Supply configurable callback results for native interface contracts."""

    def __init__(self):
        """Create live event storage and initial valid results."""
        self.result = 0
        self.changed = sc.sc_event()

    def __getattr__(self, name):
        """Provide ordinary callable methods for the selected native family."""
        return lambda *args: self.result


name = sc.sc_module_name("top")
top = sc.sc_module(name)
entries = []
for interface in (
    sc.sc_mutex_if,
    sc.sc_semaphore_if,
    sc.sc_signal_in_if[int],
    sc.sc_signal_write_if[sc_dt.sc_biguint[137]],
    sc.sc_event_queue_if,
):
    owner = Owner()
    port = sc.sc_port[interface]()
    port(owner)
    entries.append((owner, port))
del name
mutex, semaphore, signal, writer, queue = [(owner, port.get_interface()) for owner, port in entries]


def reject(owner, invoke, values, error_type):
    """Require each malformed result to fail at the callback boundary."""
    for value in values:
        owner.result = value
        try:
            invoke()
        except error_type:
            pass
        else:
            raise AssertionError(f"invalid callback result accepted: {value!r}")


for owner, native, methods in (
    (mutex[0], mutex[1], ("lock", "trylock", "unlock")),
    (semaphore[0], semaphore[1], ("wait", "trywait", "post", "get_value")),
):
    for method in methods:
        invoke = getattr(native, method)
        reject(owner, invoke, (None, True, 1.5, 2**31, -(2**31) - 1), TypeError)
        for result in (-1, 0, 2**31 - 1):
            owner.result = result
            assert invoke() == result
reject(signal[0], signal[1].event, (None, 0, 1, "yes"), TypeError)
signal[0].result = True
assert signal[1].event() is True
reject(writer[0], writer[1].get_writer_policy, (None, True, -1, 2, "many"), ValueError)
for policy in (sc.SC_ONE_WRITER, sc.SC_MANY_WRITERS, sc.SC_UNCHECKED_WRITERS):
    writer[0].result = policy
    assert writer[1].get_writer_policy() == policy
for owner, invoke in ((signal[0], signal[1].value_changed_event), (queue[0], queue[1].default_event)):
    reject(owner, invoke, (None, 1, sc_dt.sc_biguint[137](17), cppyy.bind_object(0, sc.sc_event)), TypeError)
    owner.result = owner.changed
    assert invoke() == owner.changed
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
print("STRUCTURAL_CALLBACK_CONTRACTS_OK")
