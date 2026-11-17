"""Check public boolean/bit concatenation against exact native overload calls.

The model covers mutable and const bit references from binary and four-state
vectors. It checks each operand order, live parent updates, copied Boolean
values, independent native result shells and borrowed proxy lifetimes.
"""

import gc
import weakref

from amal.eda.systemc import sc_dt as dt
import cppyy


cppyy.cppdef(
    r"""
#include <systemc>
#include <type_traits>

namespace concat_bool_bit_model
{
/** @brief Extract the vector owner type from either native bit proxy family. */
template <class Bit>
struct bit_owner;

template <class Owner>
struct bit_owner<sc_dt::sc_bitref<Owner>>
{
  using type = Owner;
};

template <class Owner>
struct bit_owner<sc_dt::sc_bitref_r<Owner>>
{
  using type = Owner;
};

/** @brief Call the independently selected native Boolean/bit concat overloads.
 * @tparam Bit Mutable or read-only vector bit proxy type.
 * @details This reference uses exact function-pointer casts, independently of
 * the production adapter. The result shell owns its cloned proxy and Boolean
 * storage while borrowing the vector's storage. Keep the parent alive while
 * reading the shell. Calls hold the GIL, do not suspend, and register no
 * callbacks. Native allocation and report exceptions propagate.
 */
template <class Bit>
struct exact_concat
{
  using Owner = typename bit_owner<Bit>::type;
  using TailResult = sc_dt::sc_concref_r<sc_dt::sc_bitref_r<Owner>, sc_dt::sc_bv_base>;
  using HeadResult = sc_dt::sc_concref_r<sc_dt::sc_bv_base, sc_dt::sc_bitref_r<Owner>>;

  /** @brief Return the exact native result for bit followed by Boolean.
   * @param bit A borrowed bit proxy whose parent must outlive the result.
   * @param value Boolean copied into the native result shell.
   * @return Owning read-only shell borrowing the vector storage.
   * @throws sc_core::sc_report If native concatenation reports an error.
   * @throws std::bad_alloc If native shell allocation fails.
   */
  static TailResult tail(Bit bit, bool value)
  {
    using Function = TailResult (*)(Bit, bool);
    const auto function = static_cast<Function>(&sc_dt::concat);
    return function(bit, value);
  }

  /** @brief Return the exact native result for Boolean followed by bit.
   * @param value Boolean copied into the native result shell.
   * @param bit A borrowed bit proxy whose parent must outlive the result.
   * @return Owning read-only shell borrowing the vector storage.
   * @throws sc_core::sc_report If native concatenation reports an error.
   * @throws std::bad_alloc If native shell allocation fails.
   */
  static HeadResult head(bool value, Bit bit)
  {
    using Function = HeadResult (*)(bool, Bit);
    const auto function = static_cast<Function>(&sc_dt::concat);
    return function(value, bit);
  }
};

/** @brief Extract a read-only bit proxy through a const binary vector owner.
 * @param owner Borrowed vector; it must outlive the returned proxy and results.
 * @param index Zero-based bit index.
 * @return A read-only proxy that borrows the supplied vector.
 * @throws sc_core::sc_report If the native bit index is out of range.
 */
inline auto readonly_bv_bit(const sc_dt::sc_bv_base& owner, int index) -> decltype(owner.bit(index))
{
  return owner.bit(index);
}

/** @brief Extract a read-only bit proxy through a const logic vector owner.
 * @param owner Borrowed vector; it must outlive the returned proxy and results.
 * @param index Zero-based bit index.
 * @return A read-only proxy that borrows the supplied vector.
 * @throws sc_core::sc_report If the native bit index is out of range.
 */
inline auto readonly_lv_bit(const sc_dt::sc_lv_base& owner, int index) -> decltype(owner.bit(index))
{
  return owner.bit(index);
}
}  // namespace concat_bool_bit_model
"""
)

REFERENCE = cppyy.gbl.concat_bool_bit_model.exact_concat
NATIVE = cppyy.gbl.concat_bool_bit_model


def _native_type_name(value):
    """Return the C++ type of one live native object."""
    name = getattr(type(value), "__cpp_name__", None)
    if not name:
        raise AssertionError(f"expected a native C++ value, got {type(value)!r}")
    return name


def _render(value):
    """Render a concatenation through its native binary string conversion."""
    return str(value.to_string())


def _expected(bit_value, boolean_value, order):
    """Return the two-character binary value expected for one operand order."""
    bit_text = "1" if bit_value else "0"
    bool_text = "1" if boolean_value else "0"
    return bit_text + bool_text if order == "tail" else bool_text + bit_text


def _make_proxy(parent, index, mutable):
    """Create a mutable or explicitly const-extracted bit proxy."""
    if mutable:
        return parent[index]
    parent_type = _native_type_name(parent)
    extractor = NATIVE.readonly_bv_bit if parent_type.startswith("sc_dt::sc_bv<") else NATIVE.readonly_lv_bit
    return extractor(parent, index)


def _public_concat(bit, boolean_value, order):
    """Invoke the public Python adapter in the requested operand order."""
    if order == "tail":
        return dt.concat(bit, boolean_value)
    return dt.concat(boolean_value, bit)


def _reference_concat(bit, boolean_value, order):
    """Invoke an independent exact native function-pointer reference."""
    exact = REFERENCE[type(bit)]
    if order == "tail":
        return exact.tail(bit, boolean_value)
    return exact.head(boolean_value, bit)


def _check_result(value, reference, parent, expected, label):
    """Check native type, value, length, ownership and independent shell storage."""
    value_type = _native_type_name(value)
    reference_type = _native_type_name(reference)
    assert value_type == reference_type, (label, value_type, reference_type)
    assert value_type.startswith("sc_dt::sc_concref_r<"), (label, value_type)
    assert value.length() == reference.length() == 2, label
    assert _render(value) == _render(reference) == expected, (
        label,
        _render(value),
        _render(reference),
        expected,
    )
    assert value.__python_owns__ and reference.__python_owns__, label

    public_address = int(cppyy.addressof(value))
    reference_address = int(cppyy.addressof(reference))
    parent_address = int(cppyy.addressof(parent))
    assert public_address != reference_address, label
    assert public_address != parent_address and reference_address != parent_address, label
    return public_address, reference_address


def _exercise_route(parent, index, mutable, order, label):
    """Check one public route against native exact dispatch under live mutation.

    The bit proxy and parent are borrowed by each result. Boolean values are
    copied into the result shell. Native result objects own distinct shells.
    """
    bit = _make_proxy(parent, index, mutable)
    assert bit.__python_owns__, (label, "bit proxy owns its shell while borrowing parent storage")
    assert _native_type_name(bit).startswith("sc_dt::sc_bitref"), label
    proxy_reference = weakref.ref(bit)
    parent_reference = weakref.ref(parent)
    bit_before = bool(bit)

    results = []
    addresses = []
    for boolean_value in (False, True):
        public_result = _public_concat(bit, boolean_value, order)
        native_result = _reference_concat(bit, boolean_value, order)
        addresses.append(
            _check_result(
                public_result,
                native_result,
                parent,
                _expected(bit_before, boolean_value, order),
                f"{label}_{boolean_value}",
            )
        )
        results.append((boolean_value, public_result, native_result))

    assert addresses[0][0] != addresses[1][0], (label, "public shell address reused")
    assert addresses[0][1] != addresses[1][1], (label, "reference shell address reused")

    # The vector bit remains borrowed by both shells and is observed live.
    changed_bit = not bit_before
    parent[index].__assign__(changed_bit)
    for boolean_value, public_result, native_result in results:
        expected = _expected(changed_bit, boolean_value, order)
        assert _render(public_result) == expected, (label, "public result lost live bit", expected)
        assert _render(native_result) == expected, (label, "reference lost live bit", expected)

    # Releasing the Python proxy must not release the separately retained parent.
    del bit
    gc.collect()
    assert proxy_reference() is None, (label, "result retained its Python proxy")
    assert parent_reference() is parent, (label, "parent was not retained by the model")
    assert all(
        _render(result) == _expected(changed_bit, boolean_value, order)
        for boolean_value, public_result, native_result in results
        for result in (public_result, native_result)
    ), (label, "shell stopped reading its live parent after proxy collection")

    # Drop all result shells before the next bit-proxy route; the parent stays live.
    del results
    gc.collect()
    assert parent_reference() is parent, label
    parent[index].__assign__(bit_before)


def _exercise_parent(parent_type, width, state, label):
    """Cover mutable/read-only head/tail routes for one vector parent type."""
    parent = parent_type(state * width)
    parent_ref = weakref.ref(parent)
    index = width // 2
    parent[index].__assign__(False)

    for mutable, mutability in ((True, "mutable"), (False, "readonly")):
        for order in ("head", "tail"):
            _exercise_route(parent, index, mutable, order, f"{label}_{mutability}_{order}")

    assert parent_ref() is parent
    assert _render(parent) == "0" * width, label
    return parent


def main():
    """Run all eight routes across binary and four-state vector parents."""
    parents = (
        _exercise_parent(dt.sc_bv[137], 137, "0", "bv"),
        _exercise_parent(dt.sc_lv[257], 257, "0", "lv"),
    )
    assert len(parents) == 2
    assert all(parent.length() in (137, 257) for parent in parents)
    print("BOOL_BIT_CONCAT_MODEL_OK parents=bv137/lv257 routes=mutable/readonly,head/tail")


if __name__ == "__main__":
    main()
