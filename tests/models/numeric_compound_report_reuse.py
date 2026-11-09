"""Check repeated compound remainder reports and reuse after recovery."""

import json
import operator

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


MARKER = "NUMERIC_COMPOUND_REPORT_REUSE="


def _value(value):
    """Render one native signed value as decimal text."""
    return str(value.to_string(dt.SC_DEC))


def _capture_report(label, operation):
    """Run one expression and return the message from its thrown report.

    Args:
        label: Operation label used when reporting a missing exception.
        operation: Zero-argument callable expected to raise ``sc_report``.
    Returns:
        The native report message.
    Raises:
        AssertionError: If the expression returns or throws another error.
    """
    try:
        operation()
    except sc.sc_report as report:
        return report.get_msg()
    raise AssertionError(f"{label} did not raise sc_report")


def _compound_cache(descriptor):
    """Find an adapted compound wrapper's closure cache if one is present."""
    wrapper = getattr(descriptor, "wrapper", None)
    if wrapper is None:
        return None
    for cell in wrapper.__closure__ or ():
        try:
            value = cell.cell_contents
        except ValueError:
            continue
        if isinstance(value, dict):
            return value
    return None


cppyy.cppdef(
    r"""
namespace pysystemc_numeric_compound_report_reuse
{
/**
 * @brief Mutate a borrowed target using its native compound remainder.
 * @details The caller keeps both owners live and holds the GIL for the call.
 *          The target is mutated in place; the source remains caller-owned.
 *          Configured native reports and C++ exceptions propagate. No
 *          callbacks are registered, so no callback lifetime is retained.
 *          No Python references or scheduler state persist, and this helper
 *          starts no native threads.
 * @tparam Target Native numeric target type.
 * @tparam Source Native numeric divisor type.
 */
template <class Target, class Source>
struct Reference
{
  /**
   * @brief Mutate borrowed storage; configured native reports propagate.
   * @param target Live native owner to mutate, borrowed for this call.
   * @param source Live native divisor, borrowed for this call.
   */
  static void compound_remainder(Target& target, const Source& source)
  {
    target %= source;
  }
};
}  // namespace pysystemc_numeric_compound_report_reuse
    """
)


def _cpp_compound_remainder(target, source):
    """Mutate a native reference owner through an independent C++ helper."""
    reference = cppyy.gbl.pysystemc_numeric_compound_report_reuse.Reference[type(target), type(source)]
    reference.compound_remainder(target, source)


public_owner = dt.sc_signed(8)
public_owner.__assign__(7)
public_denominator = dt.sc_signed(8)
public_denominator.__assign__(0)
native_owner = dt.sc_signed(8)
native_owner.__assign__(7)
native_denominator = dt.sc_signed(8)
native_denominator.__assign__(0)

native_type = type(public_owner)
assert type(public_denominator) is native_type
assert type(native_owner) is native_type
assert type(native_denominator) is native_type
descriptor = native_type.__imod__
cache = _compound_cache(descriptor)
dispatch_key = (native_type, native_type)
cache_before = None if cache is None else len(cache)
key_present_before = False if cache is None else dispatch_key in cache
cached_callable_id = None
if cache is not None and key_present_before:
    if not callable(cache[dispatch_key]):
        raise AssertionError("compound dispatch cache started with a non-callable value")
    cached_callable_id = id(cache[dispatch_key])

handler = sc.sc_report_handler
previous_actions = handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
try:
    public_owner_address = int(cppyy.addressof(public_owner))
    public_denominator_address = int(cppyy.addressof(public_denominator))
    native_owner_address = int(cppyy.addressof(native_owner))
    native_denominator_address = int(cppyy.addressof(native_denominator))
    public_owner_value_before_failures = _value(public_owner)
    native_owner_value_before_failures = _value(native_owner)
    public_owner_owns_before = bool(public_owner.__python_owns__)
    native_owner_owns_before = bool(native_owner.__python_owns__)
    public_denominator_owns_before = bool(public_denominator.__python_owns__)
    native_denominator_owns_before = bool(native_denominator.__python_owns__)

    public_messages = []
    native_messages = []
    public_failure_states = []
    native_failure_states = []
    cache_callable_stable = True
    for attempt in range(2):
        public_messages.append(
            _capture_report(
                f"public compound remainder zero, attempt {attempt + 1}",
                lambda: operator.imod(public_owner, public_denominator),
            )
        )
        public_failure_states.append(_value(public_owner))
        if attempt == 0 and cache is not None:
            if cache is not _compound_cache(descriptor) or dispatch_key not in cache:
                raise AssertionError("compound dispatch callable was not cached before the native report")
            cached_callable = cache[dispatch_key]
            if not callable(cached_callable):
                raise AssertionError("compound dispatch cache retained a non-callable failure value")
            if cached_callable_id is None:
                cached_callable_id = id(cached_callable)
            elif id(cached_callable) != cached_callable_id:
                raise AssertionError("first compound report replaced the cached callable")
        if cache is not None and (
            cache is not _compound_cache(descriptor)
            or dispatch_key not in cache
            or id(cache[dispatch_key]) != cached_callable_id
        ):
            raise AssertionError("repeated compound report did not reuse the original callable")

        native_messages.append(
            _capture_report(
                f"native compound remainder zero, attempt {attempt + 1}",
                lambda: _cpp_compound_remainder(native_owner, native_denominator),
            )
        )
        native_failure_states.append(_value(native_owner))
        if cache is not None and (
            cache is not _compound_cache(descriptor)
            or dispatch_key not in cache
            or id(cache[dispatch_key]) != cached_callable_id
        ):
            raise AssertionError("native compound report changed the cached public callable")

    if cache is not None:
        cache_callable_stable = id(cache[dispatch_key]) == cached_callable_id

    # Recovery starts from a known value regardless of native failure semantics.
    public_owner.__assign__(7)
    native_owner.__assign__(7)
    public_denominator.__assign__(2)
    native_denominator.__assign__(2)
    public_denominator_after_reset = _value(public_denominator)
    native_denominator_after_reset = _value(native_denominator)

    public_return = operator.imod(public_owner, public_denominator)
    native_return = _cpp_compound_remainder(native_owner, native_denominator)
    public_result_value = _value(public_owner)
    native_result_value = _value(native_owner)

    cache_after_success = _compound_cache(descriptor)
    if cache is not None:
        if cache_after_success is not cache or dispatch_key not in cache:
            raise AssertionError("same-type successful compound operation lost its cached dispatch")
        cache_callable_stable = cache_callable_stable and id(cache[dispatch_key]) == cached_callable_id
        cache_after = len(cache)
    else:
        cache_after = None

    public_owner_still_owned = bool(public_owner.__python_owns__)
    native_owner_still_owned = bool(native_owner.__python_owns__)
    public_denominator_still_owned = bool(public_denominator.__python_owns__)
    native_denominator_still_owned = bool(native_denominator.__python_owns__)
    ownership_unchanged = (
        public_owner_still_owned == public_owner_owns_before
        and native_owner_still_owned == native_owner_owns_before
        and public_denominator_still_owned == public_denominator_owns_before
        and native_denominator_still_owned == native_denominator_owns_before
    )
    sources_unchanged = (
        _value(public_denominator) == public_denominator_after_reset == "2"
        and _value(native_denominator) == native_denominator_after_reset == "2"
    )
    addresses_unchanged = (
        int(cppyy.addressof(public_owner)) == public_owner_address
        and int(cppyy.addressof(public_denominator)) == public_denominator_address
        and int(cppyy.addressof(native_owner)) == native_owner_address
        and int(cppyy.addressof(native_denominator)) == native_denominator_address
    )
    owner_addresses_distinct = public_owner_address != native_owner_address
    denominator_addresses_distinct = public_denominator_address != native_denominator_address
finally:
    handler.set_actions(sc.SC_ERROR, previous_actions)


messages_match_native = public_messages == native_messages
messages_report_division_by_zero = all(
    "division by zero" in message.lower() for message in public_messages + native_messages
)
failure_states_match_native = public_failure_states == native_failure_states
native_parity = public_result_value == native_result_value == "1"
cache_reused_without_storing_exception = cache is None or (
    dispatch_key in cache
    and callable(cache[dispatch_key])
    and cache_callable_stable
    and cache_after == cache_before + (0 if key_present_before else 1)
)

REPORT_RESULT = {
    "public_report_count": len(public_messages),
    "native_report_count": len(native_messages),
    "public_messages": public_messages,
    "native_messages": native_messages,
    "messages_match_native": messages_match_native,
    "messages_report_division_by_zero": messages_report_division_by_zero,
    "public_failure_states": public_failure_states,
    "native_failure_states": native_failure_states,
    "failure_states_match_native": failure_states_match_native,
    "public_owner_value_before_failures": public_owner_value_before_failures,
    "native_owner_value_before_failures": native_owner_value_before_failures,
    "public_return_is_owner": public_return is public_owner,
    "native_helper_return_is_void": native_return is None,
    "public_result_value": public_result_value,
    "native_result_value": native_result_value,
    "native_parity": native_parity,
    "public_owner_still_owned": public_owner_still_owned,
    "native_owner_still_owned": native_owner_still_owned,
    "public_denominator_still_owned": public_denominator_still_owned,
    "native_denominator_still_owned": native_denominator_still_owned,
    "ownership_unchanged": ownership_unchanged,
    "owner_addresses_distinct": owner_addresses_distinct,
    "denominator_addresses_distinct": denominator_addresses_distinct,
    "sources_unchanged": sources_unchanged,
    "addresses_unchanged": addresses_unchanged,
    "cache_present": cache is not None,
    "cache_key_present_before": key_present_before,
    "cache_key_present_after_failure": cache is not None and dispatch_key in cache,
    "cache_callable_stable_through_success": cache_callable_stable,
    "cache_size_before": cache_before,
    "cache_size_after": cache_after,
    "cache_reused_without_storing_exception": cache_reused_without_storing_exception,
}
print(MARKER + json.dumps(REPORT_RESULT, sort_keys=True, separators=(",", ":")))
