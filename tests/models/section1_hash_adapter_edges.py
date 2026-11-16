"""Exercise native hash-constructor adapter reuse and rejection paths."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.include(str(Path(__file__).resolve().parents[1] / "native/hash_constructor_defaults_probe.h"))
Probe = cppyy.gbl.hash_constructor_defaults_probe.Probe
ProbeWithTag = cppyy.gbl.hash_constructor_defaults_probe.ProbeWithTag


def native_type_error(operation, label):
    """Capture one real cppyy constructor diagnostic for an invalid call.

    Args:
        operation: Zero-argument callable that invokes the native constructor.
        label: Name included if the native constructor unexpectedly accepts it.

    Returns:
        The original TypeError message raised by cppyy.

    Raises:
        AssertionError: The call succeeds or raises an exception other than TypeError.
    """
    try:
        operation()
    except TypeError as error:
        return str(error)
    raise AssertionError(f"native hash constructor accepted invalid {label} arguments")


invalid_calls = (
    ("unknown", lambda table_type: table_type(mystery=1)),
    ("overlap", lambda table_type: table_type(cppyy.nullptr, **{"def": cppyy.nullptr})),
    ("too_many", lambda table_type: table_type(*([cppyy.nullptr] * 8))),
    ("invalid_growth", lambda table_type: table_type(grow="invalid-growth")),
)
original_errors = {label: native_type_error(lambda call=call: call(Probe), label) for label, call in invalid_calls}

# The second distinct native class reuses the already installed C++ predicate.
assert sc.adapt_inherited_hash_constructor(Probe) is Probe
assert sc.adapt_inherited_hash_constructor(ProbeWithTag) is ProbeWithTag

adapted_errors = {label: native_type_error(lambda call=call: call(Probe), label) for label, call in invalid_calls}
assert adapted_errors == original_errors


def configuration(table):
    """Read the four native constructor settings exposed by the C++ probe.

    Args:
        table: A constructed native hash-base subclass.

    Returns:
        Bucket count, maximum density, growth factor, and reorder flag.
    """
    return table.bins(), table.density(), table.growth(), table.reorder()


default_probe = Probe()
tagged_probe = ProbeWithTag(23)
assert configuration(default_probe) == (11, 5, 2.0, True)
assert configuration(tagged_probe) == (11, 5, 2.0, True)
assert tagged_probe.tag() == 23

# This is SystemC's real templated hash subclass, whose constructor is its own
# typed overload and does not expose sc_phash_base's exact inherited signature.
TypedHash = sc.sc_phash[int, int]
try:
    sc.adapt_inherited_hash_constructor(TypedHash)
except TypeError as error:
    assert str(error) == "adapter target does not expose the inherited sc_phash_base constructor signature"
else:
    raise AssertionError("the adapter accepted native sc_phash<int, int>")

retry_shapes = [label for label, message in original_errors.items() if "takes at least 7 arguments" in message]
print("HASH_ADAPTER_EDGES predicate_reuse=ProbeWithTag signature_rejection=sc_phash preserved_errors=4")
print(f"HASH_ADAPTER_RETRY_DIAGNOSTICS {','.join(retry_shapes) or 'none'}")
