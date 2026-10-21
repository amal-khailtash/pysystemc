"""Verify public CFT comparison callbacks through native pointer-vector sorting."""

# Initialize cppyy/SystemC before defining native declarations below.
from amal.eda.systemc import sc_core as sc  # noqa: F401
import cppyy

cppyy.cppdef("""
/** Borrow native pointer-vector entries for a retained Python comparison. */
namespace pointer_sort_probe {
int values[3] = {3, 1, 2};
sc_core::sc_pvector<int*> items;
void prepare() { for (auto& value : values) items.push_back(&value); }
int read_argument(const void* pointer) { return **static_cast<int* const*>(pointer); }
int item(unsigned index) { return *items[index]; }
}
""")
probe = cppyy.gbl.pointer_sort_probe
probe.prepare()
records = []


def compare(left, right):
    """Compare pointees while native vector owns only borrowed pointers."""
    a, b = probe.read_argument(left), probe.read_argument(right)
    records.append((a, b))
    return (a > b) - (a < b)


probe.items.sort(compare)
assert records and [probe.item(index) for index in range(3)] == [1, 2, 3]
print("POINTER_SORT_REFERENCE 1 2 3")
