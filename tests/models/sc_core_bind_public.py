"""Verify native bind copying, references, placeholders and process callbacks."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(r"""
/** @brief Native state copied or borrowed by a binding. */
struct BindState { int value = 10; };
/** @brief Native functor with mutable copy-owned state. */
struct BindAdder {
  int offset = 5;
  int operator()(int a, int b) { return offset + a + b; }
};
/** @brief Sum two bound or supplied native arguments. */
int bind_sum(int a, int b) { return a + b; }
/** @brief Read copied or reference-wrapped state. */
int bind_state(const BindState& state, int add) { return state.value + add; }
/** @brief Mutate reference-wrapped state through a void-returning native callable. */
void bind_store(BindState& state, int value) { state.value = value; }
/** @brief Invoke a borrowed binding through its native const overload. */
template<class Binding> int bind_const(const Binding& bound) { return bound(); }
/** @brief Return borrowed state from a binding without converting it to a copy. */
BindState& bind_reference(BindState& state) { return state; }
""")

p1, p2 = sc.sc_placeholder[1](), sc.sc_placeholder[2]()
simple = sc.sc_bind(cppyy.gbl.bind_sum, 3, 4)
reordered = sc.sc_bind(cppyy.gbl.bind_sum, p2, p1)
duplicated = sc.sc_bind(cppyy.gbl.bind_sum, p1, p1)
nested = sc.sc_bind(cppyy.gbl.bind_sum, sc.sc_bind(cppyy.gbl.bind_sum, p1, 2), 3)
assert simple() == 7 and reordered(2, 9) == 11
assert cppyy.gbl.bind_const(simple) == 7
assert duplicated(6) == 12 and nested(8) == 13
functor = cppyy.gbl.BindAdder()
copied_functor = sc.sc_bind(functor, 1, 2)
functor.offset = 100
assert copied_functor() == 8
state = cppyy.gbl.BindState()
copied = sc.sc_bind(cppyy.gbl.bind_state, state, 1)
borrowed = sc.sc_bind(cppyy.gbl.bind_state, sc.sc_ref(state), 1)
const_borrowed = sc.sc_bind(cppyy.gbl.bind_state, sc.sc_cref(state), 1)
reference = sc.sc_bind(cppyy.gbl.bind_reference, sc.sc_ref(state))
state.value = 20
assert copied() == 11 and borrowed() == const_borrowed() == 21
assert cppyy.addressof(reference()) == cppyy.addressof(state)
print("BIND_VALUES", simple(), reordered(2, 9), duplicated(6), nested(8), copied_functor(), copied(), borrowed())

store = sc.sc_bind(cppyy.gbl.bind_store, sc.sc_ref(state), p1)
assert store(29) is None and state.value == 29

visits = []
cleanup = []


def callback(value):
    """Suspend a typed bound callback and preserve reset/kill cleanup."""
    visits.append((value, sc.sc_time_stamp().value()))
    try:
        sc.wait(1, sc.SC_NS)
        return value + 1
    finally:
        cleanup.append(sc.sc_time_stamp().value())


typed = cppyy.gbl.std.function["int(int)"](callback)
bound = sc.sc_bind(typed, 17)
handle = sc.sc_spawn(bound, "bound_worker")


def controller():
    """Reset a bound callback before its first completion."""
    sc.wait(0.25, sc.SC_NS)
    handle.reset()


sc.sc_spawn(controller, "bound_controller")
sc.sc_start(2, sc.SC_NS)
assert visits == [(17, 0), (17, 250)] and cleanup == [250, 1250]
assert handle.terminated()
print("BIND_PROCESS", *(f"{value}:{time}" for value, time in visits), *cleanup)
for invalid in (lambda: 1, object()):
    try:
        sc.sc_bind(invalid)
    except TypeError:
        pass
    else:
        raise AssertionError("untyped callable accepted")
print("BIND_OK")
sc.sc_stop()
