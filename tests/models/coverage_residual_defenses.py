"""Exercise audited Python-side defensive guards in a fresh interpreter."""

from amal.eda.systemc.sc_core import _hashes, _vectors


def _verify_hash_arity_guard() -> None:
    """Reject retry input that exceeds the inherited native constructor arity."""
    assert _hashes._completed_inherited_arguments((None,) * 8, {}) is None


def _verify_assembly_error_context() -> None:
    """Preserve post-selection assembly failure context and its original cause."""

    class FailingAssembly:
        """Model an already selected helper whose assembly operation fails."""

        @staticmethod
        def free(vector: object) -> None:
            """Raise the operation failure captured by the Python wrapper."""
            raise TypeError("assembly conversion failed")

    def fake_member_helper(element_type: object, member: str) -> type[FailingAssembly]:
        """Return the controlled helper after the requested member was accepted."""
        return FailingAssembly

    original_helper = _vectors._member_helper
    _vectors._member_helper = fake_member_helper
    try:
        try:
            _vectors._named_assembly(
                object(),
                object,
                "value",
                "free",
                "sc_assemble_vector",
                "an accessible field",
            )
        except TypeError as error:
            assert "sc_assemble_vector cannot use member 'value'" in str(error)
            assert isinstance(error.__cause__, TypeError)
            assert str(error.__cause__) == "assembly conversion failed"
        else:
            raise AssertionError("the assembly wrapper did not preserve the failure")
    finally:
        _vectors._member_helper = original_helper


def _verify_policy_error_context() -> None:
    """Preserve policy construction detail after compatibility has succeeded."""

    class FailingPolicyFactory:
        """Model a compatible adapter whose final native construction fails."""

        def __getitem__(self, member_type: object):
            """Return a specialization callable that raises the selected error."""

            def construct() -> None:
                """Raise the policy construction failure under test."""
                raise TypeError("policy construction failed")

            return construct

    class CompatibleQuery:
        """Model the helper's native member compatibility query."""

        def __getitem__(self, member_type: object):
            """Return a successful result for the requested value type."""

            def compatible() -> bool:
                """Report that the selected member value type is compatible."""
                return True

            return compatible

    class CompatibleHelper:
        """Expose compatible metadata and a controlled policy factory."""

        policy_compatible = CompatibleQuery()
        policy = FailingPolicyFactory()

    def fake_member_helper(element_type: object, member: str) -> type[CompatibleHelper]:
        """Return the controlled helper after the requested member was accepted."""
        return CompatibleHelper

    original_helper = _vectors._member_helper
    _vectors._member_helper = fake_member_helper
    try:
        try:
            _vectors._named_member_policy(
                object,
                int,
                "value",
                "sc_member_access",
                "a compatible member",
            )
        except TypeError as error:
            assert "sc_member_access cannot use member 'value'" in str(error)
            assert isinstance(error.__cause__, TypeError)
            assert str(error.__cause__) == "policy construction failed"
        else:
            raise AssertionError("the policy wrapper did not preserve the failure")
    finally:
        _vectors._member_helper = original_helper


def _verify_member_token_python_type_guard() -> None:
    """Reject a mismatched proxy type even when its C++ spelling collides."""
    token_name = "fixture::member_token"
    registered_type = type("RegisteredToken", (), {"__cpp_name__": token_name})
    forged_type = type("ForgedToken", (), {"__cpp_name__": token_name})
    expected_element = type("ExpectedElement", (), {"__cpp_name__": "fixture::Element"})
    fake_vector_type = type("FakeVector", (), {"element_type": expected_element})
    previous_registration = _vectors._member_tokens.get(token_name)
    _vectors._member_tokens[token_name] = (registered_type, expected_element, int)
    try:
        try:
            _vectors._assemble_member_token(fake_vector_type(), forged_type(), "free", "sc_assemble_vector")
        except TypeError as error:
            assert "requires the registered native member token type" in str(error)
        else:
            raise AssertionError("a mismatched token proxy was accepted")
    finally:
        if previous_registration is None:
            _vectors._member_tokens.pop(token_name, None)
        else:
            _vectors._member_tokens[token_name] = previous_registration


def _verify_malformed_member_identifier_guard() -> None:
    """Reject malformed member identifiers before any cppyy helper compilation."""
    try:
        _vectors._member_helper(object, "not-a-cpp-identifier")
    except ValueError as error:
        assert "must be a C++ identifier" in str(error)
    else:
        raise AssertionError("a malformed C++ member identifier was accepted")


_verify_hash_arity_guard()
_verify_assembly_error_context()
_verify_policy_error_context()
_verify_member_token_python_type_guard()
_verify_malformed_member_identifier_guard()
print("COVERAGE_RESIDUAL_DEFENSES_OK checks=5")
