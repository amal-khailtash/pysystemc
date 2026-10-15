"""Validate native vector indexing and typed object attribute ownership."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef("""
namespace vector_probe {
struct Element : sc_core::sc_object {
    sc_core::sc_signal<int> value;
    explicit Element(const char* name) : sc_object(name), value(sc_core::sc_gen_unique_name("value")) {}
};
void check_assembly() {
    static sc_core::sc_vector<Element> elements("elements", 3);
    auto values = sc_core::sc_assemble_vector(elements, &Element::value);
    sc_assert(values.size() == 3);
    for (unsigned i = 0; i < 3; ++i) values[i].write(int(i + 1));

    for (unsigned i = 0; i < 3; ++i) sc_assert(&values[i] == &elements[i].value);
}
}
""")
module_name = sc.sc_module_name("inventory")
model = sc.sc_module(module_name)
vector = sc.sc_vector[sc.sc_signal[bool]]("signals", 3)
attribute = sc.sc_attribute[int]("identifier", 17)
assert model.add_attribute(attribute)
assert model.get_attribute("identifier").name() == "identifier"
assert model.num_attributes() == 1
assert attribute.value == 17
for index in range(3):
    vector[index].write(index % 2 == 0)
    assert vector[index].name() == f"inventory.signals_{index}"
cppyy.gbl.vector_probe.check_assembly()
del module_name
sc.sc_start(1, sc.SC_NS)
assert [vector[index].read() for index in range(3)] == [True, False, True]
removed = model.remove_attribute("identifier")
assert removed.name() == "identifier"
assert model.num_attributes() == 0
sc.sc_stop()
print("VECTOR_ATTRIBUTES_OK")
