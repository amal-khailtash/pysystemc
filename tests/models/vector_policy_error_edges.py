"""Exercise public sc_vector policy reflection, conversion errors, and recovery."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_core_vector_access_probe.h"))
probe = cppyy.gbl.sc_core_vector_access_probe
Element = probe.Element


def emit(*fields):
    """Emit one stable policy observation for native comparison."""
    print("VPOLICY\t" + "\t".join(map(str, fields)))


def expect_type_error(label, action):
    """Require TypeError from one invalid public native-policy conversion."""
    try:
        action()
    except TypeError:
        emit("error", label, "TypeError")
    else:
        raise AssertionError(f"{label} was accepted")


def main():
    """Check aliases, nested member types, rejected conversions, and live access."""
    member_type = sc.sc_member_access[Element, int]
    nested_element_type = cppyy.gbl.std.pair[int, cppyy.gbl.std.vector[int]]
    nested_value_type = cppyy.gbl.std.vector[int]
    nested_member_type = sc.sc_member_access[nested_element_type, nested_value_type]
    native_template = cppyy.gbl.sc_core.sc_member_access
    element_name = Element.__cpp_name__

    template_name = sc.sc_member_access.__cpp_name__
    assert template_name == native_template.__cpp_name__
    assert sc.sc_member_access.__name__ == native_template.__name__

    native_member_type = native_template[Element, int]
    assert member_type.element_type.__cpp_name__ == element_name
    aliases_match_native = all(
        getattr(member_type, name).__cpp_name__ == getattr(native_member_type, name).__cpp_name__
        for name in ("type", "access_type", "policy", "non_const_policy", "const_policy")
    )
    assert aliases_match_native
    emit("aliases", int(aliases_match_native))

    native_nested_type = native_template[nested_element_type, nested_value_type]
    assert nested_member_type.element_type.__cpp_name__ == native_nested_type.element_type.__cpp_name__
    assert nested_member_type.type == nested_value_type
    assert nested_member_type.__cpp_name__ == native_nested_type.__cpp_name__
    assert nested_member_type.const_policy.__cpp_name__ == native_nested_type.const_policy.__cpp_name__
    nested_element = nested_element_type()
    nested_element.first = 7
    nested_element.second.push_back(11)
    nested_element.second.push_back(23)
    nested_policy = nested_member_type("second")
    nested_value = nested_policy.get(nested_element)
    nested_address_matches = cppyy.addressof(nested_value) == cppyy.addressof(nested_element.second)
    nested_value.push_back(37)
    nested_values = ",".join(str(nested_value[index]) for index in range(nested_value.size()))
    assert nested_address_matches and nested_values == "11,23,37"
    emit("nested", int(nested_address_matches), nested_element.first, nested_value.size(), nested_values)

    vector = sc.sc_vector[Element]("policy_error_elements", 1)
    element = vector[0]
    element.value = 29

    expect_type_error("member_value_type", lambda: member_type(29))
    expect_type_error("direct_missing_native_source", lambda: sc.sc_direct_access[Element](object()))

    try:
        member_type("value-not-an-identifier")
    except ValueError:
        emit("error", "member_name", "ValueError")
    else:
        raise AssertionError("invalid member identifier was accepted")

    member = member_type("value")
    copied = member_type(member)
    non_const = member_type.non_const_policy(member)
    const_member = member_type.const_policy(member)
    expect_type_error("member_const_to_mutable", lambda: member_type(const_member))
    direct_type = sc.sc_direct_access[Element]
    const_direct_type = sc.sc_direct_access[f"const {element_name}"]
    direct = direct_type(member)
    const_direct = const_direct_type(member)

    addresses_match = all(
        probe.same_member(policy.get(element), element) for policy in (member, copied, non_const, const_member)
    )
    direct_matches = probe.same_element(direct.get(element), element) and probe.same_element(
        const_direct.get(element), element
    )
    before = probe.read_member(member.get(element))
    member.get(element)[0] = 73
    after = probe.read_member(const_member.get(element))
    assert addresses_match and direct_matches and before == 29 and after == 73
    emit("live", int(addresses_match), int(direct_matches), before, after)

    # Keep the vector alive while every policy borrowing its element is used.
    assert vector.size() == 1


main()
