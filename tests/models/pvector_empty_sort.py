"""Exercise callable and native CFT sorting for empty and populated vectors."""

# Initialize native SystemC and pointer adapters before adding C++ fixtures.
from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    """
    #include <cstdint>

    namespace pvector_empty_sort
    {
    sc_core::sc_time values[] = {
      sc_core::sc_time(3, sc_core::SC_NS),
      sc_core::sc_time(1, sc_core::SC_NS),
      sc_core::sc_time(2, sc_core::SC_NS)
    };
    int native_comparisons = 0;

    /** @brief Return one borrowed static fixture value by index. */
    sc_core::sc_time* pointer(unsigned int index)
    {
      return &values[index];
    }

    /** @brief Read the pointee value from a qsort-provided pointer slot. */
    double slot_nanoseconds(const void* slot)
    {
      auto value = *static_cast<sc_core::sc_time* const*>(slot);
      return value->to_seconds() * 1e9;
    }

    /** @brief Compare native pointer slots without throwing across qsort. */
    int compare_time_slots(const void* left, const void* right) noexcept
    {
      ++native_comparisons;
      const auto left_value = slot_nanoseconds(left);
      const auto right_value = slot_nanoseconds(right);
      return (left_value > right_value) - (left_value < right_value);
    }

    /** @brief Reset the native callback count before a sorting scenario. */
    void reset_native_comparisons()
    {
      native_comparisons = 0;
    }
    }
    """
)

probe = cppyy.gbl.pvector_empty_sort
TimeVector = sc.sc_pvector["sc_core::sc_time*"]
empty = TimeVector()
python_comparisons = []


def compare_time_slots(left, right):
    """Compare borrowed pointer slots by their native time values."""
    left_value = float(probe.slot_nanoseconds(left))
    right_value = float(probe.slot_nanoseconds(right))
    python_comparisons.append((left_value, right_value))
    return (left_value > right_value) - (left_value < right_value)


native_comparator = sc.CFT.from_function("pvector_empty_sort::compare_time_slots")

# Both public routes validate their comparator and leave an empty vector intact.
for _ in range(2):
    assert empty.sort(compare_time_slots) is None
    assert empty.size() == 0
    assert empty.sort(native_comparator) is None
    assert empty.size() == 0
assert python_comparisons == []
assert probe.native_comparisons == 0

try:
    empty.sort(sc.CFT())
except ValueError:
    pass
else:
    raise AssertionError("empty-vector sort accepted a null native comparator")

try:
    empty.sort(17)
except TypeError:
    pass
else:
    raise AssertionError("empty-vector sort accepted a non-callable comparator")
assert empty.size() == 0
assert python_comparisons == []
assert probe.native_comparisons == 0


# Keep native static pointees alive and compare their captured addresses after
# each sort so the test proves slots move while the pointees remain borrowed.
def fill(vector, indexes):
    """Append fixture pointers in the selected original order."""
    for index in indexes:
        vector.push_back(probe.pointer(index))


def addresses(vector):
    """Return pointee addresses borrowed from every existing native slot."""
    return [cppyy.addressof(vector.fetch(index)) for index in range(vector.size())]


def nanoseconds(vector):
    """Read sorted pointee values through their borrowed sc_time proxies."""
    return [round(float(vector.fetch(index).to_seconds() * 1e9)) for index in range(vector.size())]


python_vector = TimeVector()
fill(python_vector, (0, 1, 2))
python_addresses = addresses(python_vector)
python_comparisons.clear()
python_vector.sort(compare_time_slots)
assert python_comparisons
python_values = nanoseconds(python_vector)
assert python_values == [1, 2, 3], (python_values, python_comparisons)
assert addresses(python_vector) == [python_addresses[1], python_addresses[2], python_addresses[0]]

holder_vector = TimeVector()
fill(holder_vector, (0, 1, 2))
holder_addresses = addresses(holder_vector)
probe.reset_native_comparisons()
holder_vector.sort(native_comparator)
assert probe.native_comparisons > 0
holder_values = nanoseconds(holder_vector)
assert holder_values == [1, 2, 3], (holder_values, probe.native_comparisons)
assert addresses(holder_vector) == [holder_addresses[1], holder_addresses[2], holder_addresses[0]]
assert empty.size() == 0
print("PVECTOR_EMPTY_SORT_OK empty=2 routes=2 callbacks=0 populated=2")
