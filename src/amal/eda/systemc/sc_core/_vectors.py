"""Preserve native vector templates while adapting creators and member access."""

import re

import cppyy

_members = {}
_native_assemble = cppyy.gbl.sc_core.sc_assemble_vector


def _member_helper(element_type, member):
    """Compile an on-demand native member-pointer adapter for one valid identifier."""
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", member):
        raise ValueError("vector member must be a C++ identifier")
    key = (element_type, member)
    if key not in _members:
        helper_name = f"member_{len(_members)}"
        cppyy.cppdef(f"""

namespace pysystemc_vectors
{{
/** @brief Borrow one native member assembly without transferring vector ownership. */
template <class Element>
struct {helper_name}
{{
  /** @brief Assemble using the native member function. GIL held; vector is borrowed. */
  static auto member(sc_core::sc_vector<Element>& vector)
  {{
    return vector.assemble(&Element::{member});
  }}
  /** @brief Construct a native member policy from the validated member identifier. */
  template <class Member>
  static sc_core::sc_member_access<Element, Member> policy()
  {{
    return sc_core::sc_member_access<Element, Member>(&Element::{member});
  }}
  /** @brief Assemble using the native free function. GIL held; vector is borrowed. */
  static auto free(sc_core::sc_vector<Element>& vector)
  {{
    return sc_core::sc_assemble_vector(vector, &Element::{member});
  }}
}};
}}  // namespace pysystemc_vectors
""")
        _members[key] = getattr(cppyy.gbl.pysystemc_vectors, helper_name)[element_type]
    return _members[key]


def _element_type(vector_type):
    """Return the unrestricted native element spelling from its vector specialization."""
    return vector_type.__cpp_name__[len("sc_core::sc_vector<") : -1]


def assemble_vector(vector, member):
    """Assemble a vector member using a native member pointer or member name.

    Args:
        vector: Native sc_vector specialization of any element type.
        member: Native data-member pointer, or public C++ member identifier.

    Returns:
        Native sc_vector_assembly borrowing the vector and its elements.

    Raises:
        ValueError: A member name is not a valid C++ identifier.
        TypeError: The member is absent, inaccessible or not assemblable.

    The returned proxy retains the vector, including Python assembly copies,
    assignment and swap. Iterators remain borrowed; keep their owners alive. Calls hold the GIL and preserve
    native binding, range and diagnostic behavior.
    """
    if isinstance(member, str):
        helper = _member_helper(_element_type(type(vector)), member)
        assembly = helper.free(vector)
    else:
        assembly = _native_assemble(vector, member)
    assembly._vector_owner = vector
    return assembly


def _pythonize_vector(vector_type, name):
    """Adapt reflected sc_vector specializations, preserving native templates."""
    if not name.startswith("sc_vector<"):
        return
    native_constructor = vector_type.__init__
    native_initialize = vector_type.init
    native_member = vector_type.assemble
    element_type = _element_type(vector_type)

    def initialize(self, size, creator=None):
        """Initialize owning native elements with an optional synchronous creator.

        Args:
            size: Native requested element count.
            creator: Optional native creator or Python callable taking name/index.

        Python creators must return newly allocated native elements of the vector
        type. Ownership transfers to the vector; do not delete them separately.
        Callback hosts remain retained by the vector proxy. Native errors and
        callback failures propagate, with native partial-construction rollback.
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

        def create(name, index):
            """Transfer one freshly created native element to the owning vector."""
            element = creator(name, index)
            if not isinstance(element, vector_type.element_type) or not element:
                raise TypeError("vector creator must return a newly allocated native element")
            element.__python_owns__ = False
            owners.append(element)
            return element

        try:
            return cppyy.gbl.pysystemc_vectors.creator[element_type].initialize(self, size, create)
        except BaseException:
            del owners[start:]
            raise

    def construct(self, *args):
        """Construct a native vector, including its templated creator overload."""
        if len(args) == 3:
            prefix, size, creator = args
            native_constructor(self, prefix)
            initialize(self, size, creator)
        else:
            native_constructor(self, *args)

    def assemble(self, member):
        """Borrow a member assembly using its native pointer or public member name."""
        if isinstance(member, str):
            assembly = _member_helper(element_type, member).member(self)
        else:
            assembly = native_member(self, member)
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
