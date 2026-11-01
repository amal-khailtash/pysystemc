"""Exercise typed member-pointer tokens with native SystemC references."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    r"""
#include <systemc>

namespace native_member_pointer_assembly_probe
{
/** @brief Vector element with a width-parameterized native SystemC field. */
template <int Width>
struct WideElement
{
  sc_dt::sc_biguint<Width> payload;
  int spare;
  explicit WideElement(const char*) : payload(0), spare(0)
  {
  }
};

using Element = WideElement<137>;
using PayloadPointer = sc_dt::sc_biguint<137> Element::*;

/** @brief Confirm a real native C++ pointer-to-member has exact identity. */
inline bool has_exact_payload_pointer()
{
  PayloadPointer pointer = &Element::payload;
  return pointer == &Element::payload;
}
/** @brief Build an independent reference assembly using a native pointer value.
 * @param vector Source vector, which must outlive the returned borrowed assembly.
 * @return Native assembly borrowing each element's payload member.
 */
inline auto native_payload_assembly(sc_core::sc_vector<Element>& vector)
{
  return sc_core::sc_assemble_vector(vector, &Element::payload);
}

/** @brief Port-owning vector element used to check binding through a token. */
struct PortElement
{
  sc_core::sc_in<bool> input;
  explicit PortElement(const char* name) : input(name)
  {
  }
};
}  // namespace native_member_pointer_assembly_probe
"""
)

probe = cppyy.gbl.native_member_pointer_assembly_probe
Element = probe.Element
Wide = cppyy.gbl.sc_dt.sc_biguint[137]
DECIMAL = cppyy.gbl.sc_dt.SC_DEC
Vector = sc.sc_vector[Element]


def expect(error_type, action):
    """Require one invalid public token operation to raise its documented error."""
    try:
        action()
    except error_type:
        return
    raise AssertionError(f"expected {error_type.__name__}")


def main():
    """Verify tokens, wide values, assembly routes, ownership and port binding."""
    assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
    token_type = sc.sc_member_access[Element, Wide].member_type
    null = token_type()
    assert not null.valid() and null == token_type() and null.same_as(token_type())

    token = token_type.from_member("payload")
    copied = token_type(token)
    qualified = token_type.from_member(f"{Element.__cpp_name__}::payload")
    assert token.valid() and copied.valid() and token == copied == qualified and token != null
    assert token.same_as(qualified) and probe.has_exact_payload_pointer()

    expect(TypeError, lambda: token_type.from_member("spare"))
    expect(TypeError, lambda: token_type(object()))
    expect(ValueError, lambda: token_type.from_member("payload + 1"))
    expect(ValueError, lambda: token_type.from_member("OtherElement::payload"))
    expect(TypeError, lambda: token_type.from_member("missing"))
    expect(TypeError, lambda: token_type.from_member("spare"))
    assert token_type.from_member("payload").valid()

    vector = Vector("wide_token_values", 2)
    values = [
        "87112285931760246646623899502532662132737",
        "87112285931760246646623899502532662133001",
    ]
    for index, value in enumerate(values):
        vector[index].payload = Wide(value)
    member_assembly = vector.assemble(token)
    free_assembly = sc.sc_assemble_vector(vector, copied)
    native_assembly = probe.native_payload_assembly(vector)
    assert member_assembly.size() == free_assembly.size() == native_assembly.size() == 2
    assert [member_assembly[index].to_string(DECIMAL) for index in range(2)] == values
    assert [free_assembly[index].to_string(DECIMAL) for index in range(2)] == values
    assert cppyy.addressof(member_assembly[0]) == cppyy.addressof(native_assembly[0])
    assert cppyy.addressof(free_assembly[1]) == cppyy.addressof(native_assembly[1])
    assert member_assembly._vector_owner is vector and free_assembly._vector_owner is vector

    other = Vector("wide_token_other", 2)
    other_assembly = other.assemble(token)
    assembly_type = type(member_assembly)
    copied_assembly = assembly_type(member_assembly)
    assigned_assembly = assembly_type(other_assembly)
    assigned_assembly.__assign__(member_assembly)
    assert copied_assembly._vector_owner is vector and assigned_assembly._vector_owner is vector
    assert cppyy.addressof(copied_assembly[0]) == cppyy.addressof(vector[0].payload)
    assigned_assembly.swap(other_assembly)
    assert assigned_assembly._vector_owner is other and other_assembly._vector_owner is vector

    cppyy.cppdef(
        """
/** @brief Differently typed element used to reject a mismatched token signature. */
struct OtherTokenElement {
  int payload;
  explicit OtherTokenElement(const char*) : payload(0) {}
};
"""
    )
    OtherElement = cppyy.gbl.OtherTokenElement
    other_vector = sc.sc_vector[OtherElement]("other_token_vector", 1)
    assert other_vector.size() == 1
    wrong_element_token = sc.sc_member_access[OtherElement, int].member_type.from_member("payload")
    expect(TypeError, lambda: vector.assemble(wrong_element_token))
    expect(TypeError, lambda: sc.sc_assemble_vector(vector, wrong_element_token))
    expect(TypeError, lambda: token_type(wrong_element_token))
    expect(ValueError, lambda: vector.assemble(null))
    expect(ValueError, lambda: sc.sc_assemble_vector(vector, null))
    expect(TypeError, lambda: sc.sc_assemble_vector(vector, object()))
    assert vector.assemble(token).size() == 2

    def make_borrowing_assembly():
        """Return an assembly whose Python owner keeps its source vector alive."""
        source = Vector("token_lifetime_source", 1)
        source[0].payload = Wide("123456789")
        owner_ref = weakref.ref(source)
        assembly = source.assemble(token)
        del source
        gc.collect()
        assert owner_ref() is assembly._vector_owner
        return assembly, owner_ref

    retained, owner_ref = make_borrowing_assembly()
    gc.collect()
    assert owner_ref() is not None and retained[0].to_string(DECIMAL) == "123456789"

    PortElement = probe.PortElement
    PortVector = sc.sc_vector[PortElement]
    SignalVector = sc.sc_vector[sc.sc_signal[bool]]
    module_name = sc.sc_module_name("token_binding_top")
    top = sc.sc_module(module_name)
    ports = PortVector("token_ports", 1)
    signals = SignalVector("token_signals", 1)
    port_token = sc.sc_member_access[PortElement, sc.sc_in[bool]].member_type.from_member("input")
    port_assembly = sc.sc_assemble_vector(ports, port_token)
    port_assembly.bind(signals)
    del module_name
    signals[0].write(True)
    sc.sc_start(sc.SC_ZERO_TIME)
    assert bool(ports[0].input.read())
    assert top.name() == "token_binding_top"

    print(
        f"MEMBER_POINTER_TOKEN wide=137 values={len(values)} routes=2 identity=1 copy=1 lifetime=1 binding=1 recovery=1"
    )


main()
