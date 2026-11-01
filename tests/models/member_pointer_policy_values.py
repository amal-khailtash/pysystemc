"""Exercise native member policies constructed from typed member tokens."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    r"""
#include <systemc>

namespace member_pointer_policy_probe
{
/** @brief Element template carrying a width-parameterized native value. */
template <int Width>
struct Element
{
  sc_dt::sc_biguint<Width> wide;
  int sequence;
  explicit Element(const char*) : wide(0), sequence(0)
  {
  }
};

using WideElement = Element<137>;
using Wide = sc_dt::sc_biguint<137>;
using Policy = sc_core::sc_member_access<WideElement, Wide>;

/** @brief Construct the native pointer-based policy reference. */
inline Policy native_policy()
{
  return Policy(&WideElement::wide);
}

/** @brief Check that a policy selects the exact native element member address.
 * @tparam AccessPolicy Mutable or const native member policy.
 * @param policy Native policy borrowing the selected member.
 * @param element Element that remains alive for the duration of the call.
 * @return True when get() returns that element's wide field.
 */
template <class AccessPolicy>
inline bool selects_member(const AccessPolicy& policy, WideElement& element)
{
  return policy.get(&element) == &element.wide;
}

/** @brief Compare mutable/const policy get() results for the same live element.
 * @tparam LeftPolicy First member policy specialization.
 * @tparam RightPolicy Second member policy specialization.
 * @param left First policy borrowing the member.
 * @param right Second policy borrowing the member.
 * @param element Live element used for both synchronous pointer queries.
 * @return True when both policies return the same member address.
 */
template <class LeftPolicy, class RightPolicy>
inline bool policies_select_same_member(const LeftPolicy& left, const RightPolicy& right, WideElement& element)
{
  return left.get(&element) == right.get(&element);
}
}  // namespace member_pointer_policy_probe
"""
)

probe = cppyy.gbl.member_pointer_policy_probe
Element = probe.WideElement
Wide = probe.Wide
WideType = cppyy.gbl.sc_dt.sc_biguint[137]
DECIMAL = cppyy.gbl.sc_dt.SC_DEC
Vector = sc.sc_vector[Element]


def expect(error_type, action):
    """Require one invalid policy construction to raise the expected exception."""
    try:
        action()
    except error_type:
        return
    raise AssertionError(f"expected {error_type.__name__}")


def main():
    """Verify token policy construction, native parity, mutation and lifetime."""
    assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
    policy_type = sc.sc_member_access[Element, WideType]
    token_type = policy_type.member_type
    token = token_type.from_member("wide")
    null = token_type()
    expect(ValueError, lambda: policy_type(null))
    expect(TypeError, lambda: policy_type(object()))

    wrong_member_token = sc.sc_member_access[Element, int].member_type.from_member("sequence")
    wrong_element_type = cppyy.gbl.member_pointer_policy_probe.Element[136]
    wrong_element_token = sc.sc_member_access[
        wrong_element_type, cppyy.gbl.sc_dt.sc_biguint[136]
    ].member_type.from_member("wide")
    expect(TypeError, lambda: policy_type(wrong_member_token))
    expect(TypeError, lambda: policy_type(wrong_element_token))

    vector = Vector("policy_token_elements", 2)
    values = [
        "87112285931760246646623899502532662132737",
        "87112285931760246646623899502532662133001",
    ]
    for index, value in enumerate(values):
        vector[index].wide = Wide(value)
        vector[index].sequence = index + 4

    policy = policy_type(token)
    copied_policy = policy_type(policy)
    native_policy = probe.native_policy()
    const_policy = policy_type.const_policy(policy)
    for index in range(2):
        element = vector[index]
        assert probe.selects_member(policy, element)
        assert probe.selects_member(copied_policy, element)
        assert probe.selects_member(native_policy, element)
        assert probe.selects_member(const_policy, element)
        assert probe.policies_select_same_member(policy, native_policy, element)
        assert probe.policies_select_same_member(const_policy, native_policy, element)
        assert policy.get(element).to_string(DECIMAL) == values[index]
        assert const_policy.get(element).to_string(DECIMAL) == values[index]

    replacement = "123456789012345678901234567890123456789"
    policy.get(vector[1]).__assign__(Wide(replacement))
    assert vector[1].wide.to_string(DECIMAL) == replacement
    assert const_policy.get(vector[1]).to_string(DECIMAL) == replacement

    def create_unowned_policy():
        """Return a borrowed policy after its source vector becomes collectible."""
        owner = Vector("policy_token_temporary", 1)
        owner_reference = weakref.ref(owner)
        borrowed_policy = policy_type(token)
        return owner_reference, borrowed_policy

    owner_reference, detached_policy = create_unowned_policy()
    gc.collect()
    assert owner_reference() is None
    # Do not call get() on detached_policy: policies borrow elements and the owner is gone.
    assert type(detached_policy).__cpp_name__ == policy_type.__cpp_name__

    recovered = Vector("policy_token_recovery", 1)
    recovered_policy = policy_type(token)
    recovered[0].wide = Wide("987654321")
    assert probe.selects_member(recovered_policy, recovered[0])
    assert recovered_policy.get(recovered[0]).to_string(DECIMAL) == "987654321"
    assert vector.size() == 2 and recovered.size() == 1

    print("MEMBER_POINTER_POLICY wide=137 native=1 copy=1 const=1 mutation=1 lifetime=1 recovery=1")


main()
