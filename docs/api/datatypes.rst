Native datatypes and contexts
=============================

``sc_dt`` exposes native SystemC datatype templates and policy enums. Template
arguments are resolved on demand. Widths used in examples are test cases, not a
registry of supported widths; the native datatype's own limits still apply.
For Python integers beyond native scalar ranges, use an exact string constructor
or native arithmetic to build large values. cppyy can otherwise select a
floating-point constructor and lose lower bits.

Scalar bits and logic
----------------------

``sc_dt.sc_bit`` represents a binary bit. ``sc_dt.sc_logic`` represents the four
native logic states, including X and Z. Bit construction, assignment and compound
operations select exact native bool, int and single-character overloads for
Python scalar arguments. Invalid integers or characters retain native reports
instead of silently converting to bool. Constructors, conversions and operators
retain native truth tables and error reports. Keep
four-state values as logic values when X or Z matters; conversion to a binary
value follows the native report policy.

Numeric representation, quantization, overflow, encoding and cast-switch enums
are native types. ``to_string(enum_value)`` selects the exact native enum
overload, preserving its spelling even when other enums share its numeric value.
Native stream-base queries reflect C++ formatting flags. Integral typedefs
preserve the platform's native widths and numeric conversion rules.

Parameter defaults and contexts
-------------------------------

``sc_fxtype_params``, ``sc_fxcast_switch`` and ``sc_length_param`` describe native
fixed-point type, casting and default-length settings. Contexts copy their input
parameters; changing the original object does not change the copied value.

``sc_dt.sc_context[ParameterType]`` owns a native context as a C++ subobject.
``sc_fxtype_context``, ``sc_fxcast_context`` and ``sc_length_context`` use this
owner for their corresponding parameter types. Native contexts prohibit direct
heap allocation; the owner preserves native construction and destruction while
allowing a Python object to control that subobject's lifetime.

``SC_NOW`` activates a context at construction. ``SC_LATER`` defers activation
until ``begin()``. ``end()`` restores the preceding default. Active context
destruction also restores the preceding default. End or destroy nested contexts
in reverse order, in the process where they were activated. ``value()`` and
``default_value()`` return borrowed native references; copy values that need to
outlive their owning context. Native duplicate-begin and inactive-end reports
remain in effect. Context storage does not provide host-thread synchronization.

Observers and generic value sources
------------------------------------

Fixed-point observer bases expose native construct, read, write and destruct
virtual callbacks and native default-observer factory pointers. Retain a concrete
observer throughout every call that uses it. Callback values are borrowed for
the duration of the call. Python overrides run with the GIL held and must not
suspend. Automatic notifications follow the native ``SC_ENABLE_OBSERVERS``
setting; exposing an observer does not enable that build setting.

``sc_generic_base[ValueType]`` is the native CRTP base for user-defined numeric
sources. Its mutable and const arrow operators return borrowed pointers to the
derived object. A correct derived type supplies ``to_uint64()``, ``to_int64()``,
``to_sc_unsigned(output)`` and ``to_sc_signed(output)``. The base does not own
separate storage or validate that the derived-type relationship is correct.
``sc_value_base`` retains its native virtual destruction contract.

Bit views and vector operations
-------------------------------

Integer bit-reference objects borrow their parent integer. Native indexing uses
reusable proxy pools: enough subsequent selections can retarget a retained proxy
shell to another bit. Use a native copy constructor, such as
``sc_dt.sc_signed_bitref(view)``, to preserve a selected coordinate across pool
reuse, and retain the parent integer. A copied view still reads and writes the
live parent; it is not a snapshot of the bit value. ``bool(view)`` and
``not view`` follow the current native bit value. A const selection can appear
as a mutable derived proxy class in cppyy; preserve the native const-use contract.

Fixed-point ``number.bit(index)`` selects the mutable native overload and retains
its parent number. Assigning through this view changes the same storage observed
by ``number[index]``. Native copies of a view borrow the parent and require their
own parent lifetime management. Indices are storage-bit indices; the fractional
word length determines the represented exponent. Native bounds reports apply.
Fast fixed-point numbers preserve native double precision, even when their
configured word length exceeds the double significand.

Binary and four-state vector bit views also expose native assignment, compound,
clone, scan and word-access operations. Python truth conversion uses native
``to_bool()`` and retains native X/Z reports. Scalar assignment selects exact
bool, int and char overloads, preserving native invalid-value reports.

Native vector rotations, reversals, shifts, reductions and equality operations
preserve four-state data. Binary ``&``, ``|`` and ``^`` select exact C++ expressions
for native vectors and supported native scalar or bit-string operands. Compound
operators mutate and return the same owner. Non-mutating expressions return
independent native values, with native width rules. Complementing an arbitrary
unsigned integer also preserves the native result-width rule, which can add one
bit; the input's configured width is not forced onto the result.

Free ``b_and``, ``b_or``, ``b_xor`` and ``neg`` overloads support native scalar and
fixed-point types, including mixed fast/arbitrary fixed-point operands and
caller-owned output storage. Native quantization, overflow and report policies
continue to apply. Build exact large integer fixtures from strings or native
arithmetic: cppyy scalar constructor selection can narrow values even for a
64-bit bounded integer.

Range and concatenation views
-----------------------------

Integer ``range(high, low)`` views preserve native range direction, conversion,
assignment and concatenation behavior. Assignment from native generic sources
uses the exact C++ source type. Integer range shells use the same native pooled
lifetime model as integer bit shells: copy a view to preserve coordinates across
pool reuse, and retain its parent integer.

Fixed-point ``range(high, low)`` and ``number(high, low)`` return mutable views
from mutable numbers. Calling either form without indices selects the whole
word. The returned view retains its parent; native copies still require the
caller to retain that parent. ``sc_bv_base(view)`` and ``sc_bv[Width](view)`` use
the native conversion operator and own independent copied vector storage.
``to_signed()`` and ``to_unsigned()`` preserve their native deprecated alias
semantics through ``to_int()`` and ``to_uint()``, including builds where the
native deprecated names are disabled. Bounded integer bit and range views also
preserve ``bitwidth()`` as the native alias for ``length()``.

Vector subreferences and concatenation references borrow their operands. Keep
parent vectors and nested operand views alive for every operation. Native
``clone()`` allocates a proxy shell for the caller to release; the clone continues
to borrow its parent. Use vector value construction for an independent snapshot.
Subreference template arguments require native ``sc_proxy_traits`` support:
normal ``sc_bv``/``sc_lv`` range selection uses ``sc_bv_base``/``sc_lv_base``
parents, including vectors of arbitrary width.
