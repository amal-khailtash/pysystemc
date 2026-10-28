/** @file tlm_endian_public_probe.h
 * @brief Arbitrary native word size for endian template instantiation.
 * @details This plain value owns its bytes and has no callbacks or GIL needs.
 */
#ifndef PYSYSTEMC_TLM_ENDIAN_PUBLIC_PROBE_H
#define PYSYSTEMC_TLM_ENDIAN_PUBLIC_PROBE_H
/** @brief A sixteen-byte word outside the fundamental integer type list. */
struct EndianWord16
{
  unsigned char bytes[16];
};
#endif
