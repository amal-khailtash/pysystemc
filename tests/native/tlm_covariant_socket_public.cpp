/** @file tlm_covariant_socket_public.cpp
 * @brief Independent native reference using a consistently enabled library.
 */
#include <cassert>
#include <iostream>

#include "tlm_covariant_socket_probe.h"
/** @brief Verify borrowed identities before and after native elaboration. */
int sc_main(int, char**)
{
  {
    TlmCovariantOwner owner("owner");
    assert(TlmCovariantOwner::live_count == 1);
    assert(owner.identities());
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    assert(owner.identities());
  }
  assert(TlmCovariantOwner::live_count == 0);
  std::cout << "TLM_COVARIANT_NATIVE_OK\n";
  return 0;
}
