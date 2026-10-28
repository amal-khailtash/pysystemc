"""Probe scoped view, container, trace and event ownership contracts."""

import gc
from pathlib import Path
import sys
import weakref

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy


def collect():
    """Run collection before checking a weak-reference lifetime boundary."""
    gc.collect()


class EventModel(sc.sc_module):
    """Own an input port under its native module hierarchy."""

    def __init__(self, name):
        """Create and bind the signal and port before elaboration ends."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.signal = sc.sc_signal[bool]("f6_event_signal", False)
        self.port = sc.sc_in[bool]("f6_event_port")
        self.port.bind(self.signal)


def sc_dt_views():
    """Check borrowed numeric views, fixed-point parent tokens and rollback."""
    native = Path(__file__).resolve().parents[1] / "native"
    cppyy.include(str(native / "sc_dt_integer_bitrefs_probe.h"))
    cppyy.include(str(native / "sc_dt_integer_subrefs_probe.h"))
    cppyy.include(str(native / "sc_dt_vector_subrefs_probe.h"))
    cppyy.include(str(native / "f6_view_ownership.h"))
    bit_probe = cppyy.gbl.sc_dt_integer_bitrefs_probe
    range_probe = cppyy.gbl.sc_dt_integer_subrefs_probe
    fixed_probe = cppyy.gbl.f6_view_ownership

    integer_cases = (
        ("bounded_signed", lambda: dt.sc_int[19](0x12345), bit_probe.const_int_view, range_probe.const_int_range),
        ("bounded_unsigned", lambda: dt.sc_uint[19](0x12345), bit_probe.const_uint_view, range_probe.const_uint_range),
        ("arbitrary_signed", lambda: dt.sc_signed(137), bit_probe.const_signed_view, range_probe.const_signed_range),
        (
            "arbitrary_unsigned",
            lambda: dt.sc_unsigned(137),
            bit_probe.const_unsigned_view,
            range_probe.const_unsigned_range,
        ),
    )
    for label, make_owner, const_bit, const_range in integer_cases:
        owner = make_owner()
        owner.__assign__(0)
        before_owner = weakref.ref(owner)
        mutable_bit = owner[7]
        readonly_bit = const_bit(owner, 7)
        mutable_range = owner(13, 6)
        readonly_range = const_range(owner, 13, 6)
        assert not mutable_bit.__python_owns__ and not readonly_bit.__python_owns__
        assert not mutable_range.__python_owns__ and not readonly_range.__python_owns__
        bit_copy, readonly_bit_copy = type(mutable_bit)(mutable_bit), type(readonly_bit)(readonly_bit)
        range_copy, readonly_range_copy = type(mutable_range)(mutable_range), type(readonly_range)(readonly_range)
        assert all(view.__python_owns__ for view in (bit_copy, readonly_bit_copy, range_copy, readonly_range_copy))
        mutable_bit.__assign__(True)
        assert bool(readonly_bit)
        mutable_range.__assign__("0xAA")
        assert int(readonly_range.to_uint64()) == 0b10101010
        assert int(range_copy.to_uint64()) == 0b10101010
        del owner
        collect()
        assert before_owner() is None, f"{label} proxy shells unexpectedly retain their parent"
        # Do not dereference these C++ proxy copies after their caller-owned parent dies.
        del bit_copy, readonly_bit_copy, range_copy, readonly_range_copy
        del mutable_bit, readonly_bit, mutable_range, readonly_range
        collect()

    vector_cases = (
        ("binary_vector", dt.sc_bv_base, "10100101" * 17),
        ("four_state_vector", dt.sc_lv_base, "01XZ" * 32),
    )
    for label, base_type, spelling in vector_cases:
        owner = base_type(spelling)
        owner_ref = weakref.ref(owner)
        mutable_bit = owner[73]
        readonly_bit = dt.sc_bitref_r[base_type](owner, 73)
        mutable_range = owner.range(91, 73)
        readonly_range = dt.sc_subref_r[base_type](owner, 91, 73)
        assert type(mutable_bit).__cpp_name__ == f"sc_dt::sc_bitref<{base_type.__cpp_name__}>"
        assert type(readonly_bit).__cpp_name__ == f"sc_dt::sc_bitref_r<{base_type.__cpp_name__}>"
        assert type(mutable_range).__cpp_name__ == f"sc_dt::sc_subref<{base_type.__cpp_name__}>"
        assert type(readonly_range).__cpp_name__ == f"sc_dt::sc_subref_r<{base_type.__cpp_name__}>"
        mutable_bit.__assign__("1")
        assert bool(readonly_bit.to_bool())
        mutable_range.__assign__("1" * 19)
        assert str(readonly_range) == "1" * 19, label
        assert str(mutable_range) == str(readonly_range)
        del owner
        collect()
        assert owner_ref() is None, f"{label} borrowed proxies unexpectedly retain their parent"
        # The borrowed views are intentionally not used after releasing owner.
        del mutable_bit, readonly_bit, mutable_range, readonly_range
        collect()

    for label, owner_type in (
        ("fixed", dt.sc_fixed[32, 16]),
        ("fixed_fast", dt.sc_fixed_fast[32, 16]),
    ):
        owner = owner_type("3.25")
        owner_ref = weakref.ref(owner)
        bit = owner.bit(4)
        subref = owner.range(23, 16)
        is_fast = label == "fixed_fast"
        readonly_bit = (fixed_probe.fast_const_bit if is_fast else fixed_probe.fixed_const_bit)(owner, 4)
        readonly_range = (fixed_probe.fast_const_range if is_fast else fixed_probe.fixed_const_range)(owner, 23, 16)
        assert bit._pysystemc_parent is owner
        assert subref._pysystemc_parent is owner
        assert not hasattr(readonly_bit, "_pysystemc_parent")
        assert not hasattr(readonly_range, "_pysystemc_parent")
        assert readonly_bit.__python_owns__ and readonly_range.__python_owns__
        del owner
        collect()
        assert owner_ref() is not None, f"{label} borrowed views lost their attached parent token"
        bit.__assign__(True)
        assert bool(readonly_bit)
        retained_owner = owner_ref()
        assert retained_owner is not None
        del bit, subref
        collect()
        assert owner_ref() is retained_owner
        del readonly_bit, readonly_range, retained_owner
        collect()
        assert owner_ref() is None, f"{label} parent token outlived the final Python view"

    left = dt.sc_bv[64]("10100101" * 8)
    right = dt.sc_lv[64]("01011010" * 8)
    left_ref, right_ref = weakref.ref(left), weakref.ref(right)
    shell = dt.concat(left.range(31, 16), right.range(31, 16))
    assert shell.__python_owns__, type(shell).__cpp_name__
    shell.__assign__("1111000000001111" * 2)
    assert str(left.range(31, 16)) == "1111000000001111"
    readonly_left = dt.sc_subref_r[dt.sc_bv_base](left, 31, 16)
    readonly_right = dt.sc_subref_r[dt.sc_lv_base](right, 31, 16)
    readonly_shell = dt.concat(readonly_left, readonly_right)
    assert readonly_shell.__python_owns__, type(readonly_shell).__cpp_name__
    assert str(readonly_shell) == str(readonly_left) + str(readonly_right)
    assert left_ref() is left and right_ref() is right
    del readonly_shell, readonly_left, readonly_right, shell, left, right
    collect()
    assert left_ref() is None and right_ref() is None

    concat_left, concat_right = dt.sc_unsigned(16), dt.sc_unsigned(16)
    concat_left.__assign__(0xA55A)
    concat_right.__assign__(0x35C3)
    concat_ref = dt.concat(concat_left, concat_right)
    assert type(concat_ref).__cpp_name__ == "sc_dt::sc_concatref"
    assert not concat_ref.__python_owns__
    concat_copy = dt.sc_unsigned(concat_ref)
    assert concat_copy.__python_owns__
    assert int(concat_copy.to_uint64()) == 0xA55A35C3
    del concat_ref, concat_left, concat_right
    collect()

    owner = dt.sc_bv[16]("1010010110100101")
    view = owner.range(11, 4)
    before = str(owner)
    before_view = str(view)
    try:
        view.__assign__(object())
    except TypeError:
        pass
    else:
        raise AssertionError("invalid vector-view source converted successfully")
    assert str(owner) == before, "failed proxy conversion partially mutated its parent"
    assert str(view) == before_view
    print("F6_16_VIEW_OWNERSHIP_OK integers=4 vectors=2 fixed=2 concat=mutable/const rollback=1")


def fixed_contexts():
    """Check copied FX context values and last-owner release for each context."""
    context_cases = (
        ("fixed_type", dt.sc_fxtype_context, lambda: dt.sc_fxtype_params(17, 5), "wl"),
        ("length", dt.sc_length_context, lambda: dt.sc_length_param(13), "len"),
        ("cast", dt.sc_fxcast_context, lambda: dt.sc_fxcast_switch(dt.SC_OFF), "to_string"),
    )
    for label, context_type, make_source, member in context_cases:
        source = make_source()
        source_ref = weakref.ref(source)
        baseline = context_type.default_value()
        context = context_type(source, dt.SC_LATER)
        context_ref = weakref.ref(context)
        value = context.value()
        assert not value.__python_owns__, f"{label} value() should borrow the context's stored value"
        expected = getattr(source, member)()
        del source
        collect()
        assert source_ref() is None, f"{label} context retained the caller's source proxy instead of its value"
        observed = getattr(context.value(), member)()
        assert observed == expected, (label, observed, expected)
        del value
        del context
        collect()
        assert context_ref() is None, f"{label} context owner did not release after its last wrapper died"
        assert context_type.default_value() == baseline, f"{label} SC_LATER changed global defaults"

        active = context_type(make_source(), dt.SC_LATER)
        active.begin()
        active_value = active.value()
        now = context_type(make_source(), dt.SC_NOW)
        now_ref = weakref.ref(now)
        assert context_type.default_value() == now.value(), f"{label} SC_NOW did not install its value"
        del now
        collect()
        assert now_ref() is None, f"{label} SC_NOW context wrapper survived collection"
        assert context_type.default_value() == active_value, f"{label} SC_NOW failed to restore active context"
        active.end()
        del active, active_value
        collect()
        assert context_type.default_value() == baseline, f"{label} active context failed to restore baseline"

    # Invalid argument conversion must fail before a native context is created.
    try:
        dt.sc_fxtype_context(dt.sc_fxtype_params(21, 7), object())
    except TypeError:
        pass
    else:
        raise AssertionError("invalid context-begin selector was accepted")
    print("F6_17_CONTEXT_OWNERSHIP_OK forms=fxtype,length,cast copy=1 rollback=1")


def vector_owners():
    """Check creator and access-route owners, assembly borrowing and rollback."""
    cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_core_vector_container_probe.h"))
    probe = cppyy.gbl.sc_core_vector_container_probe
    element_type = probe.Element
    vector_type = sc.sc_vector[element_type]

    created_refs = []

    class Creator:
        """Allocate native vector elements and leave only weak audit handles."""

        def __call__(self, name, index):
            """Return one newly allocated native module to its vector."""
            element = element_type(name, index)
            created_refs.append(weakref.ref(element))
            return element

    creator = Creator()
    creator_ref = weakref.ref(creator)
    vector = vector_type("f6_vector", 2, creator)
    vector_ref = weakref.ref(vector)
    signal_vector = sc.sc_vector[sc.sc_signal[int]]("f6_signal_vector", 3)
    signal_vector[0].write(31)
    signal_vector.at(1).write(47)
    borrowed_elements = (signal_vector[0], signal_vector.at(1))
    borrowed_refs = tuple(weakref.ref(element) for element in borrowed_elements)
    assert all(not element.__python_owns__ for element in borrowed_elements)
    assert cppyy.addressof(signal_vector.begin()[0]) == cppyy.addressof(signal_vector[0])
    assert cppyy.addressof(signal_vector.at(1)) == cppyy.addressof(signal_vector[1])
    assert signal_vector.end() - signal_vector.begin() == 3
    del borrowed_elements
    collect()
    assert all(reference() is None for reference in borrowed_refs)
    assert vector.size() == 2 and vector.get_elements().size() == 2
    assert all(reference() is not None for reference in created_refs)

    payloads = vector.assemble("payload")
    payloads_ref = weakref.ref(payloads)
    assert payloads.size() == 2
    assert cppyy.addressof(payloads[1]) == cppyy.addressof(vector[1].payload)
    del creator, vector
    collect()
    assert creator_ref() is None, "synchronous creator callable leaked beyond init"
    assert vector_ref() is not None, "assembly failed to retain its known source vector"
    assert all(reference() is not None for reference in created_refs)
    assert int(payloads[1].read()) == 0
    del payloads
    collect()
    assert payloads_ref() is None and vector_ref() is None
    assert all(reference() is None for reference in created_refs), "vector kept returned element proxies after teardown"

    before_destroyed = int(probe.destroyed_counter())
    rollback_vector = vector_type("f6_vector_rollback")
    failed_refs = []

    def failing_creator(name, index):
        """Raise after one allocation so native partial-init rollback is observable."""
        if index == 1:
            raise RuntimeError("f6 creator failure")
        element = element_type(name, index)
        failed_refs.append(weakref.ref(element))
        return element

    try:
        rollback_vector.init(2, failing_creator)
    except RuntimeError as error:
        assert "f6 creator failure" in str(error)
    else:
        raise AssertionError("creator exception did not escape vector initialization")
    collect()
    assert rollback_vector.size() == 0
    assert rollback_vector.__dict__.get("_vector_element_owners", []) == []
    assert int(probe.destroyed_counter()) == before_destroyed + 1
    assert all(reference() is None for reference in failed_refs)
    del rollback_vector, signal_vector
    collect()
    print("F6_19_VECTOR_OWNERSHIP_OK creator=sync returned_proxies=2 assembly=retains rollback_destroyed=1")


def trace_sources():
    """Check borrowed trace sources, registration rollback and VCD transitions."""
    from amal.eda.systemc import sc_core as public_sc

    path = Path("f6_view_ownership")
    trace = public_sc.sc_create_vcd_trace_file(str(path))
    trace.set_time_unit(1, public_sc.SC_NS)
    signal = public_sc.sc_signal[bool]("f6_signal", False)
    vector = public_sc.sc_signal[dt.sc_bv[8]]("f6_vector_signal", dt.sc_bv[8]("00000000"))
    event = public_sc.sc_event("f6_event")
    signal_ref, vector_ref, event_ref = weakref.ref(signal), weakref.ref(vector), weakref.ref(event)
    public_sc.sc_trace(trace, signal, "f6_signal")
    public_sc.sc_trace(trace, vector, "f6_vector")
    public_sc.sc_trace(trace, event, "f6_event")

    holder = public_sc.sc_trace_value["unsigned int"](0)
    holder_ref = weakref.ref(holder)
    retained = len(trace.__dict__.get("_trace_value_owners", []))
    try:
        public_sc.sc_trace(trace, holder, "bad_labels", ["ok", 17])
    except TypeError:
        pass
    else:
        raise AssertionError("invalid enum labels registered a trace owner")
    assert len(trace.__dict__.get("_trace_value_owners", [])) == retained
    del holder
    collect()
    assert holder_ref() is None

    native_trace = cppyy.gbl.sc_core.sc_create_vcd_trace_file("f6_native_trace")
    native_trace.set_time_unit(1, cppyy.gbl.sc_core.SC_NS)
    native_signal = public_sc.sc_signal[int]("f6_native_signal", 9)
    native_ref = weakref.ref(native_signal)
    cppyy.gbl.sc_core.sc_trace(native_trace, native_signal, "f6_native_signal")

    sc.sc_start(1, sc.SC_NS)
    signal.write(True)
    vector.write(dt.sc_bv[8]("10101010"))
    event.notify(sc.sc_time(1, sc.SC_NS))
    sc.sc_start(2, sc.SC_NS)
    sc.sc_stop()
    public_sc.sc_close_vcd_trace_file(trace)
    cppyy.gbl.sc_core.sc_close_vcd_trace_file(native_trace)
    del signal, vector, event
    collect()
    assert signal_ref() is None and vector_ref() is None and event_ref() is None
    del native_signal
    collect()
    assert native_ref() is None
    print("F6_22_TRACE_OWNERSHIP_OK sources=scalar,vector,event failures=enum_rollback native_file=1")


def event_views():
    """Check event-list borrowing, finder cache ownership and sensitivity dispatch."""
    module = EventModel("f6_event_module")
    port = module.port
    interface_type = sc.sc_signal_in_if[bool]
    cache = sc.sc_event_finder_cache()
    cache_ref = weakref.ref(cache)
    cached = sc.sc_event_finder.cached_create[interface_type](cache, port, interface_type.value_changed_event)
    assert not cached.__python_owns__
    assert cached._pysystemc_cache_owner is cache
    del cache
    collect()
    assert cache_ref() is not None, "cached finder lost its Python cache owner"
    del cached
    collect()
    assert cache_ref() is None, "cached finder retained its cache after the last wrapper died"

    direct = sc.sc_event_finder_t[interface_type](port, interface_type.value_changed_event)
    direct_ref = weakref.ref(direct)
    assert direct.__python_owns__
    del direct
    collect()
    assert direct_ref() is None, "direct finder wrapper survived collection"

    first = sc.sc_event("f6_event_first")
    second = sc.sc_event("f6_event_second")
    first_ref, second_ref = weakref.ref(first), weakref.ref(second)
    first_list = sc.sc_event_or_list(first)
    expression = first_list | second
    try:
        expression | object()
    except TypeError:
        pass
    else:
        raise AssertionError("invalid event-expression composition succeeded")
    assert first_list.size() == 1
    expression_list = sc.sc_event_or_list(expression)
    assert expression_list.size() == 2
    # Converting/consuming an expression has one native use; do not revisit it.
    del expression, expression_list, first_list
    collect()
    assert first_ref() is first and second_ref() is second
    del first, second
    collect()
    assert first_ref() is None and second_ref() is None

    seen = []
    retained_event = sc.sc_event("f6_retained_event")

    def retained_method():
        """Record when the statically sensitive process dispatches."""
        seen.append(int(sc.sc_time_stamp().value()))

    module.sc_method("f6_retained_method", retained_method)
    module.sensitive(retained_event)
    del retained_method
    collect()
    handle = sc.sc_get_last_created_process_handle()
    handle_ref = weakref.ref(handle)
    retained_handle = sc.sc_process_handle(handle)
    assert retained_handle.valid()
    del handle
    collect()
    assert handle_ref() is None, "released process-handle wrapper survived collection"
    assert retained_handle.valid(), "separate process handle lost the registered process"
    retained_event.notify(sc.sc_time(1, sc.SC_NS))
    sc.sc_start(2, sc.SC_NS)
    assert seen, "sensitivity registration did not dispatch after GC"
    sc.sc_stop()
    del retained_handle
    collect()
    print(
        "F6_24_EVENT_OWNERSHIP_OK cache=retained_until_finder direct_finder=gc "
        "handle=separate_copy events=borrowed_failed_composition=1 dispatch=1"
    )


MODES = {
    "dt": sc_dt_views,
    "contexts": fixed_contexts,
    "vectors": vector_owners,
    "trace": trace_sources,
    "events": event_views,
}


if __name__ == "__main__":
    """Run one isolated ownership scope per subprocess invocation."""
    selected = sys.argv[1]
    if selected not in MODES:
        raise SystemExit(f"unknown ownership scope: {selected}")
    MODES[selected]()
