"""Exercise TLM put, compound request/response and FIFO interface families."""

from collections import deque
from pathlib import Path

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_put_compound_probe.h"))


WIDE = sc_dt.sc_biguint[137]


def wide(value):
    """Create a 137-bit value with its high bit set and a known low word."""
    result = WIDE(value)
    result[136] = True
    return result


def wide_pair(value):
    """Return the low word and high bit of one native 137-bit value."""
    return int(value.to_uint64()), int(bool(value[136]))


def value_number(value):
    """Convert a callback value to its observable low word."""
    return int(value.to_uint64()) if hasattr(value, "to_uint64") else int(value)


class PutOwner:
    """Retain requests and exact callback counts for put interface views."""

    def __init__(self):
        """Create empty request storage and zeroed callback counts."""
        self.values = []
        self.put_calls = self.nb_put_calls = self.can_put_calls = 0

    def put(self, value):
        """Retain one copied blocking request."""
        self.values.append(value_number(value))
        self.put_calls += 1

    def nb_put(self, value):
        """Retain one copied nonblocking request."""
        self.values.append(value_number(value))
        self.nb_put_calls += 1
        return True

    def nb_can_put(self):
        """Report that the test endpoint accepts requests."""
        self.can_put_calls += 1
        return True


class CompoundOwner:
    """Provide request/response callbacks for both native directions."""

    def __init__(self):
        """Initialize response samples, request history and callback counts."""
        self.sent = []
        self.responses = deque((wide(7), wide(9)))
        self.counts = dict.fromkeys(
            ("put", "get", "peek", "nb_put", "can_put", "nb_get", "can_get", "nb_peek", "can_peek"), 0
        )

    def set_responses(self, values):
        """Replace pending responses with copied wide values."""
        self.responses = deque(values)

    def put(self, value):
        """Retain one blocking request."""
        self.sent.append(value_number(value))
        self.counts["put"] += 1

    def get(self):
        """Consume one response for a blocking get callback."""
        self.counts["get"] += 1
        return self.responses.popleft()

    def peek(self):
        """Return the oldest pending response without consuming it."""
        self.counts["peek"] += 1
        return self.responses[0]

    def nb_put(self, value):
        """Retain one nonblocking request and report success."""
        self.sent.append(value_number(value))
        self.counts["nb_put"] += 1
        return True

    def nb_can_put(self):
        """Report request readiness."""
        self.counts["can_put"] += 1
        return True

    def nb_get(self):
        """Return one response tuple or failure without changing an empty queue."""
        self.counts["nb_get"] += 1
        return (True, self.responses.popleft()) if self.responses else (False, None)

    def nb_can_get(self):
        """Report whether a response is queued."""
        self.counts["can_get"] += 1
        return bool(self.responses)

    def nb_peek(self):
        """Return a nonconsuming response tuple or failure."""
        self.counts["nb_peek"] += 1
        return (True, self.responses[0]) if self.responses else (False, None)

    def nb_can_peek(self):
        """Report whether a response can be inspected."""
        self.counts["can_peek"] += 1
        return bool(self.responses)


class FifoOwner:
    """Implement Python-owned FIFO state behind native FIFO interface adapters."""

    def __init__(self):
        """Create newest-first samples and zeroed callback counters."""
        self.values = [wide(17), wide(23)]
        self.counts = dict.fromkeys(
            (
                "put",
                "nb_put",
                "can_put",
                "get",
                "peek",
                "nb_get",
                "can_get",
                "nb_peek",
                "can_peek",
                "indexed_peek",
                "poke",
                "debug",
            ),
            0,
        )

    def put(self, value):
        """Insert a copied value at the newest end."""
        self.values.insert(0, WIDE(value))
        self.counts["put"] += 1

    def nb_put(self, value):
        """Insert a copied value without waiting."""
        self.values.insert(0, WIDE(value))
        self.counts["nb_put"] += 1
        return True

    def nb_can_put(self):
        """Report available test FIFO capacity."""
        self.counts["can_put"] += 1
        return True

    def get(self):
        """Consume the oldest FIFO sample."""
        self.counts["get"] += 1
        return self.values.pop()

    def peek(self):
        """Return the oldest FIFO sample without consuming it."""
        self.counts["peek"] += 1
        return self.values[-1]

    def nb_get(self):
        """Return a success/value pair or failure without altering a sentinel."""
        self.counts["nb_get"] += 1
        return (True, self.values.pop()) if self.values else (False, None)

    def nb_can_get(self):
        """Report whether the FIFO has a value to consume."""
        self.counts["can_get"] += 1
        return bool(self.values)

    def nb_peek(self, index=None):
        """Return an oldest-value or newest-first indexed peek result."""
        if index is None:
            self.counts["nb_peek"] += 1
            return (True, self.values[-1]) if self.values else (False, None)
        self.counts["indexed_peek"] += 1
        return (True, self.values[index]) if 0 <= index < len(self.values) else (False, None)

    def nb_can_peek(self):
        """Report whether the FIFO has a value to inspect."""
        self.counts["can_peek"] += 1
        return bool(self.values)

    def nb_poke(self, value, index=0):
        """Replace a newest-first indexed value when the index exists."""
        self.counts["poke"] += 1
        if not 0 <= index < len(self.values):
            return False
        self.values[index] = WIDE(value)
        return True

    def used(self):
        """Return current FIFO occupancy."""
        return len(self.values)

    def size(self):
        """Return fixed test FIFO capacity."""
        return 4

    def debug(self):
        """Record one debug inspection."""
        self.counts["debug"] += 1


class TransportOwner:
    """Implement the custom request/response transport callback."""

    def __init__(self):
        """Initialize an exact dispatch counter."""
        self.calls = 0

    def transport(self, request):
        """Return a fresh custom response nine greater than the request."""
        self.calls += 1
        response = cppyy.gbl.pysystemc_tlm_put_compound.Record()
        response.code = request.code + 9
        return response


def check_put(module):
    """Exercise blocking, nonblocking and combined put interface overloads."""
    owner = PutOwner()
    combined = sc.sc_port[tlm.tlm_put_if[WIDE]]("put_combined")
    blocking = sc.sc_port[tlm.tlm_blocking_put_if[WIDE]]("put_blocking")
    nonblocking = sc.sc_port[tlm.tlm_nonblocking_put_if[WIDE]]("put_nonblocking")
    combined.bind(owner)
    blocking.bind(owner)
    nonblocking.bind(owner)
    combined_if = combined.get_interface()
    blocking_if = blocking.get_interface()
    nonblocking_if = nonblocking.get_interface()
    assert isinstance(combined_if, tlm.tlm_put_if[WIDE])
    assert isinstance(blocking_if, tlm.tlm_blocking_put_if[WIDE])
    assert isinstance(nonblocking_if, tlm.tlm_nonblocking_put_if[WIDE])
    assert len({cppyy.addressof(item) for item in (combined_if, blocking_if, nonblocking_if)}) == 1
    tag = tlm.tlm_tag[WIDE]()
    blocking_if.put(wide(17))
    assert nonblocking_if.nb_put(wide(19))
    assert nonblocking_if.nb_can_put() and nonblocking_if.nb_can_put(tag)
    put_event = cppyy.addressof(nonblocking_if.ok_to_put())
    assert put_event == cppyy.addressof(nonblocking_if.ok_to_put(tag))
    combined_if.put(wide(23))
    assert combined_if.nb_put(wide(29))
    assert combined_if.nb_can_put() and combined_if.nb_can_put(tag)
    counts = (len(owner.values), owner.put_calls, owner.nb_put_calls, owner.can_put_calls)
    assert counts == (4, 2, 2, 4), counts
    print("TLM_PUT", *counts, 2, int(put_event != 0))


def check_compound(module, direction):
    """Exercise all inherited method and output overloads for a master/slave."""
    interface_type = getattr(tlm, f"tlm_{direction}_if")[WIDE, WIDE]
    blocking_type = getattr(tlm, f"tlm_blocking_{direction}_if")[WIDE, WIDE]
    nonblocking_type = getattr(tlm, f"tlm_nonblocking_{direction}_if")[WIDE, WIDE]
    owner = CompoundOwner()
    combined = sc.sc_port[interface_type](f"{direction}_combined")
    blocking = sc.sc_port[blocking_type](f"{direction}_blocking")
    nonblocking = sc.sc_port[nonblocking_type](f"{direction}_nonblocking")
    combined.bind(owner)
    blocking.bind(owner)
    nonblocking.bind(owner)
    combined_if = combined.get_interface()
    blocking_if = blocking.get_interface()
    nonblocking_if = nonblocking.get_interface()
    assert isinstance(combined_if, interface_type)
    assert isinstance(combined_if, blocking_type)
    assert isinstance(combined_if, nonblocking_type)
    assert isinstance(blocking_if, blocking_type) and isinstance(nonblocking_if, nonblocking_type)
    assert len({cppyy.addressof(item) for item in (combined_if, blocking_if, nonblocking_if)}) == 1
    tag = tlm.tlm_tag[WIDE]()

    owner.set_responses((wide(7), wide(9), wide(11)))
    blocking_if.put(wide(17))
    assert wide_pair(blocking_if.get()) == (7, 1)
    assert wide_pair(blocking_if.get(tag)) == (9, 1)
    output = wide(99)
    blocking_if.get(output)
    assert wide_pair(output) == (11, 1)
    owner.set_responses((wide(13),))
    assert wide_pair(blocking_if.peek()) == (13, 1)
    assert wide_pair(blocking_if.peek(tag)) == (13, 1)
    output = wide(99)
    blocking_if.peek(output)
    assert wide_pair(output) == (13, 1)

    assert nonblocking_if.nb_put(wide(19))
    assert nonblocking_if.nb_can_put() and nonblocking_if.nb_can_put(tag)
    put_event = cppyy.addressof(nonblocking_if.ok_to_put())
    assert put_event == cppyy.addressof(nonblocking_if.ok_to_put(tag))
    owner.set_responses((wide(31), wide(37)))
    assert nonblocking_if.nb_can_get() and nonblocking_if.nb_can_get(tag)
    get_event = cppyy.addressof(nonblocking_if.ok_to_get())
    assert get_event == cppyy.addressof(nonblocking_if.ok_to_get(tag))
    output = WIDE(0)
    assert nonblocking_if.nb_get(output) and wide_pair(output) == (31, 1)
    assert nonblocking_if.nb_get(output) and wide_pair(output) == (37, 1)
    output = wide(91)
    assert not nonblocking_if.nb_get(output) and wide_pair(output) == (91, 1)
    owner.set_responses((wide(41),))
    assert nonblocking_if.nb_can_peek() and nonblocking_if.nb_can_peek(tag)
    peek_event = cppyy.addressof(nonblocking_if.ok_to_peek())
    assert peek_event == cppyy.addressof(nonblocking_if.ok_to_peek(tag))
    output = WIDE(0)
    assert nonblocking_if.nb_peek(output) and wide_pair(output) == (41, 1)
    owner.set_responses(())
    output = wide(93)
    assert not nonblocking_if.nb_peek(output) and wide_pair(output) == (93, 1)

    combined_if.put(wide(23))
    assert combined_if.nb_put(wide(29))
    owner.set_responses((wide(43), wide(47)))
    assert wide_pair(combined_if.get()) == (43, 1)
    output = WIDE(0)
    assert combined_if.nb_get(output) and wide_pair(output) == (47, 1)
    assert put_event != get_event and get_event != peek_event
    values = (
        len(owner.sent),
        *(
            owner.counts[name]
            for name in ("put", "get", "peek", "nb_put", "can_put", "nb_get", "can_get", "nb_peek", "can_peek")
        ),
        *wide_pair(output),
    )
    expected = (4, 2, 4, 3, 2, 2, 4, 2, 2, 2, 47, 1)
    assert values == expected, (direction, values)
    print(f"TLM_{direction.upper()}", *values)


def check_fifo(module):
    """Exercise FIFO put/get contracts and both overloaded peek families."""
    owner = FifoOwner()
    get = sc.sc_port[tlm.tlm_fifo_get_if[WIDE]]("fifo_get")
    put = sc.sc_port[tlm.tlm_fifo_put_if[WIDE]]("fifo_put")
    debug = sc.sc_port[tlm.tlm_fifo_debug_if[WIDE]]("fifo_debug")
    get.bind(owner)
    put.bind(owner)
    debug.bind(owner)
    get_if = get.get_interface()
    put_if = put.get_interface()
    debug_if = debug.get_interface()
    assert isinstance(get_if, tlm.tlm_fifo_get_if[WIDE])
    assert isinstance(get_if, tlm.tlm_fifo_debug_if[WIDE])
    assert isinstance(put_if, tlm.tlm_fifo_put_if[WIDE])
    assert isinstance(put_if, tlm.tlm_fifo_debug_if[WIDE])
    assert len({cppyy.addressof(item) for item in (get_if, put_if, debug_if)}) == 1
    tag = tlm.tlm_tag[WIDE]()
    assert debug_if.used() == 2 and debug_if.size() == 4
    debug_if.debug()
    newest, oldest = WIDE(0), WIDE(0)
    assert debug_if.nb_peek(newest, 0) and debug_if.nb_peek(oldest, 1)
    assert wide_pair(newest) == (17, 1) and wide_pair(oldest) == (23, 1)
    sentinel = wide(95)
    assert not debug_if.nb_peek(sentinel, 8) and wide_pair(sentinel) == (95, 1)
    assert debug_if.nb_poke(wide(31)) and debug_if.nb_poke(wide(37), 1)
    assert not debug_if.nb_poke(wide(99), 8)
    assert put_if.nb_can_put() and put_if.nb_can_put(tag)
    put_event = cppyy.addressof(put_if.ok_to_put())
    assert put_event == cppyy.addressof(put_if.ok_to_put(tag))
    put_if.put(wide(41))
    assert put_if.nb_put(wide(43))
    assert wide_pair(get_if.peek()) == (37, 1)
    assert wide_pair(get_if.peek(tag)) == (37, 1)
    output = WIDE(0)
    get_if.peek(output)
    assert wide_pair(output) == (37, 1)
    assert get_if.nb_can_get() and get_if.nb_can_get(tag)
    assert get_if.nb_can_peek() and get_if.nb_can_peek(tag)
    get_event = cppyy.addressof(get_if.ok_to_get())
    peek_event = cppyy.addressof(get_if.ok_to_peek())
    assert get_event == cppyy.addressof(get_if.ok_to_get(tag))
    assert peek_event == cppyy.addressof(get_if.ok_to_peek(tag))
    assert put_event != get_event and get_event != peek_event
    assert get_if.nb_peek(output) and wide_pair(output) == (37, 1)
    assert wide_pair(get_if.get()) == (37, 1)
    assert wide_pair(get_if.get(tag)) == (31, 1)
    get_if.get(output)
    assert wide_pair(output) == (41, 1)
    assert get_if.nb_get(output) and wide_pair(output) == (43, 1)
    output = wide(91)
    assert not get_if.nb_get(output) and wide_pair(output) == (91, 1)
    peek_sentinel = wide(93)
    assert not get_if.nb_peek(peek_sentinel) and wide_pair(peek_sentinel) == (93, 1)
    assert not get_if.nb_can_get() and not get_if.nb_can_get(tag)
    assert not get_if.nb_can_peek() and not get_if.nb_can_peek(tag)
    assert debug_if.used() == 0
    counts = (
        debug_if.used(),
        *wide_pair(newest),
        *wide_pair(oldest),
        *wide_pair(output),
        *(
            owner.counts[name]
            for name in (
                "put",
                "nb_put",
                "can_put",
                "get",
                "peek",
                "nb_peek",
                "nb_get",
                "can_get",
                "can_peek",
                "indexed_peek",
                "poke",
                "debug",
            )
        ),
    )
    expected = (0, 17, 1, 23, 1, 91, 1, 1, 1, 2, 3, 3, 2, 2, 4, 4, 3, 3, 1)
    assert counts == expected, counts
    print("TLM_FIFO", *counts)


def check_transport(module):
    """Exercise value-returning and inherited output-reference transport."""
    record = cppyy.gbl.pysystemc_tlm_put_compound.Record
    request = record()
    request.code = 37
    owner = TransportOwner()
    port = sc.sc_port[tlm.tlm_transport_if[record, record]]("transport")
    port.bind(owner)
    interface = port.get_interface()
    assert isinstance(interface, tlm.tlm_transport_if[record, record])
    value_result = interface.transport(request)
    output = record()
    output.code = -1
    interface.transport(request, output)
    assert value_result.code == output.code == 46
    assert owner.calls == 2
    print("TLM_TRANSPORT", value_result.code, output.code, owner.calls)


module_name = sc.sc_module_name("tlm_put_compound_top")
module = sc.sc_module(module_name)
check_put(module)
check_compound(module, "master")
check_compound(module, "slave")
check_fifo(module)
check_transport(module)
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
print("TLM_PUT_COMPOUND_PUBLIC_OK")
