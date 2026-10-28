"""Prepare native analysis FIFO writes for arbitrary cppyy payload types."""

import cppyy

cppyy.cppdef("""
namespace pysystemc_interfaces
{
/** @brief Reflect an arbitrary payload type without instantiating its FIFO. */
template <class T>
struct analysis_value_name
{
  using value_type = T;
};
}
""")


class _AnalysisFIFO:
    """Instantiate exact native analysis FIFOs with dependent-base lookup repaired.

    SystemC 2.3.4's two write definitions omit dependent-base qualification.
    On-demand native member specializations preserve the original class,
    constructors, virtual interfaces, FIFO behavior and arbitrary payload types.
    Use this template facade before instantiating the same native FIFO through
    cppyy directly; C++ prohibits specializing an already-instantiated method.
    """

    def __init__(self):
        """Retain prepared native type proxies without retaining FIFO objects."""
        self._types = {}

    def __getitem__(self, payload):
        """Return the exact native FIFO specialization for a payload type.

        Args:
            payload: Native cppyy type or C++ type name, with native FIFO value
                construction, copying, comparison and streaming requirements.
        Returns:
            Native ``tlm::tlm_analysis_fifo<payload>`` class proxy. Queued values
            are native copies; references to their application storage retain
            their payload type's ownership rules.
        Raises:
            TypeError: The native payload type cannot be resolved.
            RuntimeError: C++ rejects preparation, including methods already
                instantiated directly through cppyy before this facade.
        Preparation holds the GIL and does not enter simulation. Construct FIFO
        objects during native elaboration and retain them through simulation.
        """
        name = cppyy.gbl.pysystemc_interfaces.analysis_value_name[payload].__cpp_name__
        native = name.split("<", 1)[1][:-1].strip()
        if native in self._types:
            return self._types[native]
        try:
            cppyy.cppdef(f"""
/** @brief Queue a borrowed value using the native nonblocking FIFO operation.
 * @details The FIFO copies the value; no Python callback or GIL transition is
 * performed. Native allocation/copy exceptions propagate. Writes never wait.
 */
template <>
inline void tlm::tlm_analysis_fifo<{native}>::write(
  const typename pysystemc_interfaces::analysis_value_name<{native}>::value_type& value);
/** @brief Queue the transaction copied from a borrowed analysis triple. */
template <>
inline void tlm::tlm_analysis_fifo<{native}>::write(const tlm::tlm_analysis_triple<{native}>& value);
template <>
inline void tlm::tlm_analysis_fifo<{native}>::write(
  const typename pysystemc_interfaces::analysis_value_name<{native}>::value_type& value)
{{
  this->nb_put(value);
}}
template <>
inline void tlm::tlm_analysis_fifo<{native}>::write(const tlm::tlm_analysis_triple<{native}>& value)
{{
  this->nb_put(value.transaction);
}}
""")
        except SyntaxError as error:
            raise RuntimeError(
                f"Cannot prepare native tlm_analysis_fifo<{native}>; instantiate "
                "through amal.eda.systemc.tlm before using the raw cppyy template."
            ) from error
        result = cppyy.gbl.tlm.tlm_analysis_fifo[native]
        self._types[native] = result
        return result


tlm_analysis_fifo = _AnalysisFIFO()
