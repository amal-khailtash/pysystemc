"""Select exact native blocking output overloads across virtual TLM bases."""

import cppyy

from ..sc_core import _ProcessUnwind

cppyy.cppdef("""
namespace pysystemc_interfaces
{
/** @brief Invoke the exact native blocking peek output overload.
 * @tparam T Arbitrary native transaction type.
 * @param interface Borrowed interface retained through the operation.
 * @param output Caller-owned output retained through possible suspension.
 * @return False when Python must unwind before scheduler control is restored.
 * @details Release the GIL before entry; native virtual dispatch preserves
 * callback behavior. Allocation/conversion and callback errors propagate.
 */
template <class T>
struct exact_tlm_peek
{
  /** @brief Dispatch with borrowed storage and guarded native scheduler control. */
  static bool call(tlm::tlm_blocking_peek_if<T>& interface, T& output)
  {
    return sc_core::python_guarded_operation([&] { interface.peek(output); });
  }
};
}
""")


def _pythonize(interface_type, name):
    """Repair cppyy's output overload selection on native combined interfaces."""
    for base in interface_type.__mro__:
        cpp_name = getattr(base, "__cpp_name__", "")
        prefix = "tlm::tlm_blocking_peek_if<"
        if not cpp_name.startswith(prefix):
            continue
        transaction = cpp_name[len(prefix) : -1]
        native_peek = interface_type.peek
        operation = cppyy.gbl.pysystemc_interfaces.exact_tlm_peek[transaction].call
        operation.__release_gil__ = True

        def peek(owner, *args, _native=native_peek, _operation=operation, _type=transaction):
            """Peek into caller-owned output or use native value/tag dispatch.

            Output storage and interface remain borrowed through suspension.
            Reset/kill executes Python cleanup before native scheduler unwind.
            Native conversion and callback exceptions propagate.
            """
            if len(args) == 1 and getattr(type(args[0]), "__cpp_name__", "") == _type:
                if not _operation(owner, args[0]):
                    raise _ProcessUnwind()
                return None
            return _native(owner, *args)

        if hasattr(native_peek, "__overload__"):
            peek.__overload__ = native_peek.__overload__
        interface_type.peek = peek
        break


cppyy.py.add_pythonization(_pythonize)
