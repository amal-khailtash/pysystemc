"""Native member-access constructors with on-demand named member selection."""

import cppyy

from ._vectors import _named_member_policy, _named_member_token, _register_member_token

_native_member_template = cppyy.gbl.sc_core.sc_member_access


def _split_policy(native_type):
    """Separate the two outer template arguments while preserving nested types."""
    spelling = native_type.__cpp_name__[len("sc_core::sc_member_access<") : -1]
    depth = 0
    for index, char in enumerate(spelling):
        depth += (char == "<") - (char == ">")
        if char == "," and depth == 0:
            return spelling[:index], spelling[index + 1 :]
    raise TypeError("expected a native member policy specialization")


class _MemberPolicyType:
    """Construct native policy values and expose their original native aliases."""

    def __init__(self, native_type):
        """Retain the native specialization; no element ownership is acquired."""
        self._native_type = native_type
        self.__cpp_name__ = native_type.__cpp_name__
        self._element, self._member = _split_policy(native_type)
        self._element_type = native_type.element_type

    def __call__(self, source):
        """Construct from a public member name or copy/convert a native policy.

        Args:
            source: C++ member identifier or native member-access policy.

        Returns:
            Native policy borrowing selected element members.

        Raises:
            ValueError: Member name is not a C++ identifier.
            TypeError: Member or const conversion is invalid.

        GIL remains held. The policy owns no elements; retain every element while
        using get(). Native aliases and conversion restrictions are preserved.
        """
        if isinstance(source, str):
            return _named_member_policy(
                self._element_type,
                self._member,
                source,
                "sc_member_access[...]",
                "an accessible C++ data-member identifier or a compatible native member policy",
            )
        helper = cppyy.gbl.pysystemc_vectors.member_policy[self._element, self._member]
        if isinstance(source, self._native_type):
            return helper.copy(source)
        if not isinstance(source, self._native_type.non_const_policy):
            raise TypeError("source must use this member policy's non-const specialization")
        return helper.convert(source)

    def __getattr__(self, name):
        """Expose native aliases and members, adapting policy constructors only."""
        if name == "member_type":
            try:
                native_alias = getattr(self._native_type, name)
            except AttributeError:
                native_alias = None
            return _MemberPointerType(
                native_alias,
                cppyy.gbl.pysystemc_vectors.member_pointer[self._element, self._member],
                self._element_type,
                self._member,
            )
        attribute = getattr(self._native_type, name)
        if name in ("policy", "const_policy", "non_const_policy"):
            return _MemberPolicyType(attribute)
        return attribute


class _MemberPointerType:
    """Construct typed native pointer-to-member value tokens on demand."""

    def __init__(self, native_alias, token_type, element_type, member_type):
        """Retain the native alias and exact value-token specialization."""
        self._native_alias = native_alias
        self._token_type = token_type
        self._element_type = element_type
        self._member_type = member_type
        self.__cpp_name__ = token_type.__cpp_name__
        self.native_cpp_name = getattr(native_alias, "__cpp_name__", f"{member_type} {element_type.__cpp_name__}::*")
        self.__name__ = getattr(native_alias, "__name__", "member_type")
        _register_member_token(token_type, element_type, member_type)

    def __call__(self, *args):
        """Construct a null token or copy a token with this exact signature.

        Args:
            *args: No argument for a null value, or one same-signature token.

        Returns:
            A native value token storing the pointer-to-member by value.

        Raises:
            TypeError: The source is not a token with this exact signature.

        Tokens own no element or member storage. Calls hold the GIL and do not
        enter the SystemC kernel.
        """
        if not args:
            return self._token_type()
        if len(args) == 1 and isinstance(args[0], self._token_type):
            return self._token_type(args[0])
        raise TypeError("member_type accepts no arguments or a same-signature member token")

    def from_member(self, member):
        """Create a typed token for one public member identifier.

        Args:
            member: Bare C++ identifier, or this element's exact ``Type::name``.

        Returns:
            A native value token whose pointer type is exactly ``Member Element::*``.

        Raises:
            ValueError: The identifier is malformed or qualified for another type.
            TypeError: The member is missing, inaccessible, static, or incompatible.

        The token owns no vector or element. Keep vector elements alive while an
        assembly made from the token is in use.
        """
        if not isinstance(member, str):
            raise TypeError("member must be a public C++ data-member identifier")
        element_name = self._element_type.__cpp_name__
        if member.startswith(element_name + "::"):
            member = member[len(element_name) + 2 :]
        elif "::" in member:
            raise ValueError("member qualification must exactly match the token element type")
        return _named_member_token(
            self._element_type,
            self._member_type,
            member,
            "sc_member_access.member_type.from_member",
            "an accessible data-member identifier compatible with this member_type",
        )

    def __getattr__(self, name):
        """Expose metadata from the original native member-pointer alias."""
        if self._native_alias is None:
            raise AttributeError(name)
        return getattr(self._native_alias, name)


class MemberAccessTemplate:
    """Select arbitrary native element/member specializations on demand."""

    def __getitem__(self, parameters):
        """Return a native policy constructor for the requested template arguments."""
        return _MemberPolicyType(_native_member_template[parameters])

    def __getattr__(self, name):
        """Delegate native template metadata and explicit type discovery."""
        return getattr(_native_member_template, name)


def _pythonize_direct(native_type, name):
    """Restore a missing templated conversion constructor for direct policies."""
    if not name.startswith("sc_direct_access<"):
        return
    native_constructor = native_type.__init__
    element = native_type.__cpp_name__[len("sc_core::sc_direct_access<") : -1]

    def construct(self, *args):
        """Construct a native direct policy, including member-policy conversion."""
        try:
            native_constructor(self, *args)
        except TypeError:
            if len(args) != 1 or not getattr(type(args[0]), "__cpp_name__", None):
                raise
            source_type = type(args[0]).__cpp_name__
            value = cppyy.gbl.pysystemc_vectors.direct_policy[element].convert[source_type](args[0])
            native_constructor(self, value)

    native_type.__init__ = construct


def _pythonize_iterator(native_type, name):
    """Expose the native arrow pointer as an explicit cppyy-style operation."""
    if not name.startswith("sc_vector_iter<"):
        return

    def follow(self):
        """Return the borrowed native operator-> pointer for a valid iterator.

        Keep the vector/assembly and element alive while using this pointer.
        Calling on an end/invalid iterator has native undefined behavior. GIL
        remains held; no pointer ownership transfers.
        """
        return cppyy.gbl.pysystemc_vectors.iterator_pointer[type(self).__cpp_name__].get(self)

    native_type.__follow__ = follow


def install():
    """Install direct-policy pythonization before new specializations are reflected."""
    cppyy.py.add_pythonization(_pythonize_direct, "sc_core")
    cppyy.py.add_pythonization(_pythonize_iterator, "sc_core")
