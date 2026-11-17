"""Compare public scalar bit/logic operators with exact native expressions."""

from __future__ import annotations

import json
import operator

import amal.eda.systemc as systemc
from amal.eda.systemc import sc_core as sc, sc_dt as dt

import cppyy


cppyy.cppdef(
    r"""
namespace pysystemc_scalar_bit_dispatch
{ /** @brief Evaluate exact native scalar expressions for an independent reference.  * @details Inputs are borrowed only
     for each call; native value results own  * their storage. Calls run with the GIL held, retain no Python references,
     * and propagate configured SystemC reports and C++ exceptions. */
template <class Left, class Right>
struct Reference
{ /** @brief Return the native conjunction expression. */
  static auto bit_and(const Left& left, const Right& right)
  {
    return left & right;
  } /** @brief Return the native disjunction expression. */
  static auto bit_or(const Left& left, const Right& right)
  {
    return left | right;
  } /** @brief Return the native exclusive-or expression. */
  static auto bit_xor(const Left& left, const Right& right)
  {
    return left ^ right;
  } /** @brief Return the native equality expression as a copied boolean. */
  static bool equal(const Left& left, const Right& right)
  {
    return left == right;
  } /** @brief Return the native inequality expression as a copied boolean. */
  static bool not_equal(const Left& left, const Right& right)
  {
    return left != right;
  }
};
}  // namespace pysystemc_scalar_bit_dispatch
    """
)


def _argument_type(value):
    """Return the exact template argument used by scalar Python operators."""
    if type(value) is str:
        return "char"
    if type(value) in (bool, int):
        return "bool" if type(value) is bool else "int"
    return type(value)


def _type_name(value):
    """Return a stable native or Python type name for result comparisons."""
    return getattr(type(value), "__cpp_name__", type(value).__name__)


def _value(value):
    """Render native bit and logic values as their state character."""
    if isinstance(value, (dt.sc_bit, dt.sc_logic)):
        return str(value.to_char())
    return value


def _descriptor(native, operation_name, reverse):
    """Return the Python adapter descriptor selected by one public expression."""
    suffix = {"bit_and": "and", "bit_or": "or", "bit_xor": "xor"}.get(operation_name)
    name = (
        f"__r{suffix}__"
        if reverse
        else f"__{suffix}__"
        if suffix
        else {
            "equal": "__eq__",
            "not_equal": "__ne__",
        }[operation_name]
    )
    return getattr(type(native), name)


def _cache(descriptor):
    """Find an optional descriptor wrapper's dispatch dictionary."""
    wrapper = getattr(descriptor, "wrapper", descriptor)
    for cell in getattr(wrapper, "__closure__", None) or ():
        try:
            contents = cell.cell_contents
        except ValueError:
            continue
        if isinstance(contents, dict):
            return contents
    return None


def _key_text(key):
    """Format a dispatch key without process-specific object representations."""
    return tuple(getattr(item, "__cpp_name__", item) for item in key)


def _snapshot(descriptor):
    """Capture cache contents and callable identities for a stable descriptor."""
    cache = _cache(descriptor)
    if cache is None:
        return None, (), {}
    keys = tuple(sorted((_key_text(key) for key in cache), key=repr))
    identities = {_key_text(key): id(value) for key, value in cache.items()}
    return len(cache), keys, identities


def _reference(operation_name, left, right):
    """Evaluate an independent exact C++ expression-template reference."""
    key = (_argument_type(left), _argument_type(right))
    reference = cppyy.gbl.pysystemc_scalar_bit_dispatch.Reference[key[0], key[1]]
    return getattr(reference, operation_name)(left, right)


def _operand_state(value):
    """Record a native operand's current value, address and Python ownership."""
    native = hasattr(type(value), "__cpp_name__")
    return {
        "native": native,
        "address": int(cppyy.addressof(value)) if native else None,
        "value": _value(value) if native else value,
        "owned": bool(getattr(value, "__python_owns__", False)) if native else None,
    }


PUBLIC_OPERATORS = {
    "bit_and": operator.and_,
    "bit_or": operator.or_,
    "bit_xor": operator.xor,
    "equal": operator.eq,
    "not_equal": operator.ne,
}
BITWISE_OPERATIONS = ("bit_and", "bit_or", "bit_xor")
OPERATIONS = tuple(PUBLIC_OPERATORS)


def _check_one(operation_name, left, right, *, reverse=False):
    """Check native parity, independent ownership and one exact cache key."""
    comparison = operation_name in ("equal", "not_equal")
    native = left if hasattr(type(left), "__cpp_name__") else right if comparison or reverse else left
    descriptor = _descriptor(native, operation_name, False if comparison else reverse)
    dispatch_left, dispatch_right = (left, right)
    if comparison:
        dispatch_left, dispatch_right = native, right if native is left else left
    key = (_argument_type(dispatch_left), _argument_type(dispatch_right))
    cache = _cache(descriptor)
    before = _snapshot(descriptor)
    source_states = (_operand_state(left), _operand_state(right))

    public = PUBLIC_OPERATORS[operation_name](left, right)
    reference = _reference(operation_name, left, right)
    after = _snapshot(descriptor)
    public_type, reference_type = _type_name(public), _type_name(reference)
    public_value, reference_value = _value(public), _value(reference)
    if public_type != reference_type or public_value != reference_value:
        raise AssertionError(
            f"{operation_name} mismatch for {public_type} {public_value!r} and {reference_type} {reference_value!r}"
        )

    if operation_name in BITWISE_OPERATIONS:
        if not getattr(public, "__python_owns__", False) or not getattr(reference, "__python_owns__", False):
            raise AssertionError(f"{operation_name} returned a result without owned native storage")
        result_addresses = (int(cppyy.addressof(public)), int(cppyy.addressof(reference)))
        input_addresses = tuple(state["address"] for state in source_states if state["native"])
        if result_addresses[0] == result_addresses[1] or any(
            address in input_addresses for address in result_addresses
        ):
            raise AssertionError(f"{operation_name} aliased an input or its reference result")
    elif type(public) is not bool or type(reference) is not bool:
        raise AssertionError(f"{operation_name} did not return copied Python booleans")

    for value, state in zip((left, right), source_states, strict=True):
        if state["native"] and (
            int(cppyy.addressof(value)) != state["address"]
            or _value(value) != state["value"]
            or bool(getattr(value, "__python_owns__", False)) != state["owned"]
        ):
            raise AssertionError(f"{operation_name} changed an input value, address or ownership")

    if cache is None:
        raise AssertionError(f"{operation_name} descriptor has no observable dispatch cache")
    if _cache(descriptor) is not cache or key not in cache or not callable(cache[key]):
        raise AssertionError(f"{operation_name} did not cache its successful exact type key")
    callable_id = id(cache[key])
    return {
        "descriptor": descriptor,
        "cache": cache,
        "cache_before": before,
        "cache_after": after,
        "key": key,
        "callable_id": callable_id,
        "public": public,
        "reference": reference,
        "result_type": public_type,
        "result_value": public_value,
    }


def _values(kind, family):
    """Return two valid values for one native or scalar operand category."""
    if kind == "bit":
        return dt.sc_bit("0"), dt.sc_bit("1")
    if kind == "logic":
        return dt.sc_logic("0"), dt.sc_logic("1")
    if kind == "bool":
        return False, True
    if kind == "char":
        return "0", "1"
    if kind == "int":
        return 0, 1
    raise AssertionError(f"unknown operand kind {kind}")


def _new_value(kind, value):
    """Construct an independent operand of one supported scalar category."""
    if kind == "bit":
        return dt.sc_bit(value)
    if kind == "logic":
        return dt.sc_logic(value)
    if kind == "bool":
        return bool(value)
    if kind == "char":
        return str(value)
    return int(value)


def _path_cases(family, native_kind, other_kind, reverse, operation_name):
    """Yield discriminating initial/current-value pairs for one public path."""
    native_values = _values(native_kind, family)
    other_values = _values(other_kind, family)
    if reverse:
        initial = (other_values[0], native_values[1])
        if operation_name == "bit_or":
            changed = (other_values[0], native_values[0])
        else:
            changed = (other_values[1], native_values[1])
    else:
        initial = (native_values[0], other_values[1])
        if operation_name == "bit_or":
            changed = (native_values[0], other_values[0])
        else:
            changed = (native_values[1], other_values[1])
    kinds = (other_kind, native_kind) if reverse else (native_kind, other_kind)
    return (
        tuple(_new_value(kind, value) for kind, value in zip(kinds, initial, strict=True)),
        tuple(_new_value(kind, value) for kind, value in zip(kinds, changed, strict=True)),
    )


def _exercise_paths():
    """Exercise forward/reverse bitwise dispatch and all scalar comparisons."""
    rows = 0
    observed = {}
    for family, native_kind in (("bit", "bit"), ("logic", "logic")):
        partners = (native_kind, "bool", "char", "int")
        for other_kind in partners:
            for reverse in (False,) if other_kind == native_kind else (False, True):
                for operation_name in OPERATIONS:
                    if operation_name in BITWISE_OPERATIONS:
                        initial, changed = _path_cases(family, native_kind, other_kind, reverse, operation_name)
                    elif reverse:
                        native_values = _values(native_kind, family)
                        other_values = _values(other_kind, family)
                        initial = tuple(
                            _new_value(kind, value)
                            for kind, value in zip(
                                (other_kind, native_kind), (other_values[0], native_values[1]), strict=True
                            )
                        )
                        changed = tuple(
                            _new_value(kind, value)
                            for kind, value in zip(
                                (other_kind, native_kind), (other_values[1], native_values[1]), strict=True
                            )
                        )
                    else:
                        native_values = _values(native_kind, family)
                        other_values = _values(other_kind, family)
                        initial = tuple(
                            _new_value(kind, value)
                            for kind, value in zip(
                                (native_kind, other_kind), (native_values[0], other_values[1]), strict=True
                            )
                        )
                        changed = tuple(
                            _new_value(kind, value)
                            for kind, value in zip(
                                (native_kind, other_kind), (native_values[1], other_values[1]), strict=True
                            )
                        )
                    first = _check_one(operation_name, *initial, reverse=reverse)
                    second = _check_one(operation_name, *changed, reverse=reverse)
                    if first["callable_id"] != second["callable_id"]:
                        raise AssertionError(f"{operation_name} replaced a callable for changed values")
                    if first["cache_after"] != second["cache_after"]:
                        raise AssertionError(f"{operation_name} changed cache keys on a current-value hit")
                    if first["result_value"] == second["result_value"]:
                        raise AssertionError(f"{operation_name} returned a stale or nondiscriminating value")
                    observed[(family, other_kind, reverse, operation_name)] = second
                    rows += 2

    # Cover all native sc_logic states through the same warmed native/native keys.
    logic_descriptor = dt.sc_logic.__and__
    for state in ("0", "1", "Z", "X"):
        for operation_name in BITWISE_OPERATIONS + ("equal", "not_equal"):
            left = dt.sc_logic(state)
            right = dt.sc_logic("1")
            result = _check_one(operation_name, left, right)
            if result["cache_after"] != result["cache_before"]:
                raise AssertionError(f"logic {state} call did not hit its warmed descriptor cache")
            rows += 1
    if _cache(logic_descriptor) is None:
        raise AssertionError("sc_logic adapter does not expose its dispatch cache")
    return rows, observed


def _invalid_operands():
    """Ensure invalid public operands raise without inserting cache entries."""
    native = dt.sc_bit("1")
    probes = (("bit_and", operator.and_), ("bit_or", operator.or_), ("bit_xor", operator.xor))
    rows = 0
    for operation_name, public_operator in probes:
        for reverse in (False, True):
            descriptor = _descriptor(native, operation_name, reverse)
            before = _snapshot(descriptor)
            for invalid in (None, object()):
                operands = (invalid, native) if reverse else (native, invalid)
                try:
                    public_operator(*operands)
                except TypeError:
                    pass
                else:
                    raise AssertionError(f"{operation_name} accepted {type(invalid).__name__}")
                if _snapshot(descriptor) != before:
                    raise AssertionError(f"{operation_name} cached an invalid operand type")
                rows += 1
    return rows


def _capture_report(label, operation):
    """Return a configured native report's message or fail if none is raised."""
    try:
        operation()
    except sc.sc_report as report:
        return report.get_msg()
    raise AssertionError(f"{label} did not throw a SystemC report")


def _report_recovery():
    """Compare repeated invalid-char reports and verify same-key recovery."""
    handler = sc.sc_report_handler
    owner = dt.sc_bit("1")
    owner_value = _value(owner)
    owner_address = int(cppyy.addressof(owner))
    descriptor = dt.sc_bit.__and__
    cache = _cache(descriptor)
    if cache is None:
        raise AssertionError("sc_bit and adapter has no observable dispatch cache")
    key = (dt.sc_bit, "char")
    if key not in cache or not callable(cache[key]):
        raise AssertionError("sc_bit/char dispatch key was not warmed by valid operator cases")
    cached_callable_id = id(cache[key])
    cache_before = _snapshot(descriptor)

    previous_actions = handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
    try:
        public_messages = [_capture_report("public sc_bit and invalid char", lambda: owner & "Q") for _ in range(2)]
        native_messages = [
            _capture_report(
                "native sc_bit and invalid char",
                lambda: _reference("bit_and", owner, "Q"),
            )
            for _ in range(2)
        ]
        public_result = owner & "1"
        native_result = _reference("bit_and", owner, "1")
        recovery_parity = (
            _type_name(public_result) == _type_name(native_result)
            and _value(public_result) == _value(native_result) == "1"
            and bool(public_result.__python_owns__)
            and bool(native_result.__python_owns__)
            and int(cppyy.addressof(public_result)) != int(cppyy.addressof(native_result))
            and int(cppyy.addressof(public_result)) != owner_address
            and int(cppyy.addressof(native_result)) != owner_address
        )
        cache_after = _snapshot(descriptor)
        callable_stable = (
            _cache(descriptor) is cache
            and key in cache
            and id(cache[key]) == cached_callable_id
            and cache_before == cache_after
        )
        owner_preserved = int(cppyy.addressof(owner)) == owner_address and _value(owner) == owner_value == "1"
    finally:
        handler.set_actions(sc.SC_ERROR, previous_actions)

    return {
        "public_messages": public_messages,
        "native_messages": native_messages,
        "messages_match": public_messages == native_messages,
        "public_report_count": len(public_messages),
        "native_report_count": len(native_messages),
        "recovery_parity": recovery_parity,
        "owner_preserved": owner_preserved,
        "cache_callable_stable": callable_stable,
    }


_path_count, _observed = _exercise_paths()
_invalid_count = _invalid_operands()
_reports = _report_recovery()

SCALAR_BIT_RESULT = {
    "package_imported_before_cppyy": True,
    "package_version": str(systemc.__version__),
    "native_parity": True,
    "cache_present": all(_cache(result["descriptor"]) is result["cache"] for result in _observed.values()),
    "cache_callable_stable": _reports["cache_callable_stable"]
    and all(
        callable(result["cache"][result["key"]]) and id(result["cache"][result["key"]]) == result["callable_id"]
        for result in _observed.values()
    ),
    "report_recovery_parity": _reports["messages_match"]
    and _reports["recovery_parity"]
    and _reports["owner_preserved"],
    "public_report_count": _reports["public_report_count"],
    "native_report_count": _reports["native_report_count"],
    "paths_checked": _path_count,
    "invalid_operands_checked": _invalid_count,
    "public_report_messages": _reports["public_messages"],
    "native_report_messages": _reports["native_messages"],
}

if not all(
    (
        SCALAR_BIT_RESULT["native_parity"],
        SCALAR_BIT_RESULT["cache_present"],
        SCALAR_BIT_RESULT["cache_callable_stable"],
        SCALAR_BIT_RESULT["report_recovery_parity"],
        SCALAR_BIT_RESULT["public_report_count"] == 2,
        SCALAR_BIT_RESULT["native_report_count"] == 2,
    )
):
    raise AssertionError(f"scalar bit dispatch reference checks failed: {SCALAR_BIT_RESULT}")

print("SCALAR_BIT_RESULT=" + json.dumps(SCALAR_BIT_RESULT, sort_keys=True, separators=(",", ":")))
