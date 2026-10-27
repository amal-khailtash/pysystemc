"""Compare native and Python bound callable return categories and lifetimes."""

import gc

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    r"""
    int f8_void_value = 0;

    /** @brief State returned by reference from a bound callable. */
    struct F8ReferenceState { int value = 0; };

    /** @brief Copy-only result type with a live-object counter. */
    struct F8CopyOnly {
      static int live;
      static int copies;
      int value;
      explicit F8CopyOnly(int input) : value(input) { ++live; }
      F8CopyOnly(const F8CopyOnly& other) : value(other.value) { ++live; ++copies; }
      F8CopyOnly(F8CopyOnly&&) = delete;
      ~F8CopyOnly() { --live; }
    };
    int F8CopyOnly::live = 0;
    int F8CopyOnly::copies = 0;

    /** @brief Move-only result type with a live-object counter. */
    struct F8MoveOnly {
      static int live;
      int value;
      explicit F8MoveOnly(int input) : value(input) { ++live; }
      F8MoveOnly(const F8MoveOnly&) = delete;
      F8MoveOnly(F8MoveOnly&& other) noexcept : value(other.value) { ++live; }
      ~F8MoveOnly() { --live; }
    };
    int F8MoveOnly::live = 0;

    /** @brief Return an integer value for bind result testing. */
    int f8_value(int input) { return input + 2; }
    /** @brief Return void after updating observable native state. */
    void f8_void(int input) { f8_void_value = input; }
    /** @brief Return the mutable state member by reference. */
    int& f8_reference(F8ReferenceState& state) { return state.value; }
    /** @brief Construct a copy-only return value. */
    F8CopyOnly f8_copy_only(int input) { return F8CopyOnly(input); }
    /** @brief Construct a move-only return value. */
    F8MoveOnly f8_move_only(int input) { return F8MoveOnly(input); }
    /** @brief Throw a native exception to exercise the binder exception path. */
    int f8_throw() { throw std::runtime_error("f8 binder exception"); }

    template<class Binding> int f8_invoke_value(Binding& binding) { return binding(); }
    template<class Binding> void f8_invoke_void(Binding& binding) { binding(); }
    template<class Binding>
    bool f8_invoke_reference(Binding& binding, F8ReferenceState& state) {
      return &binding() == &state.value;
    }
    template<class Binding> int f8_invoke_copy_only(Binding& binding) {
      const F8CopyOnly value = binding();
      return value.value;
    }
    template<class Binding> int f8_invoke_move_only(Binding& binding) {
      F8MoveOnly value = binding();
      return value.value;
    }
    template<class Binding> bool f8_invoke_throw(Binding& binding) {
      try { binding(); }
      catch (const std::runtime_error& error) {
        return std::string(error.what()) == "f8 binder exception";
      }
      return false;
    }

    int f8_native_value(int input) {
      auto binding = sc_core::sc_bind(&f8_value, input);
      return f8_invoke_value(binding);
    }
    int f8_native_void(int input) {
      auto binding = sc_core::sc_bind(&f8_void, input);
      f8_invoke_void(binding);
      return f8_void_value;
    }
    bool f8_native_reference(int input) {
      F8ReferenceState state{input};
      auto binding = sc_core::sc_bind(&f8_reference, sc_core::sc_ref(state));
      return f8_invoke_reference(binding, state);
    }
    int f8_native_copy_only(int input) {
      auto binding = sc_core::sc_bind(&f8_copy_only, input);
      return f8_invoke_copy_only(binding);
    }
    int f8_native_move_only(int input) {
      auto binding = sc_core::sc_bind(&f8_move_only, input);
      return f8_invoke_move_only(binding);
    }
    bool f8_native_throw() {
      auto binding = sc_core::sc_bind(&f8_throw);
      return f8_invoke_throw(binding);
    }
    """
)


def compare_bound_result_categories():
    """Compare native and public bind calls across result and exception types.

    Returns:
        A tuple of matching native/public observations and live-object counts.
    """
    native = cppyy.gbl
    value = sc.sc_bind(native.f8_value, 17)
    python_value = native.f8_invoke_value(value)
    native_value = native.f8_native_value(17)

    void = sc.sc_bind(native.f8_void, 23)
    native.f8_invoke_void(void)
    python_void = int(native.f8_void_value)
    native_void = native.f8_native_void(23)

    reference_state = native.F8ReferenceState()
    reference_state.value = 29
    reference = sc.sc_bind(native.f8_reference, sc.sc_ref(reference_state))
    python_reference = bool(native.f8_invoke_reference(reference, reference_state))
    native_reference = bool(native.f8_native_reference(29))

    copy_only = sc.sc_bind(native.f8_copy_only, 31)
    python_copy_only = int(native.f8_invoke_copy_only(copy_only))
    python_copy_live = int(native.F8CopyOnly.live)
    copy_result = copy_only()
    direct_copy_value = int(copy_result.value)
    direct_copy_live = int(native.F8CopyOnly.live)
    del copy_result
    gc.collect()
    direct_copy_released = int(native.F8CopyOnly.live)
    native_copy_only = int(native.f8_native_copy_only(31))
    native_copy_live = int(native.F8CopyOnly.live)

    move_only = sc.sc_bind(native.f8_move_only, 37)
    python_move_only = int(native.f8_invoke_move_only(move_only))
    python_move_live = int(native.F8MoveOnly.live)
    move_result = move_only()
    direct_move_value = int(move_result.value)
    direct_move_live = int(native.F8MoveOnly.live)
    del move_result
    gc.collect()
    direct_move_released = int(native.F8MoveOnly.live)
    native_move_only = int(native.f8_native_move_only(37))
    native_move_live = int(native.F8MoveOnly.live)

    throws = sc.sc_bind(native.f8_throw)
    python_throw = bool(native.f8_invoke_throw(throws))
    native_throw = bool(native.f8_native_throw())
    return (
        python_value,
        native_value,
        python_void,
        native_void,
        python_reference,
        native_reference,
        python_copy_only,
        native_copy_only,
        python_copy_live,
        native_copy_live,
        direct_copy_value,
        direct_copy_live,
        direct_copy_released,
        python_move_only,
        native_move_only,
        python_move_live,
        native_move_live,
        direct_move_value,
        direct_move_live,
        direct_move_released,
        python_throw,
        native_throw,
    )


result = compare_bound_result_categories()
assert result == (
    19,
    19,
    23,
    23,
    True,
    True,
    31,
    31,
    0,
    0,
    31,
    1,
    0,
    37,
    37,
    0,
    0,
    37,
    1,
    0,
    True,
    True,
), result
print("BIND_RESULT_CATEGORIES", *result)
