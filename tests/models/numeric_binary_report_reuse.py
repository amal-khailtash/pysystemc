"""Check repeated remainder reports and successful reuse of one type key."""

import json

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


MARKER = "NUMERIC_BINARY_REPORT_REUSE="


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


def _binary_cache(descriptor):
    """Find the adapted binary wrapper's closure cache when one is present."""
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


def _cpp_reference(left, right):
    """Evaluate the independent C++ remainder expression template."""
    reference = cppyy.gbl.pysystemc_numeric_binary_report_reuse.Reference[type(left), type(right)]
    return reference.remainder(left, right)


cppyy.cppdef(
    "namespace pysystemc_numeric_binary_report_reuse { "
    "/** @brief Independently evaluate borrowed operands with the GIL held. */ "
    "template<class Left, class Right> struct Reference { "
    "/** @brief Return an owned native value; configured reports propagate. */ "
    "static auto remainder(const Left& left, const Right& right) { return left % right; } "
    "}; }"
)


numerator = dt.sc_signed(8)
numerator.__assign__(7)
denominator = dt.sc_signed(8)
denominator.__assign__(0)
native_type = type(numerator)
assert type(denominator) is native_type
descriptor = native_type.__mod__
cache = _binary_cache(descriptor)
dispatch_key = (native_type, native_type)
cache_before = None if cache is None else len(cache)
key_present_before = False if cache is None else dispatch_key in cache
cached_callable_id = None

handler = sc.sc_report_handler
previous_actions = handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
try:
    numerator_value = _value(numerator)
    denominator_value = _value(denominator)
    numerator_address = int(cppyy.addressof(numerator))
    denominator_address = int(cppyy.addressof(denominator))

    public_messages = [_capture_report("public remainder zero, first call", lambda: numerator % denominator)]
    cache_after_first_failure = _binary_cache(descriptor)
    if cache is not None:
        if cache_after_first_failure is not cache or dispatch_key not in cache:
            raise AssertionError("successful callable lookup was not cached before the native report")
        cached_callable = cache[dispatch_key]
        if not callable(cached_callable):
            raise AssertionError("binary dispatch cache retained a non-callable failure value")
        cached_callable_id = id(cached_callable)
    public_messages.append(_capture_report("public remainder zero, cached call", lambda: numerator % denominator))
    cache_after_second_failure = _binary_cache(descriptor)
    if cache is not None and (
        cache_after_second_failure is not cache
        or dispatch_key not in cache
        or id(cache[dispatch_key]) != cached_callable_id
    ):
        raise AssertionError("repeated native report did not reuse the original binary callable")

    reference_messages = [
        _capture_report("native remainder zero, first call", lambda: _cpp_reference(numerator, denominator)),
        _capture_report("native remainder zero, repeated call", lambda: _cpp_reference(numerator, denominator)),
    ]

    denominator.__assign__(2)
    denominator_after_mutation = _value(denominator)
    public_result = numerator % denominator
    reference_result = _cpp_reference(numerator, denominator)
    public_result_value = _value(public_result)
    reference_result_value = _value(reference_result)
    public_result_type = type(public_result).__cpp_name__
    reference_result_type = type(reference_result).__cpp_name__
    public_result_address = int(cppyy.addressof(public_result))
    reference_result_address = int(cppyy.addressof(reference_result))
    result_addresses_independent = (
        all(
            address not in (numerator_address, denominator_address)
            for address in (public_result_address, reference_result_address)
        )
        and public_result_address != reference_result_address
    )
    results_owned = bool(public_result.__python_owns__) and bool(reference_result.__python_owns__)
    sources_unchanged = (
        _value(numerator) == numerator_value
        and _value(denominator) == denominator_after_mutation
        and denominator_after_mutation == "2"
    )
    addresses_unchanged = (
        int(cppyy.addressof(numerator)) == numerator_address
        and int(cppyy.addressof(denominator)) == denominator_address
    )

    cache_after_success = _binary_cache(descriptor)
    if cache is not None:
        if cache_after_success is not cache or dispatch_key not in cache:
            raise AssertionError("same-type successful operation lost its cached dispatch")
        cache_callable_stable = id(cache[dispatch_key]) == cached_callable_id
        cache_after = len(cache)
    else:
        cache_callable_stable = True
        cache_after = None
finally:
    handler.set_actions(sc.SC_ERROR, previous_actions)


messages_match_native = public_messages == reference_messages
messages_report_division_by_zero = all(
    "division by zero" in message.lower() for message in public_messages + reference_messages
)
native_parity = public_result_type == reference_result_type and public_result_value == reference_result_value == "1"
cache_reused = cache is None or (
    dispatch_key in cache
    and callable(cache[dispatch_key])
    and cache_callable_stable
    and cache_before == 0
    and cache_after == 1
    and not key_present_before
)

REPORT_RESULT = {
    "public_report_count": len(public_messages),
    "native_report_count": len(reference_messages),
    "public_messages": public_messages,
    "native_messages": reference_messages,
    "messages_match_native": messages_match_native,
    "messages_report_division_by_zero": messages_report_division_by_zero,
    "cache_present": cache is not None,
    "cache_key_present_before": key_present_before,
    "cache_key_present_after_failure": cache is not None and dispatch_key in cache,
    "cache_callable_stable_through_success": cache_callable_stable,
    "cache_size_before": cache_before,
    "cache_size_after": cache_after,
    "cache_reused_without_storing_exception": cache_reused,
    "denominator_after_same_owner_mutation": denominator_after_mutation,
    "public_result_value": public_result_value,
    "native_result_value": reference_result_value,
    "public_result_type": public_result_type,
    "native_result_type": reference_result_type,
    "native_parity": native_parity,
    "results_owned": results_owned,
    "result_addresses_independent": result_addresses_independent,
    "sources_unchanged": sources_unchanged,
    "source_addresses_unchanged": addresses_unchanged,
}
print(MARKER + json.dumps(REPORT_RESULT, sort_keys=True, separators=(",", ":")))
