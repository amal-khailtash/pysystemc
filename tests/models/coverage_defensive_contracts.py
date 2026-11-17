"""Exercise defensive adapter contracts with explicit fault and metadata fixtures.

This model imports the real SystemC bindings first, then probes residual branches
that ordinary supported native objects cannot enter. Fault injection and the
alternate reflection fixture below verify error translation and metadata
forwarding; they do not establish additional native SystemC support.
"""

from amal.eda.systemc import sc_core as sc

import cppyy

from amal.eda.systemc import _native_help
from amal.eda.systemc.sc_core import _callback_pointers, _vector_policies, _vectors


def _check_enum_metadata_error() -> None:
    """Return an empty enum-member list when reflection has no instance dict."""
    assert _native_help._enum_members(object()) == []


def _check_hash_overload_forwarding() -> None:
    """Preserve reflected setter overload metadata on an adapted native fixture."""
    cppyy.cppdef(
        r"""
namespace coverage_defensive_contracts {
inline unsigned hash_calls = 0;
inline unsigned compare_calls = 0;

/** @brief Count calls through the exact SystemC hash callback signature. */
inline unsigned hash_key(const void* key)
{
  (void)key;
  ++hash_calls;
  return 0;
}

/** @brief Count calls through the exact SystemC comparison callback signature. */
inline int compare_keys(const void* left, const void* right)
{
  ++compare_calls;
  const int lhs = *static_cast<const int*>(left);
  const int rhs = *static_cast<const int*>(right);
  return (lhs > rhs) - (lhs < rhs);
}

/** @brief Expose reflected overloads while forwarding to the native hash base.
 * @details Callback pointers are borrowed from static functions in this fixture.
 * The table and its stack keys live only for the synchronous check; no Python
 * callbacks, GIL release, or simulation-kernel entry occur.
 */
class ReflectedHashConsumer : public sc_core::sc_phash_base
{
 public:
  /** @brief Construct a table before the exact callback holders are installed. */
  ReflectedHashConsumer()
      : sc_phash_base(nullptr, 11, 5, 2.0, true, nullptr, nullptr)
  {
  }

  using sc_phash_base::set_hash_fn;
  using sc_phash_base::set_cmpr_fn;

  /** @brief Preserve the native hash setter and expose another reflected overload. */
  void set_hash_fn(hash_fn_t callback, int)
  {
    sc_phash_base::set_hash_fn(callback);
  }

  /** @brief Preserve the native comparator setter and expose another overload. */
  void set_cmpr_fn(cmpr_fn_t callback, int)
  {
    sc_phash_base::set_cmpr_fn(callback);
  }

  /** @brief Exercise both callbacks while two borrowed stack keys remain alive. */
  bool exercise_callbacks()
  {
    hash_calls = 0;
    compare_calls = 0;
    int first_key = 1;
    int second_key = 2;
    insert(&first_key, nullptr);
    insert(&second_key, nullptr);
    return hash_calls >= 2 && compare_calls >= 1;
  }
};
}  // namespace coverage_defensive_contracts
"""
    )

    fixture = cppyy.gbl.coverage_defensive_contracts.ReflectedHashConsumer
    _callback_pointers._wrap_hash_consumer(fixture, "sc_phash_base")
    select_hash = fixture.set_hash_fn.__overload__
    select_compare = fixture.set_cmpr_fn.__overload__
    selected_hash = select_hash("sc_core::sc_phash_base::hash_fn_t, int")
    selected_compare = select_compare("sc_core::sc_phash_base::cmpr_fn_t, int")
    assert callable(selected_hash) and callable(selected_compare)

    hash_pointer = sc.sc_phash_base.hash_fn_t.from_function("coverage_defensive_contracts::hash_key")
    compare_pointer = sc.sc_phash_base.cmpr_fn_t.from_function("coverage_defensive_contracts::compare_keys")
    table = fixture()
    table.set_hash_fn(hash_pointer)
    table.set_cmpr_fn(compare_pointer)
    assert table.exercise_callbacks()
    selected_hash(table, hash_pointer.get(), 7)
    selected_compare(table, compare_pointer.get(), 7)
    assert table.exercise_callbacks()


def _check_member_alias_metadata_delegation() -> None:
    """Delegate unknown metadata through an explicitly reflected alias fixture."""
    cppyy.cppdef(
        r"""
namespace coverage_defensive_contracts {
/** @brief Native metadata target used to exercise cppyy alias delegation. */
struct AliasMetadata {
  /** @brief Nested reflected type used as an identity-bearing metadata value. */
  struct nested_type {};
};
}
"""
    )

    element_type = cppyy.gbl.coverage_defensive_contracts.ReflectedHashConsumer
    token_type = cppyy.gbl.pysystemc_vectors.member_pointer[element_type.__cpp_name__, "int"]
    native_alias = cppyy.gbl.coverage_defensive_contracts.AliasMetadata
    adapter = _vector_policies._MemberPointerType(
        native_alias,
        token_type,
        element_type,
        int,
    )
    assert adapter.nested_type is native_alias.nested_type
    try:
        adapter.missing_native_alias_metadata
    except AttributeError as error:
        assert "missing_native_alias_metadata" in str(error)
    else:
        raise AssertionError("missing reflected alias metadata did not propagate")


def _check_pointer_getter_backend_failures() -> None:
    """Translate invocation-time backend failures and restore the native getter."""
    cppyy.cppdef(
        r"""
namespace coverage_defensive_contracts {
/** @brief Element with a field selected by the native pointer getter. */
struct PointerElement {
  int value;
};
using element_member = int PointerElement::*;
inline element_member selected_member = &PointerElement::value;
}
"""
    )

    element_type = cppyy.gbl.coverage_defensive_contracts.PointerElement
    member_type = sc.sc_member_access[element_type, int].member_type
    symbol = "coverage_defensive_contracts::selected_member"
    token = member_type.from_pointer(symbol)
    assert token.valid()

    key = (element_type.__cpp_name__, "int", symbol)
    native_getter = _vectors._pointer_getters[key]
    try:
        for error_type in (SyntaxError, TypeError, ValueError):

            def fail_invocation(error_type=error_type):
                """Raise one selected cppyy invocation error for the adapter."""
                raise error_type("injected cppyy getter invocation failure")

            _vectors._pointer_getters[key] = fail_invocation
            try:
                member_type.from_pointer(symbol)
            except TypeError as error:
                assert "native pointer symbol '" + symbol + "' is unavailable or incompatible" == str(error)
                assert isinstance(error.__cause__, error_type)
                assert str(error.__cause__) == "injected cppyy getter invocation failure"
            else:
                raise AssertionError(f"{error_type.__name__} invocation failure was not translated")
    finally:
        _vectors._pointer_getters[key] = native_getter

    recovered = member_type.from_pointer(symbol)
    assert recovered.valid() and recovered == token
    assert recovered == member_type.from_member("value")


def main() -> None:
    """Run seven classified defensive statements without resetting kernel state."""
    _check_enum_metadata_error()
    _check_hash_overload_forwarding()
    _check_member_alias_metadata_delegation()
    _check_pointer_getter_backend_failures()
    print(
        "COVERAGE_DEFENSIVE_CONTRACTS_OK statements=7 "
        "enum=helper-robustness hash=reflected-fixture "
        "alias=alternate-metadata pointer=backend-fault-injection"
    )


main()
