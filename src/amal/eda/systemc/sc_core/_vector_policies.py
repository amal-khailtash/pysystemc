"""Native member-access constructors with on-demand named member selection."""

import cppyy

from ._vectors import _member_helper

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
            return _member_helper(self._element, source).policy[self._member]()
        helper = cppyy.gbl.pysystemc_vectors.member_policy[self._element, self._member]
        if isinstance(source, self._native_type):
            return helper.copy(source)
        return helper.convert(source)

    def __getattr__(self, name):
        """Expose native aliases and members, adapting policy constructors only."""
        attribute = getattr(self._native_type, name)
        if name in ("policy", "const_policy", "non_const_policy"):
            return _MemberPolicyType(attribute)
        return attribute


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
