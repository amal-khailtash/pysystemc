"""Check public sc_vector named-member failures, causes, and recovery."""

from amal.eda.systemc import sc_core as sc
import cppyy
from pathlib import Path

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_vector_access_probe.h"))
probe = cppyy.gbl.sc_core_vector_access_probe
Element = probe.Element


def expect_member_error(operation, member, action, cause_type):
    """Require an operation-specific error and its retained native cause.

    Args:
        operation: Public adapter spelling expected in the outer message.
        member: Rejected native member spelling.
        action: Zero-argument callable that performs the rejected selection.
        cause_type: Exception type expected from native reflection or overload selection.

    Returns:
        The native cause for additional detail checks.
    """
    try:
        action()
    except TypeError as error:
        assert operation in str(error)
        assert repr(member) in str(error)
        assert "member identifier" in str(error) or "member policy" in str(error)
        assert "pysystemc_vectors" not in str(error)
        assert "member_metadata_" not in str(error)
        assert isinstance(error.__cause__, cause_type), repr(error.__cause__)
        return error.__cause__
    raise AssertionError(f"{operation} accepted invalid member {member!r}")


def expect_bad_identifier(operation, action):
    """Require an operation-specific ValueError for a malformed identifier.

    Args:
        operation: Public adapter spelling expected in the exception.
        action: Zero-argument callable that performs the malformed selection.
    """
    try:
        action()
    except ValueError as error:
        assert operation in str(error)
        assert "C++ identifier" in str(error)
        return
    raise AssertionError(f"{operation} accepted a malformed identifier")


def main():
    """Exercise invalid forms, native cause preservation, and valid recovery."""
    vector = sc.sc_vector[Element]("named_error_elements", 1)
    policy = sc.sc_member_access[Element, int]

    missing_cause = expect_member_error(
        "sc_vector.assemble",
        "missing",
        lambda: vector.assemble("missing"),
        AttributeError,
    )
    assert "Element" in str(missing_cause) and "missing" in str(missing_cause)
    expect_member_error(
        "sc_assemble_vector",
        "missing",
        lambda: sc.sc_assemble_vector(vector, "missing"),
        AttributeError,
    )
    expect_member_error(
        "sc_member_access",
        "missing",
        lambda: policy("missing"),
        AttributeError,
    )

    expect_member_error(
        "sc_member_access",
        "destroyed_count",
        lambda: policy("destroyed_count"),
        AttributeError,
    )
    wrong_type_cause = expect_member_error(
        "sc_member_access",
        "value",
        lambda: sc.sc_member_access[Element, cppyy.gbl.std.string]("value"),
        TypeError,
    )
    assert "overloaded methods succeeded" in str(wrong_type_cause)
    assert "member_type ptr" in str(wrong_type_cause)

    expect_bad_identifier("sc_vector.assemble", lambda: vector.assemble("bad-name"))
    expect_bad_identifier("sc_assemble_vector", lambda: sc.sc_assemble_vector(vector, "bad-name"))
    expect_bad_identifier("sc_member_access", lambda: policy("bad-name"))

    vector[0].value = 41
    assembled_value = vector.assemble("value").begin().__deref__()
    policy_value = policy("value").get(vector[0])[0]
    assert assembled_value == policy_value == 41

    cppyy.cppdef(
        r"""
#include <vector>

namespace vector_error_presentation
{
/** @brief Base value used to check inherited native member-pointer conversion. */
struct Base
{
  int inherited = 0;
};

/** @brief Element with inherited and nested data members for unrestricted selection. */
struct Element : sc_core::sc_object, Base
{
  std::vector<int> nested;

  /** @brief Construct a named SystemC vector element. */
  explicit Element(const char* name) : sc_core::sc_object(name), Base(), nested()
  {
  }
};

/** @brief Element whose virtual base member cannot form a native member view. */
struct VirtualElement : sc_core::sc_object, virtual Base
{
  /** @brief Construct a named SystemC element with a virtual data base. */
  explicit VirtualElement(const char* name) : sc_core::sc_object(name), Base()
  {
  }
};
}  // namespace vector_error_presentation
"""
    )
    NestedElement = cppyy.gbl.vector_error_presentation.Element
    nested_vector = sc.sc_vector[NestedElement]("nested_named_error_elements", 1)
    nested_vector[0].inherited = 17
    nested_vector[0].nested.push_back(13)
    nested_vector[0].nested.push_back(29)
    inherited_policy = sc.sc_member_access[NestedElement, int]("inherited")
    inherited_view = nested_vector.assemble("inherited")
    nested_value_type = cppyy.gbl.std.vector[int]
    nested_policy = sc.sc_member_access[NestedElement, nested_value_type]("nested")
    nested_values = nested_policy.get(nested_vector[0])
    assert inherited_view.begin().__deref__() == inherited_policy.get(nested_vector[0])[0] == 17
    assert [nested_values[index] for index in range(nested_values.size())] == [13, 29]

    VirtualElement = cppyy.gbl.vector_error_presentation.VirtualElement
    virtual_policy = sc.sc_member_access[VirtualElement, int]
    expect_member_error(
        "sc_member_access",
        "inherited",
        lambda: virtual_policy("inherited"),
        AttributeError,
    )

    callback_vector = sc.sc_vector[Element]("creator_exception_identity")
    marker = LookupError("creator marker")

    def creator(name, index):
        """Raise the marker object to check callback exception identity."""
        raise marker

    try:
        callback_vector.init(1, creator)
    except LookupError as error:
        assert error is marker
        assert error.__cause__ is None
    else:
        raise AssertionError("vector creator exception was suppressed")

    cause_vector = sc.sc_vector[Element]("creator_exception_cause")
    marker_cause = RuntimeError("creator root cause")
    marker_with_cause = LookupError("creator marker with cause")

    def creator_with_cause(name, index):
        """Raise the marker with an explicit user-owned cause."""
        raise marker_with_cause from marker_cause

    try:
        cause_vector.init(1, creator_with_cause)
    except LookupError as error:
        assert error is marker_with_cause
        assert error.__cause__ is marker_cause
    else:
        raise AssertionError("creator exception cause was suppressed")

    print("VECTOR_ERROR_PRESENTATION errors=6 malformed=3 recovery=1 inherited=17 nested=13,29 creator_identity=2")


main()
