"""Adapt native pointer-vector slots without restricting pointer types."""

from operator import index as integer_index

import cppyy


def _wrap(native, name=""):
    """Adapt arbitrary pointer-vector classes as cppyy reflects them."""
    spelling = getattr(native, "__cpp_name__", "")
    if not spelling.startswith("sc_core::sc_pvector<"):
        return
    item = spelling[len("sc_core::sc_pvector<") : -1].strip()
    if not item.endswith("*") or native.__dict__.get("_pointer_slots_wrapped", False):
        return
    access = cppyy.gbl.pysystemc_public.pvector_access[item]

    def get(values, index):
        """Return a borrowed pointee, growing slots with native null initialization.

        Args:
            values: Owning native pointer vector.
            index: Nonnegative integer slot index.

        Returns:
            Borrowed native pointer; newly grown slots hold native null pointers.

        Raises:
            IndexError: Index is negative.
            TypeError: Index is not an integer.

        GIL stays held. Growth invalidates slot/iterator views; retain pointees.
        """
        index = integer_index(index)
        if index < 0:
            raise IndexError("native pointer-vector index must be nonnegative")
        return access.get(values, index)

    def set_slot(values, index, value):
        """Assign a borrowed pointer to an auto-growing native slot.

        Args:
            values: Owning native pointer vector.
            index: Nonnegative integer slot index.
            value: Borrowed correctly typed pointee or native null pointer.

        Returns:
            None after native assignment; the vector does not own the pointee.

        Raises:
            IndexError: Index is negative.
            TypeError: Index or pointer conversion is invalid.

        GIL stays held; retain pointees and discard old slot views after growth.
        """
        index = integer_index(index)
        if index < 0:
            raise IndexError("native pointer-vector index must be nonnegative")
        access.set(values, index, value)

    def fetch(values, index):
        """Return a borrowed pointee from an existing slot without growing.

        Args:
            values: Owning native pointer vector.
            index: Integer in the existing native slot range.

        Returns:
            Borrowed correctly typed pointer, including native null pointers.

        Raises:
            IndexError: Index is outside the vector.
            TypeError: Index is not an integer.

        GIL stays held; the caller retains the pointee through its use.
        """
        index = integer_index(index)
        if not 0 <= index < values.size():
            raise IndexError("native pointer-vector fetch index is out of range")
        return access.fetch(values, index)

    native.__getitem__ = get
    native.__setitem__ = set_slot
    native.fetch = fetch
    native._pointer_slots_wrapped = True


def install():
    """Install slot adaptation for current and future native pointer vectors."""
    for candidate in tuple(vars(cppyy.gbl.sc_core).values()):
        if isinstance(candidate, type):
            _wrap(candidate)
    cppyy.py.add_pythonization(_wrap)
