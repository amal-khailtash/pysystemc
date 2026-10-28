"""Exercise exact native callback pointer values and borrowed consumer ownership."""

from amal.eda.systemc import sc_core as sc
import cppyy
import gc
import weakref

cppyy.cppdef(r"""
int callback_pointer_values_global(const void*, const void*) { return 19; }
namespace callback_pointer_values {
struct item { int value; };
int compare_slots(const void* left, const void* right) {
  auto l = *static_cast<item* const*>(left);
  auto r = *static_cast<item* const*>(right);
  return (l->value > r->value) - (l->value < r->value);
}
int compare_items(const void* left, const void* right) {
  auto l = static_cast<const item*>(left);
  auto r = static_cast<const item*>(right);
  return (l->value > r->value) - (l->value < r->value);
}
void increment(void* data, void* context) {
  static_cast<item*>(data)->value += static_cast<item*>(context)->value;
}
int reports = 0;
void handler(const sc_core::sc_report&, const sc_core::sc_actions&) { ++reports; }
int wrong(int a, int b) { return a+b; }
int overloaded_compare(const void* left, const void* right) { return compare_items(left,right); }
int overloaded_compare(int left, int right) { return left-right; }
void overloaded_mapper(void* data, void* context) { increment(data,context); }
void overloaded_mapper(int, int) {}
void overloaded_handler(const sc_core::sc_report& report, const sc_core::sc_actions& actions) { handler(report,actions); }
void overloaded_handler(int, int) {}
int global_result(const pysystemc::callback_pointer<0>& pointer) { return pointer.get()(nullptr,nullptr); }
bool exact_overload(const pysystemc::callback_pointer<0>& pointer) {
 return pointer.get() == static_cast<sc_core::CFT>(&overloaded_compare);
}
int destroyed = 0;
struct tracked_queue: sc_core::sc_ppq<item*> {
  tracked_queue(int size, compare_fn_t cmp): sc_ppq(size,cmp) {}
  ~tracked_queue() { ++destroyed; }
};
static_assert(std::is_same<sc_core::CFT, sc_core::sc_ppq_base::compare_fn_t>::value);
static_assert(std::is_same<pysystemc::callback_pointer<0>::pointer_type, sc_core::CFT>::value);
static_assert(std::is_same<pysystemc::callback_pointer<1>::pointer_type, sc_core::sc_plist_map_fn>::value);
bool exact(const pysystemc::callback_pointer<0>& pointer) { return pointer.get() == &compare_items; }
}
""")
p = cppyy.gbl.callback_pointer_values
assert sc.CFT is sc.sc_ppq_base.compare_fn_t
for adapter in (sc.CFT, sc.sc_plist_map_fn, sc.sc_report_handler_proc):
    assert "exact native callback pointer" in adapter.__doc__
    empty = adapter()
    assert not empty.valid() and empty == adapter(empty)
    try:
        adapter(lambda *_: 1)
    except TypeError:
        pass
    else:
        raise AssertionError("Python callback must not become unretained holder")
    for bad in ("x", "&callback_pointer_values::compare_items", "a::b();"):
        try:
            adapter.from_function(bad)
        except TypeError:
            pass
        else:
            raise AssertionError("malformed native expression accepted")
    try:
        adapter.from_function("callback_pointer_values::wrong")
    except TypeError:
        pass
    else:
        raise AssertionError("wrong signature accepted")

global_pointer = sc.CFT.from_function("::callback_pointer_values_global")
assert p.global_result(global_pointer) == 19
assert sc.CFT.from_function("::callback_pointer_values::compare_items") == sc.CFT.from_function(
    "callback_pointer_values::compare_items"
)
assert p.exact_overload(sc.CFT.from_function("callback_pointer_values::overloaded_compare"))
assert sc.sc_plist_map_fn.from_function("callback_pointer_values::overloaded_mapper").valid()
assert sc.sc_report_handler_proc.from_function("callback_pointer_values::overloaded_handler").valid()
comparison = sc.CFT.from_function("callback_pointer_values::compare_items")
assert comparison.valid() and p.exact(comparison)
assert comparison == sc.CFT(comparison) and comparison != sc.CFT()
values = [p.item(3), p.item(1), p.item(2)]
vector = sc.sc_pvector[p.item.__cpp_name__ + "*"]()
for value in values:
    vector.push_back(value)
assert hasattr(vector.sort, "__overload__")
vector.sort(sc.CFT.from_function("callback_pointer_values::compare_slots"))
assert [vector.fetch(i).value for i in range(vector.size())] == [1, 2, 3]
try:
    vector.sort(sc.CFT())
except ValueError:
    pass
else:
    raise AssertionError("null sort comparator accepted")

linked = sc.sc_plist[p.item.__cpp_name__ + "*"]()
for value in values:
    linked.push_back(value)
context = p.item(4)
mapper = sc.sc_plist_map_fn.from_function("callback_pointer_values::overloaded_mapper")
assert hasattr(linked.mapcar, "__overload__")
linked.mapcar(mapper, context)
assert [value.value for value in values] == [7, 5, 6]
try:
    linked.mapcar(sc.sc_plist_map_fn(), context)
except ValueError:
    pass
else:
    raise AssertionError("null mapper accepted")
# Existing Python callback adaptation stays usable and observes all payloads.
mapped = []
linked.mapcar(lambda data, arg: mapped.append(bool(data)), cppyy.nullptr)
assert mapped == [True, True, True]

queue = comparison.create_queue(p.tracked_queue, 2)
assert queue.__python_owns__
for value in values:
    queue.insert(value)
# Queue borrows the native pointer value, so destroying its holder is safe while
# the selected native code remains loaded. Pointees stay alive in values.
reference = weakref.ref(comparison)
del comparison
gc.collect()
assert reference() is None
assert [queue.extract_top().value for _ in values] == [7, 6, 5]
assert queue.empty()
del queue
gc.collect()
assert p.destroyed == 1
try:
    sc.CFT().create_queue(sc.sc_ppq_base, 2)
except ValueError:
    pass
else:
    raise AssertionError("null comparator accepted")

handler = sc.sc_report_handler_proc.from_function("callback_pointer_values::overloaded_handler")
assert handler == sc.sc_report_handler_proc(handler) and handler.valid()
old = sc.sc_report_handler.set_handler(handler)
assert sc.sc_report_handler.get_handler() == handler
sc.sc_report_handler.report(sc.SC_INFO, "pointer-values", "exact handler", "probe", 9)
assert p.reports == 1
previous = sc.sc_report_handler.set_handler(old)
assert previous == handler and sc.sc_report_handler.get_handler() == old
print("CALLBACK_POINTER_VALUES exact null copy signatures sort map queue ownership report 1")
