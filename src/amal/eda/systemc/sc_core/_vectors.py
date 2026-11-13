"""Preserve native vector templates while adapting creators and member access."""

import re

import cppyy

_members = {}
_member_failures = {}
_member_tokens = {}
_pointer_getters = {}
_next_member_helper_id = 0


def _pointer_member_token(element_type, member_type, symbol):
    """Capture a native pointer variable by symbol without marshalling its bytes.

    Args:
        element_type: Native element class of the requested pointer signature.
        member_type: Native member type, including arbitrary template arguments.
        symbol: C++ variable identifier, optionally qualified by namespaces.

    Returns:
        An independent native token copying the variable's current pointer value.

    Raises:
        ValueError: The symbol is not a qualified C++ identifier.
        TypeError: The variable is missing, inaccessible or not a compatible
            pointer to a data member; the compiler error is retained as cause.

    Only generated callables are cached. Pointer values are sampled on every
    call. Tokens own no element storage; callers retain assembly owners. Calls
    hold the GIL, invoke no Python callbacks and do not enter the kernel.
    """
    if not isinstance(symbol, str):
        raise TypeError("pointer symbol must be a C++ variable identifier")
    if not re.fullmatch(r"(?:::)?[A-Za-z_][A-Za-z_0-9]*(?:::[A-Za-z_][A-Za-z_0-9]*)*", symbol):
        raise ValueError("pointer symbol must be a qualified C++ variable identifier")
    symbol = symbol.removeprefix("::")
    key = (element_type.__cpp_name__, str(getattr(member_type, "__cpp_name__", member_type)), symbol)
    if key not in _pointer_getters:
        global _next_member_helper_id
        identifier = f"pointer_symbol_{_next_member_helper_id}"
        _next_member_helper_id += 1
        try:
            cppyy.cppdef(f"""
namespace pysystemc_vectors {{
/** @brief Read one native data-member pointer variable without raw marshalling.
 * @return A fresh token for the symbol's current value, including null.
 * @details Calls hold the GIL and own only the copied pointer value. No Python
 * callbacks or kernel entry occur. Elements remain borrowed by later assemblies.
 * The native capture parameter checks the exact or convertible pointer signature.
 */
member_pointer<{key[0]}, {key[1]}> {identifier}() {{
  return member_pointer_access<{key[0]}, {key[1]}>::capture(::{symbol});
}}
}}
""")
            _pointer_getters[key] = getattr(cppyy.gbl.pysystemc_vectors, identifier)
        except (SyntaxError, TypeError, ValueError) as error:
            raise TypeError(f"native pointer symbol {symbol!r} is unavailable or incompatible") from error
    try:
        token = _pointer_getters[key]()
    except (SyntaxError, TypeError, ValueError) as error:
        raise TypeError(f"native pointer symbol {symbol!r} is unavailable or incompatible") from error
    _register_member_token(type(token), element_type, member_type)
    return token


def _member_helper(element_type, member):
    """Compile an on-demand native member-pointer adapter for one valid identifier."""
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", member):
        raise ValueError("vector member must be a C++ identifier")
    element_name = getattr(element_type, "__cpp_name__", element_type)
    key = (element_name, member)
    if key in _member_failures:
        try:
            getattr(element_type, member)
        except AttributeError:
            raise
        raise TypeError(f"native name {member!r} on {element_name} is not an accessible non-static data member")
    if key not in _members:
        global _next_member_helper_id
        helper_index = _next_member_helper_id
        _next_member_helper_id += 1
        helper_name = f"member_{helper_index}"
        metadata_name = f"member_metadata_{helper_index}"
        cppyy.cppdef(f"""

#include <type_traits>
#include <utility>

namespace pysystemc_vectors
{{
/** @brief Detect whether a public name selects an accessible data member.
 * @tparam Element Native vector element type, including arbitrary templates.
 * @tparam Member Native value type requested by sc_member_access.
 */
template <class Element, class = void>
struct {metadata_name} : std::false_type
{{
}};

template <class Element>
struct {metadata_name}<Element, std::void_t<decltype(&Element::{member}),
                                        decltype(std::declval<Element&>().{member})>>
  : std::bool_constant<
      std::is_member_object_pointer_v<decltype(&Element::{member})> &&
      std::is_convertible_v<decltype(&Element::{member}),
                        std::remove_reference_t<decltype(std::declval<Element&>().{member})> Element::*>>
{{
  template <class Member>
  static constexpr bool policy_compatible =
    std::is_convertible_v<decltype(&Element::{member}), Member Element::*>;
}};

/** @brief Borrow one native member assembly without transferring vector ownership.
 * @tparam Element Native vector element type.
 */
template <class Element>
struct {helper_name}
{{
  /** @brief Assemble using the native member pointer; vector ownership is borrowed.
   * @param vector Native vector whose members form the returned assembly.
   * @return Native assembly retaining no ownership of the vector or elements.
   * @throws sc_core::sc_report Native reports from SystemC pass through unchanged.
   * @note Called with the GIL held; no Python callbacks or GIL transitions occur.
   */
  static auto member(sc_core::sc_vector<Element>& vector)
  {{
    using value_type = std::remove_reference_t<decltype(std::declval<Element&>().{member})>;
    return vector.assemble(static_cast<value_type Element::*>(&Element::{member}));
  }}

  /** @brief Check whether a requested value type accepts this native member pointer.
   * @tparam Member Value type requested by the native member policy.
   */
  template <class Member>
  static constexpr bool policy_compatible()
  {{
    return {metadata_name}<Element>::template policy_compatible<Member>;
  }}

  /** @brief Construct a native member policy from the validated public identifier.
   * @tparam Member Value type requested by the native member policy.
   * @return A policy borrowing the selected member.
   */
  template <class Member>
  static sc_core::sc_member_access<Element, Member> policy()
  {{
    using value_type = std::remove_reference_t<decltype(std::declval<Element&>().{member})>;
    return sc_core::sc_member_access<Element, Member>(
      static_cast<value_type Element::*>(&Element::{member}));
  }}

  /** @brief Create a token for this exact native pointer-to-member value.
   * @tparam Member Value type requested by the public member_type alias.
   * @return A typed value containing the compile-time pointer-to-member.
   * @note The token owns no element, vector or member storage; GIL remains held.
   */
  template <class Member>
  static auto token()
  {{
    using token_factory = member_pointer_factory<
      Element, Member, static_cast<Member Element::*>(&Element::{member})>;
    return token_factory::create();
  }}

  /** @brief Assemble through the native free function; vector ownership is borrowed.
   * @param vector Native vector whose members form the returned assembly.
   * @return Native assembly retaining no ownership of the vector or elements.
   * @throws sc_core::sc_report Native reports from SystemC pass through unchanged.
   * @note Called with the GIL held; no Python callbacks or GIL transitions occur.
   */
  static auto free(sc_core::sc_vector<Element>& vector)
  {{
    using value_type = std::remove_reference_t<decltype(std::declval<Element&>().{member})>;
    return sc_core::sc_assemble_vector(
      vector, static_cast<value_type Element::*>(&Element::{member}));
  }}
}};
}}  // namespace pysystemc_vectors
""")
        metadata = getattr(cppyy.gbl.pysystemc_vectors, metadata_name)[element_type]
        if not bool(metadata.value):
            # A failed class-level lookup supplies the native AttributeError as
            # the cause without instantiating an invalid member expression.
            try:
                getattr(element_type, member)
            except AttributeError:
                _member_failures[key] = "inaccessible"
                raise
            _member_failures[key] = "unsupported"
            raise TypeError(f"native name {member!r} on {element_name} is not an accessible non-static data member")
        helper = getattr(cppyy.gbl.pysystemc_vectors, helper_name)[element_type]
        _members[key] = helper
    return _members[key]


def _named_member_error(operation, member, accepted, exc):
    """Raise a public named-member TypeError while retaining native detail in its cause."""
    raise TypeError(f"{operation} cannot use member {member!r}; {accepted}") from exc


def _named_assembly(vector, element_type, member, helper_method, operation, accepted):
    """Select a named member then perform one callback-free native assembly call."""
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", member):
        raise ValueError(f"{operation} member {member!r} must be a public C++ identifier; {accepted}")
    try:
        helper = _member_helper(element_type, member)
        native_operation = getattr(helper, helper_method)
    except (AttributeError, TypeError) as exc:
        _named_member_error(operation, member, accepted, exc)
    try:
        # This helper only calls sc_vector::assemble/sc_assemble_vector. It cannot
        # invoke Python callbacks or execute SystemC report actions.
        return native_operation(vector)
    except TypeError as exc:
        _named_member_error(operation, member, accepted, exc)


def _named_member_policy(element_type, member_type, member, operation, accepted):
    """Select and construct a named policy without hiding native overload detail."""
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", member):
        raise ValueError(f"{operation} member {member!r} must be a public C++ identifier; {accepted}")
    try:
        helper = _member_helper(element_type, member)
        compatible = helper.policy_compatible[member_type]()
    except (AttributeError, TypeError) as exc:
        _named_member_error(operation, member, accepted, exc)
    if not compatible:
        try:
            # Native overload selection reports candidate types without entering
            # the generated member-pointer adapter or invoking user/SystemC code.
            native_policy = cppyy.gbl.sc_core.sc_member_access[element_type, member_type]
            return native_policy(member)
        except TypeError as exc:
            _named_member_error(operation, member, accepted, exc)
    try:
        # Policy construction is a synchronous native value conversion and does
        # not call Python or execute SystemC report actions.
        return helper.policy[member_type]()
    except TypeError as exc:
        _named_member_error(operation, member, accepted, exc)


def _named_member_token(element_type, member_type, member, operation, accepted):
    """Build and register a typed value token for one checked public member name."""
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", member):
        raise ValueError(f"{operation} member {member!r} must be a public C++ identifier; {accepted}")
    try:
        helper = _member_helper(element_type, member)
        if not helper.policy_compatible[member_type]():
            raise TypeError("the requested member value type is incompatible")
        token = helper.token[member_type]()
    except (AttributeError, TypeError) as exc:
        _named_member_error(operation, member, accepted, exc)
    _register_member_token(type(token), element_type, member_type)
    return token


def _register_member_token(token_type, element_type, member_type):
    """Register one native token specialization for checked public routing."""
    _member_tokens[token_type.__cpp_name__] = (token_type, element_type, member_type)


def _assemble_member_token(vector, token, helper_method, operation):
    """Validate a registered token signature then call its native assembly adapter."""
    token_name = getattr(type(token), "__cpp_name__", None)
    registration = _member_tokens.get(token_name)
    if registration is None:
        return None
    token_type, element_type, member_type = registration
    if not isinstance(token, token_type):
        raise TypeError(f"{operation} requires the registered native member token type")
    actual_element = type(vector).element_type
    if actual_element.__cpp_name__ != element_type.__cpp_name__:
        raise TypeError(
            f"{operation} member token uses element {element_type.__cpp_name__}, "
            f"but vector uses {actual_element.__cpp_name__}"
        )
    if not token.valid():
        raise ValueError(f"{operation} member pointer token is null")
    access = cppyy.gbl.pysystemc_vectors.member_pointer_access[element_type, member_type]
    return getattr(access, helper_method)(vector, token)


def _element_type(vector_type):
    """Return the unrestricted native element spelling from its vector specialization."""
    return vector_type.__cpp_name__[len("sc_core::sc_vector<") : -1]


def assemble_vector(vector, member):
    """Assemble a vector member using a typed member token or member name.

    Args:
        vector: Native sc_vector specialization of any element type.
        member: Typed token from sc_member_access[Element, Member].member_type,
            or a public C++ member identifier.

    Returns:
        Native sc_vector_assembly borrowing the vector and its elements.

    Raises:
        ValueError: A member name is invalid or a typed member token is null.
        TypeError: The member is absent, inaccessible or not assemblable; the native
            selection error is retained as the exception cause.

    The returned proxy retains the vector, including Python assembly copies,
    assignment and swap. Iterators remain borrowed; keep their owners alive. Calls hold the GIL and preserve
    native binding, range and diagnostic behavior.
    Capture native pointer variables using member_type.from_pointer(symbol).
    Untyped raw cppyy views are rejected before native dispatch.
    """
    if isinstance(member, str):
        vector_element_type = type(vector).element_type
        assembly = _named_assembly(
            vector,
            vector_element_type,
            member,
            "free",
            "sc_assemble_vector",
            "an accessible C++ data-member identifier or a native member pointer",
        )
    else:
        assembly = _assemble_member_token(vector, member, "free", "sc_assemble_vector")
        if assembly is None:
            raise TypeError(
                "sc_assemble_vector requires a member name or typed token; capture native pointer variables with member_type.from_pointer(symbol)"
            )
    assembly._vector_owner = vector
    return assembly


def _pythonize_vector(vector_type, name):
    """Adapt reflected sc_vector specializations, preserving native templates."""
    if not name.startswith("sc_vector<"):
        return
    native_constructor = vector_type.__init__
    native_initialize = vector_type.init
    element_type = _element_type(vector_type)
    element_type_proxy = vector_type.element_type

    def initialize(self, size, creator=None):
        """Initialize owning native elements with an optional synchronous creator.

        Args:
            size: Native requested element count.
            creator: Optional native creator or Python callable taking name/index.

        Python creators must return newly allocated native elements of the vector
        type. Ownership transfers to the vector; do not delete them separately.
        Callback hosts remain retained by the vector proxy. Native errors propagate;
        Python creator exceptions preserve their identity after native rollback.
        Calls hold the GIL and must satisfy SystemC elaboration constraints.
        """
        if creator is None:
            return native_initialize(self, size)
        creator_type = getattr(creator, "__cpp_name__", None) or getattr(type(creator), "__cpp_name__", None)
        if creator_type:
            return cppyy.gbl.pysystemc_vectors.native_creator[element_type, creator_type].initialize(
                self, size, creator
            )
        if not callable(creator):
            raise TypeError("vector creator must be callable")
        owners = self.__dict__.setdefault("_vector_element_owners", [])
        start = len(owners)
        callback_exception = None

        def create(name, index):
            """Transfer one freshly created native element to the owning vector."""
            nonlocal callback_exception
            try:
                element = creator(name, index)
            except BaseException as exc:
                callback_exception = exc
                raise
            if not isinstance(element, vector_type.element_type) or not element:
                raise TypeError("vector creator must return a newly allocated native element")
            element.__python_owns__ = False
            owners.append(element)
            return element

        try:
            result = cppyy.gbl.pysystemc_vectors.creator[element_type].initialize(self, size, create)
        except BaseException as native_error:
            del owners[start:]
            if (
                callback_exception is None
                or type(native_error) is not type(callback_exception)
                or str(callback_exception) not in str(native_error)
            ):
                raise
            original_callback_exception = callback_exception
            callback_exception = None
        else:
            return result
        raise original_callback_exception

    def construct(self, *args):
        """Construct a native vector, including its templated creator overload."""
        if len(args) == 3:
            prefix, size, creator = args
            native_constructor(self, prefix)
            initialize(self, size, creator)
        else:
            native_constructor(self, *args)

    def assemble(self, member):
        """Borrow a member assembly using a typed token or public member name.

        Args:
            member: Token from sc_member_access[Element, Member].member_type,
                or an accessible C++ data-member identifier.

        Returns:
            Native assembly retaining this vector; its iterators remain borrowed.

        Raises:
            ValueError: The member name is invalid or the token is null.
            TypeError: The token signature or member selection is incompatible.

        Calls hold the GIL. Native pointer variables can be captured with
        member_type.from_pointer(symbol). Untyped raw cppyy views are rejected.
        Keep assembly owners alive while using their iterators.
        """
        if isinstance(member, str):
            assembly = _named_assembly(
                self,
                element_type_proxy,
                member,
                "member",
                "sc_vector.assemble",
                "an accessible C++ data-member identifier or a native member pointer",
            )
        else:
            assembly = _assemble_member_token(self, member, "member", "sc_vector.assemble")
            if assembly is None:
                raise TypeError(
                    "sc_vector.assemble requires a member name or typed token; capture native pointer variables with member_type.from_pointer(symbol)"
                )
        assembly._vector_owner = self
        return assembly

    vector_type.__init__ = construct
    vector_type.init = initialize
    vector_type.assemble = assemble


def _pythonize_assembly(assembly_type, name):
    """Keep known vector owners synchronized with native assembly copy and swap."""
    if not name.startswith("sc_vector_assembly<"):
        return
    native_constructor = assembly_type.__init__
    native_assign = assembly_type.__assign__
    native_swap = assembly_type.swap

    def construct(self, *args):
        """Copy a borrowed assembly and retain its known Python vector owner."""
        native_constructor(self, *args)
        if len(args) == 1 and "_vector_owner" in args[0].__dict__:
            self._vector_owner = args[0]._vector_owner

    def assign(self, source):
        """Apply native copy assignment and retain the source's known vector owner."""
        source_owner = source.__dict__.get("_vector_owner")
        result = native_assign(self, source)
        self.__dict__.pop("_vector_owner", None)
        if source_owner is not None:
            self._vector_owner = source_owner
        return result

    def swap(self, other):
        """Swap native assembly state and the corresponding known vector owners."""
        own_owner = self.__dict__.get("_vector_owner")
        other_owner = other.__dict__.get("_vector_owner")
        native_swap(self, other)
        self.__dict__.pop("_vector_owner", None)
        other.__dict__.pop("_vector_owner", None)
        if other_owner is not None:
            self._vector_owner = other_owner
        if own_owner is not None:
            other._vector_owner = own_owner

    assembly_type.__init__ = construct
    assembly_type.__assign__ = assign
    assembly_type.swap = swap


def install():
    """Install on-demand vector pythonization before any specializations are used."""
    cppyy.py.add_pythonization(_pythonize_vector, "sc_core")
    cppyy.py.add_pythonization(_pythonize_assembly, "sc_core")
