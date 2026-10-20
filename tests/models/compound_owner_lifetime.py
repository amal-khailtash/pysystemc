"""Verify callback-owner retention and explicit release across compound interfaces."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy
import ctypes
import gc
import weakref


class FifoPutOwner:
    """Provide the full FIFO put/debug callbacks over retained integer samples."""

    def __init__(self):
        """Initialize capacity-limited FIFO storage and callback history."""
        self.values = []
        self.calls = []

    def used(self):
        """Return the number of samples held by the owner."""
        self.calls.append(("used", str(sc.sc_time_stamp())))
        return len(self.values)

    def size(self):
        """Return the native FIFO capacity."""
        return 4

    def debug(self):
        """Record a non-mutating debug inspection."""
        self.calls.append(("debug", str(sc.sc_time_stamp())))

    def nb_peek(self, index):
        """Return a debug sample by index without changing the FIFO."""
        return (True, self.values[index]) if 0 <= index < len(self.values) else (False, 0)

    def nb_poke(self, value, index=0):
        """Replace one occupied debug entry, rejecting unavailable indexes."""
        if not 0 <= index < len(self.values):
            return False
        self.values[index] = value
        return True

    def put(self, value):
        """Copy a blocking put sample into FIFO state."""
        self.calls.append(("put", value, str(sc.sc_time_stamp())))
        self.values.append(value)

    def nb_put(self, value):
        """Copy a nonblocking put sample when capacity remains."""
        self.calls.append(("nb_put", value, str(sc.sc_time_stamp())))
        if len(self.values) >= 4:
            return False
        self.values.append(value)
        return True

    def nb_can_put(self):
        """Report whether FIFO storage has room for another sample."""
        return len(self.values) < 4


class FifoGetOwner:
    """Provide the full FIFO get/debug callbacks over retained integer samples."""

    def __init__(self):
        """Initialize readable samples and callback history."""
        self.values = [23, 29, 31]
        self.calls = []

    def used(self):
        """Return the number of samples held by the owner."""
        self.calls.append(("used", str(sc.sc_time_stamp())))
        return len(self.values)

    def size(self):
        """Return the native FIFO capacity."""
        return 4

    def debug(self):
        """Record a non-mutating debug inspection."""
        self.calls.append(("debug", str(sc.sc_time_stamp())))

    def nb_poke(self, value, index=0):
        """Replace one occupied debug entry, rejecting unavailable indexes."""
        if not 0 <= index < len(self.values):
            return False
        self.values[index] = value
        return True

    def get(self):
        """Consume one sample through blocking get."""
        self.calls.append(("get", str(sc.sc_time_stamp())))
        return self.values.pop(0)

    def peek(self):
        """Read the next sample without consuming it."""
        self.calls.append(("peek", str(sc.sc_time_stamp())))
        return self.values[0]

    def nb_get(self):
        """Consume one sample only when available."""
        self.calls.append(("nb_get", str(sc.sc_time_stamp())))
        return (True, self.values.pop(0)) if self.values else (False, 0)

    def nb_peek(self, index=None):
        """Return the next TLM sample or an indexed debug sample."""
        self.calls.append(("nb_peek", str(sc.sc_time_stamp())))
        if index is None:
            return (True, self.values[0]) if self.values else (False, 0)
        return (True, self.values[index]) if 0 <= index < len(self.values) else (False, 0)

    def nb_can_get(self):
        """Report whether blocking or nonblocking get can read a sample."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report whether blocking or nonblocking peek can read a sample."""
        return bool(self.values)


class CompoundOwner:
    """Provide a full int master or slave interface and ordered response state."""

    def __init__(self, family, responses):
        """Retain a family label, requests, response samples and callback history."""
        self.family = family
        self.requests = []
        self.responses = list(responses)
        self.calls = []

    def put(self, value):
        """Retain one request or response value supplied to the compound port."""
        self.calls.append(("put", value, str(sc.sc_time_stamp())))
        self.requests.append(value)

    def get(self):
        """Consume one response or request value through blocking get."""
        self.calls.append(("get", str(sc.sc_time_stamp())))
        return self.responses.pop(0)

    def peek(self):
        """Return the next response or request value without consuming it."""
        self.calls.append(("peek", str(sc.sc_time_stamp())))
        return self.responses[0]

    def nb_put(self, value):
        """Retain one nonblocking request or response value."""
        self.calls.append(("nb_put", value, str(sc.sc_time_stamp())))
        self.requests.append(value)
        return True

    def nb_get(self):
        """Consume a response or request value when one is available."""
        self.calls.append(("nb_get", str(sc.sc_time_stamp())))
        return (True, self.responses.pop(0)) if self.responses else (False, 0)

    def nb_peek(self):
        """Peek at a response or request value when one is available."""
        self.calls.append(("nb_peek", str(sc.sc_time_stamp())))
        return (True, self.responses[0]) if self.responses else (False, 0)

    def nb_can_put(self):
        """Report that the owner accepts compound put operations."""
        return True

    def nb_can_get(self):
        """Report whether the owner has a response or request sample."""
        return bool(self.responses)

    def nb_can_peek(self):
        """Report whether the owner has a peekable response or request."""
        return bool(self.responses)


class Top(sc.sc_module):
    """Check retained owners through simulation and end-of-simulation hooks."""

    def end_of_simulation(self):
        """Assert all owners remain callable after external references are dropped."""
        assert all(reference() is not None for reference in owner_refs)
        assert fifo_put_port.get_interface().used() == 2
        assert fifo_get_port.get_interface().peek() == 31
        assert master_port.get_interface().peek() == 59
        assert slave_port.get_interface().peek() == 73
        lifecycle.append(("end", str(sc.sc_time_stamp())))


def create_group(full_interface, inherited_interfaces, owner, names):
    """Bind one retained Python owner through its full and inherited interfaces.

    Args:
        full_interface: Native full interface template specialization.
        inherited_interfaces: Named native base interface specializations.
        owner: Python callbacks retained by the structural adapter.
        names: Prefix used to assign stable port names.

    Returns:
        The owner weak reference and a mapping of named bound ports.
    """
    reference = weakref.ref(owner)
    ports = {}
    full_port = sc.sc_port[full_interface](f"{names}_full")
    full_port(owner)
    ports["full"] = full_port
    for name, interface in inherited_interfaces:
        port = sc.sc_port[interface](f"{names}_{name}")
        port(owner)
        ports[name] = port
    return reference, ports


def state_snapshot(references):
    """Capture owner state without retaining owners beyond this function call."""
    return tuple(
        (
            tuple(reference().calls),
            tuple(reference().values if hasattr(reference(), "values") else reference().responses),
        )
        for reference in references
    )


def event_snapshot(groups, event_methods):
    """Capture addresses of native readiness events borrowed by each port."""
    snapshot = []
    for family, ports in groups.items():
        for port_name, port in ports.items():
            interface = port.get_interface()
            for method in event_methods[family]:
                if hasattr(interface, method):
                    snapshot.append((family, port_name, method, cppyy.addressof(getattr(interface, method)())))
    return tuple(snapshot)


def check_live_interfaces():
    """Exercise callable methods for each family and record their native time."""
    fifo_put = fifo_put_port.get_interface()
    fifo_put.put(11)
    assert fifo_put.nb_put(13)
    assert fifo_put.used() == 2 and fifo_put.nb_can_put()
    fifo_put.debug()

    fifo_get = fifo_get_port.get_interface()
    assert fifo_get.peek() == 23
    assert fifo_get.get() == 23
    output = ctypes.c_int(0)
    assert fifo_get.nb_peek(output) and output.value == 29
    assert fifo_get.nb_get(output) and output.value == 29
    assert fifo_get.nb_can_get() and fifo_get.nb_can_peek()

    master = master_port.get_interface()
    master.put(17)
    assert master.nb_put(19)
    assert master.peek() == 47
    assert master.get() == 47
    output = ctypes.c_int(0)
    assert master.nb_peek(output) and output.value == 53
    assert master.nb_get(output) and output.value == 53
    assert master.nb_can_put() and master.nb_can_get() and master.nb_can_peek()

    slave = slave_port.get_interface()
    slave.put(79)
    assert slave.nb_put(83)
    assert slave.peek() == 67
    assert slave.get() == 67
    output = ctypes.c_int(0)
    assert slave.nb_peek(output) and output.value == 71
    assert slave.nb_get(output) and output.value == 71
    assert slave.nb_can_put() and slave.nb_can_get() and slave.nb_can_peek()
    lifecycle.append(("callbacks", str(sc.sc_time_stamp())))


def run_simulation():
    """Create four unique adapters and verify their retained callback lifetime."""
    global fifo_put_port, fifo_get_port, master_port, slave_port, owner_refs, groups
    module_name = sc.sc_module_name("top")
    top = Top(module_name)
    int_type = int
    fifo_put_owner = FifoPutOwner()
    fifo_get_owner = FifoGetOwner()
    master_owner = CompoundOwner("master", (47, 53, 59))
    slave_owner = CompoundOwner("slave", (67, 71, 73))
    owner_refs = []
    groups = {}

    owner_ref, groups["fifo_put"] = create_group(
        tlm.tlm_fifo_put_if[int],
        (("blocking", tlm.tlm_blocking_put_if[int]), ("nonblocking", tlm.tlm_nonblocking_put_if[int])),
        fifo_put_owner,
        "fifo_put",
    )
    owner_refs.append(owner_ref)
    owner_ref, groups["fifo_get"] = create_group(
        tlm.tlm_fifo_get_if[int],
        (
            ("combined", tlm.tlm_get_peek_if[int]),
            ("blocking_get", tlm.tlm_blocking_get_if[int]),
            ("nonblocking_get", tlm.tlm_nonblocking_get_if[int]),
            ("blocking_peek", tlm.tlm_blocking_peek_if[int]),
            ("nonblocking_peek", tlm.tlm_nonblocking_peek_if[int]),
        ),
        fifo_get_owner,
        "fifo_get",
    )
    owner_refs.append(owner_ref)
    owner_ref, groups["master"] = create_group(
        tlm.tlm_master_if[int, int],
        (
            ("blocking", tlm.tlm_blocking_master_if[int, int]),
            ("nonblocking", tlm.tlm_nonblocking_master_if[int, int]),
        ),
        master_owner,
        "master",
    )
    owner_refs.append(owner_ref)
    owner_ref, groups["slave"] = create_group(
        tlm.tlm_slave_if[int, int],
        (
            ("blocking", tlm.tlm_blocking_slave_if[int, int]),
            ("nonblocking", tlm.tlm_nonblocking_slave_if[int, int]),
        ),
        slave_owner,
        "slave",
    )
    owner_refs.append(owner_ref)
    fifo_put_port = groups["fifo_put"]["full"]
    fifo_get_port = groups["fifo_get"]["full"]
    master_port = groups["master"]["full"]
    slave_port = groups["slave"]["full"]
    del fifo_put_owner, fifo_get_owner, master_owner, slave_owner, module_name, int_type
    gc.collect()
    assert all(reference() is not None for reference in owner_refs)

    event_methods = {
        "fifo_put": ("ok_to_put",),
        "fifo_get": ("ok_to_get", "ok_to_peek"),
        "master": ("ok_to_put", "ok_to_get", "ok_to_peek"),
        "slave": ("ok_to_put", "ok_to_get", "ok_to_peek"),
    }
    events_before = event_snapshot(groups, event_methods)
    for family, ports in groups.items():
        expected_methods = event_methods[family]
        for method in expected_methods:
            addresses = [
                cppyy.addressof(getattr(port.get_interface(), method)())
                for port in ports.values()
                if hasattr(port.get_interface(), method)
            ]
            assert len(set(addresses)) == 1, (family, method, addresses)

    def process():
        """Reject callback release while running, then exercise all live owners."""
        before = state_snapshot(owner_refs)
        try:
            sc.release_interface_callbacks()
        except RuntimeError as error:
            assert "after simulation stop" in str(error)
        else:
            raise AssertionError("callback release succeeded during simulation")
        assert state_snapshot(owner_refs) == before
        assert all(reference() is not None for reference in owner_refs)
        lifecycle.append(("running_release_rejected", str(sc.sc_time_stamp())))
        check_live_interfaces()

    top.sc_thread("owner_lifetime", process)
    del process
    sc.sc_start(1, sc.SC_NS)
    sc.sc_stop()
    assert lifecycle == [
        ("running_release_rejected", "0 s"),
        ("callbacks", "0 s"),
        ("end", "1 ns"),
    ], lifecycle
    assert all(reference() is not None for reference in owner_refs)

    released = sc.release_interface_callbacks()
    assert released == 4, released
    assert sc.release_interface_callbacks() == 0
    gc.collect()
    assert all(reference() is None for reference in owner_refs)
    assert event_snapshot(groups, event_methods) == events_before

    cppyy.cppdef(
        """
        #include <Python.h>
        /** Clear a callback exception while preserving the caller's GIL state. */
        void compound_lifetime_clear_python_error() {
            PyGILState_STATE state = PyGILState_Ensure();
            PyErr_Clear();
            PyGILState_Release(state);
        }
        /** Check caller output preservation when a released get callback throws.
         * The channel remains owned by its native port; GIL state is restored after
         * clearing the Python callback exception and no adapter is deleted.
         */
        template<class Interface> bool compound_lifetime_released_get_output(Interface& channel) {
            int output = 79;
            try { channel.get(output); }
            catch (...) { compound_lifetime_clear_python_error(); return output == 79; }
            return false;
        }
        /** Check caller output preservation when a released peek callback throws. */
        template<class Interface> bool compound_lifetime_released_peek_output(Interface& channel) {
            int output = 83;
            try { channel.peek(output); }
            catch (...) { compound_lifetime_clear_python_error(); return output == 83; }
            return false;
        }
        /** Check caller output preservation when a released nonblocking get throws. */
        template<class Interface> bool compound_lifetime_released_nb_get_output(Interface& channel) {
            int output = 89;
            try { channel.nb_get(output); }
            catch (...) { compound_lifetime_clear_python_error(); return output == 89; }
            return false;
        }
        /** Check caller output preservation when a released nonblocking peek throws. */
        template<class Interface> bool compound_lifetime_released_nb_peek_output(Interface& channel) {
            int output = 97;
            try { channel.nb_peek(output); }
            catch (...) { compound_lifetime_clear_python_error(); return output == 97; }
            return false;
        }
        """
    )

    for port in (fifo_get_port, master_port, slave_port):
        interface = port.get_interface()
        assert cppyy.gbl.compound_lifetime_released_get_output(interface)
        assert cppyy.gbl.compound_lifetime_released_peek_output(interface)
        assert cppyy.gbl.compound_lifetime_released_nb_get_output(interface)
        assert cppyy.gbl.compound_lifetime_released_nb_peek_output(interface)
    for port, method, args in (
        (fifo_put_port, "put", (101,)),
        (fifo_get_port, "get", ()),
        (master_port, "put", (103,)),
        (master_port, "get", ()),
        (slave_port, "put", (107,)),
        (slave_port, "get", ()),
    ):
        try:
            getattr(port.get_interface(), method)(*args)
        except RuntimeError as error:
            assert "released" in str(error)
        else:
            raise AssertionError(f"{method} callback executed after owner release")
    print("COMPOUND_OWNER_LIFETIME", released, len(events_before), lifecycle[-1][1])
    print("COMPOUND_OWNER_LIFETIME_OK")


lifecycle = []
run_simulation()
