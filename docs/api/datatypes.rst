Native datatypes and contexts
=============================

``sc_dt`` exposes native SystemC datatype templates and policy enums. Template
arguments are resolved on demand. Widths used in examples are test cases, not a
registry of supported widths; the native datatype's own limits still apply.
The ``sc_int``, ``sc_uint``, ``sc_bigint`` and ``sc_biguint`` template constructors
accept exact Python integers, preserving all bits before native width and sign
conversion. Explicit native scalar arguments retain their declared overloads.
Other numeric classes may require exact strings for values beyond native scalar
ranges.

Integer and vector owners
-------------------------

Integer templates copy native number, range, generic and fixed-point sources
into independently owned storage. Bounded integers retain their native 64-bit
width limit. Arbitrary-width integer templates instantiate on demand. Native
conversion reports, overflow and fixed-point rounding rules remain in effect.

``sc_int_base`` and ``sc_uint_base`` use native width constructors; a scalar
constructor argument specifies width rather than an initial numeric value.
``sc_signed`` and ``sc_unsigned`` also retain native dynamic-width construction.
Assignment from an ordinary Python integer uses exact decimal conversion before
native truncation. Deprecated ``to_signed()`` and ``to_unsigned()`` names retain
their native ``to_int()`` and ``to_uint()`` alias behavior.

``sc_bv`` and ``sc_lv`` constructors and assignments select exact native source
types for vector proxies and numeric values. Base vectors accept legacy numeric
constructor forms using the native source length and assignment implementation
when those declarations are disabled by the native build. Binary conversion of
X/Z values preserves native reports. Copying a vector owns independent storage;
a range or bit view continues to borrow its live parent.

Fixed-point numeric owners
--------------------------

``sc_fxval`` and ``sc_fxval_fast`` retain native arbitrary and double precision.
``sc_fixed``, ``sc_ufixed`` and their fast variants instantiate native width,
integer-width, quantization and overflow template parameters on demand. Typed
construction, assignment and arithmetic preserve the declared scalar source
type. Arithmetic results own independent storage; compound operations mutate
and return the original owner. Exact native ``__overload__`` selectors remain
available on both classes and bound methods.

``sc_fix``, ``sc_ufix`` and their fast variants configure those parameters at
construction. Native parameter-only forms and source-plus-parameter forms
remain distinct, including width/integer-width, quantization/overflow, overflow
bit count, ``sc_fxtype_params`` and cast switches. Observers are borrowed;
construction initializes the owner's storage directly and preserves observer
identity. Numeric Python integers beyond C++ ``int`` select signed or unsigned
64-bit arithmetic overloads; explicit native scalar types select their own
overloads. Integer assignment and fixed-point construction can also use exact
decimal strings for larger values before native conversion.

``sc_dt.sc_fxnum`` and ``sc_dt.sc_fxnum_fast`` expose owning native derived
facades because native base construction is protected. Construct with an optional
source followed by ``sc_fxtype_params``, ``sc_enc``, ``sc_fxcast_switch`` and an
optional observer pointer. The facade is a native base instance and stores its
value independently. The native private default and copy constructors remain
unavailable; use the source-plus-parameters constructor to copy a value.
These exports name the owning derived classes. The original base types remain
available as ``cppyy.gbl.sc_dt.sc_fxnum`` and ``sc_fxnum_fast``; use those base
types when checking membership of other native derived classes such as
``sc_fix`` or ``sc_fixed``.
Observers are borrowed and must remain live through number destruction.
``type_params()`` and ``cast_switch()`` return borrowed references to owner
storage. Native precision, reports and observer build settings apply.

``sc_dt.add(output, left, right)``, ``sub``, ``mult`` and ``div`` call the exact
native output-parameter arithmetic overload and return ``None``. Output owners
retain their identity, native precision and borrowed observers. Keep all
operands live for the synchronous call. An arbitrary-precision ``sc_fxnum``
output must be distinct from both input owners: native named arithmetic deletes
its output representation before reading input representations. Native
conversion, allocation and arithmetic reports remain in effect.

Shifts, streams and integer concatenation
-----------------------------------------

Native arbitrary-width integers, fixed-point values and integer concatenation
views support ``value << count`` and ``value >> count``. Count datatypes preserve
their native conversion rules; an ordinary Python integer uses C++ ``int``.
Results own independent storage and retain native result widths and signedness.
Native count preconditions and error reports apply, including the distinct
integer and fixed-point behavior for negative counts.

Character streams insert native datatype values with ``stream << value`` and
extract into mutable values or views with ``stream >> target``. Both operations
return the original Python stream for chaining. Native formatting flags,
conversion reports, EOF/failure state and exception masks remain in effect.
Keep each stream and target owner live; extraction through a view also requires
its parent. These synchronous operations hold the GIL. cppyy can expose native
I/O failures through the ``std::exception`` base with its generic diagnostic;
stream failure state and exception masks still follow native behavior.

``sc_dt.concat(left, right)`` selects a native ``sc_concatref`` for integer
operands. The shell borrows its operands and comes from the native concatenation
pool. Keep operands live throughout use. Pool reuse can retarget a
retained shell. ``value()`` and implicit unsigned-reference conversion additionally
borrow reusable unsigned shells and temporary numeric storage; copy the value
immediately when an independent snapshot is needed. Unary ``+``, ``-`` and ``~``
produce independent native values. Explicit scalar conversions follow native
width rules, including the low 64 bits for implicit integer conversion. The
legacy ``bitwidth()`` name delegates to native ``length()`` when disabled by the
native deprecated-API setting.

Legacy strings
--------------

``sc_string_old`` retains native copy-on-write storage, substring operations and
character mutation through ``text[index] = character``. Mutable indexing is
unchecked; callers must supply valid native positions. ``to_string(format, *args)``
and ``fmt(value)`` call the native printf-style formatter. Python ``int`` arguments
use C++ ``int``, ``float`` uses ``double``, and ``str`` supplies null-terminated
bytes; use explicit cppyy scalar types for other format specifiers. The format
must match the argument types, as required by native printf. Formatted results
and concatenations own their storage; ``fmt`` mutates and returns its owner.

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

Native vector expressions also accept borrowed ``bool*`` and ``sc_logic*``
arrays. Keep the array owner live and provide at least as many elements as the
vector length. cppyy bool pointer views and logic pointer proxies select the
corresponding native array overloads, including reflected operations. These
overloads retain native unchecked pointer-length preconditions. Native
``sc_proxy[OwnerType]`` expressions support arbitrary CRTP owners with valid
``sc_proxy_traits`` and the native bit/word interface.

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

``sc_dt.concat(left, right)`` also concatenates binary/four-state vectors, bits,
ranges and nested vector views. Mutable operands produce writable views when
the native overload allows it; scalar and read-only operands produce read-only
views. Python strings are copied into native vector storage and preserve X/Z
states. Boolean/vector-bit forms select the defined native binary-vector result.
Parent vectors remain borrowed even when the result owns cloned proxy shells.

Use ``sc_dt.concat[NativeTypes...](left, right)`` for explicit native template
arguments or ``sc_dt.concat.__overload__(signature)(left, right)`` when selecting
a particular const-qualified overload. Python comma expressions create tuples;
use ``sc_dt.concat`` for the corresponding native concatenation behavior.


Fixed-point policy boundaries
-----------------------------

Quantization, overflow and cast switches retain native SystemC semantics.
Native comparisons exercise all seven quantizers across signed/unsigned fixed
and fast families, using positive/negative half ties and adjacent values.
Saturation, zero saturation, symmetric saturation and wrap policies are checked
at both ends of the representable range, including lossy ``SC_ON`` versus
``SC_OFF`` assignments and the effects of ``n_bits`` on wrap results.

Unsigned fixed-point families reject ``SC_WRAP_SM`` with the native report;
this mode requires signed encoding. Unsigned ``SC_SAT`` and ``SC_SAT_SYM`` have
the same endpoint behavior. These numerical checks complement constructor and
arbitrary-width template audits; widths remain native template parameters.
