"""Preserve full-width addresses around native generic endian conversion."""

import cppyy

cppyy.cppdef("""

namespace pysystemc_interfaces
{
/** @brief Preserve 64-bit addresses while using the native generic converter.
 * @tparam Word Arbitrary native data word type with native converter constraints.
 * @details SystemC 2.3.4 forms its generic address mask in unsigned-int width.
 * Convert a bus-relative address, then translate the payload and native context
 * back to the original address window. Buffer conversion and response dispatch
 * remain native. Payload and original buffers are borrowed through both halves.
 * Calls hold the GIL, do not suspend, and propagate native allocation errors.
 */
template <class Word>
struct generic_endian_request
{
  /** @brief Convert a borrowed payload, preserving its full-width address window. */
  static void call(tlm::tlm_generic_payload* payload, unsigned int bus)
  {
    const sc_dt::uint64 address = payload->get_address();
    const sc_dt::uint64 mask = sc_dt::uint64(bus) - 1;
    payload->set_address(address & mask);
    try
    {
      tlm::tlm_to_hostendian_generic<Word>(payload, bus);
    }
    catch (...)
    {
      payload->set_address(address);
      throw;
    }
    const sc_dt::uint64 anchor = address & ~mask;
    auto* context = payload->get_extension<tlm::tlm_endian_context>();
    context->address = address;
    context->new_address = anchor;
    payload->set_address(anchor);
  }
};
}  // namespace pysystemc_interfaces
""")


class _GenericEndianRequest:
    """Instantiate native request adapters without restricting word types."""

    def __getitem__(self, word):
        """Return the converter for a native word type or cppyy type name.

        The returned call accepts a borrowed payload and bus width in bytes.
        Native power-of-two, streaming and buffer-lifetime constraints apply.
        """
        return cppyy.gbl.pysystemc_interfaces.generic_endian_request[word].call


tlm_to_hostendian_generic = _GenericEndianRequest()
