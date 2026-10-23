"""Compare Python sc_ref/sc_cref wrappers with generic native references."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy
import gc
import weakref


cppyy.cppdef(
    r"""
    #include <functional>
    #include <string>
    #include <systemc>
    #include <type_traits>
    #include <utility>
    #include <vector>

    struct RefCustomState {
      int value = 0;
      void add(int amount) { value += amount; }
    };
    using RefWide = sc_dt::sc_biguint<137>;
    using RefCustomConst = decltype(sc_core::sc_cref(std::declval<RefCustomState&>()));
    using RefCustomConstValue = decltype(std::declval<RefCustomConst>().get());
    static_assert(std::is_const_v<std::remove_reference_t<RefCustomConstValue>>);
    static_assert(!std::is_assignable_v<RefCustomConstValue, RefCustomState>);
    using RefWideConst = decltype(sc_core::sc_cref(std::declval<RefWide&>()));
    static_assert(std::is_const_v<std::remove_reference_t<decltype(std::declval<RefWideConst>().get())>>);

    int consume_custom(std::reference_wrapper<RefCustomState> state, int amount) {
      state.get().add(amount);
      return state.get().value;
    }
    int invoke_bound_custom(std::reference_wrapper<RefCustomState> state, int amount) {
      auto bound = sc_core::sc_bind(&RefCustomState::add, state, amount);
      bound();
      return state.get().value;
    }
    int read_const_custom(std::reference_wrapper<const RefCustomState> state) {
      return state.get().value;
    }
    void assign_wide(std::reference_wrapper<RefWide> value, unsigned int low) {
      RefWide expected = 1;
      expected <<= 136;
      expected += low;
      value.get() = expected;
    }
    bool wide_matches(std::reference_wrapper<const RefWide> value, unsigned int low) {
      RefWide expected = 1;
      expected <<= 136;
      expected += low;
      return value.get() == expected;
    }

    std::vector<std::string> ref_bound_records;
    void ref_bound_process(std::reference_wrapper<RefCustomState> state, int amount) {
      ref_bound_records.push_back("start@" + std::to_string(sc_core::sc_time_stamp().value()) + "=" +
                                  std::to_string(state.get().value));
      sc_core::wait(2, sc_core::SC_NS);
      state.get().add(amount);
      ref_bound_records.push_back("end@" + std::to_string(sc_core::sc_time_stamp().value()) + "=" +
                                  std::to_string(state.get().value));
    }
    sc_core::sc_process_handle spawn_ref_bound(std::reference_wrapper<RefCustomState> state, int amount) {
      auto callback = sc_core::sc_bind(&ref_bound_process, state, amount);
      return sc_core::sc_spawn(callback, "bound_reference_callback");
    }
    std::string ref_bound_record(int index) { return ref_bound_records.at(index); }
    """
)


def verify_wrapper_lifetime(factory, expected):
    """Check a reference wrapper retains and then releases its Python owner.

    Args:
        factory: Public wrapper function, either sc_ref or sc_cref.
        expected: Value that remains readable while the wrapper owns the proxy.
    """
    target = cppyy.gbl.RefCustomState()
    target.value = expected
    target_weak = weakref.ref(target)
    wrapper = factory(target)
    del target
    gc.collect()
    assert target_weak() is not None
    assert int(wrapper.get().value) == expected
    del wrapper
    gc.collect()
    assert target_weak() is None


state = cppyy.gbl.RefCustomState()
state.value = 10
reference = sc.sc_ref(state)
const_reference = sc.sc_cref(state)
assert cppyy.addressof(reference.get()) == cppyy.addressof(state)
assert cppyy.addressof(const_reference.get()) == cppyy.addressof(state)
reference.get().value = 12
consumed = cppyy.gbl.consume_custom(reference, 3)
assert consumed == 15 and int(const_reference.get().value) == 15
bound = cppyy.gbl.invoke_bound_custom(reference, 2)
assert bound == 17 and int(cppyy.gbl.read_const_custom(const_reference)) == 17

wide = dt.sc_biguint[137](5)
wide_reference = sc.sc_ref(wide)
wide_const_reference = sc.sc_cref(wide)
assert cppyy.addressof(wide_reference.get()) == cppyy.addressof(wide)
assert cppyy.addressof(wide_const_reference.get()) == cppyy.addressof(wide)
cppyy.gbl.assign_wide(wide_reference, 7)
assert cppyy.gbl.wide_matches(wide_const_reference, 7)

spawn_handle = cppyy.gbl.spawn_ref_bound(reference, 5)
assert spawn_handle.valid()
sc.sc_start()
assert int(state.value) == 22
bound_records = [cppyy.gbl.ref_bound_record(index) for index in range(2)]
assert bound_records == ["start@0=17", "end@2000=22"], bound_records

verify_wrapper_lifetime(sc.sc_ref, 31)
verify_wrapper_lifetime(sc.sc_cref, 41)
print("REF_CUSTOM_REFERENCE", f"value={int(state.value)} address=1 copies=1 const=17 consumed={consumed} bound={bound}")
print("REF_WIDE_REFERENCE width=137 address=1 low=7 high=1")
print("REF_BOUND_REFERENCE", *bound_records)
print("REF_LIFETIME_REFERENCE ref=retained-released cref=retained-released")
