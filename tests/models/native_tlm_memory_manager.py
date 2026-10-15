"""Validate Python memory-manager callbacks and native payload reference counting."""

from amal.eda.systemc import tlm

import cppyy
import gc


class MemoryManager(tlm.tlm_mm_interface):
    """Retain recycled payload identities without deleting borrowed transactions."""

    def __init__(self):
        """Initialize the native interface and retained callback records."""
        super().__init__()
        self.freed = []

    def free(self, trans):
        """Record recycling at zero references; leave payload ownership with Python."""
        assert trans.get_ref_count() == 0
        self.freed.append((cppyy.addressof(trans), int(trans.get_address())))


manager = MemoryManager()
trans = tlm.tlm_generic_payload(manager)
address = cppyy.addressof(trans)
trans.set_address(17)
assert trans.has_mm()
assert trans.get_ref_count() == 0
trans.acquire()
trans.acquire()
gc.collect()
assert trans.get_ref_count() == 2
trans.release()
assert trans.get_ref_count() == 1
assert manager.freed == []
trans.release()
assert manager.freed == [(address, 17)]
trans.set_address(23)
trans.acquire()
trans.release()
assert manager.freed == [(address, 17), (address, 23)]
print("NATIVE_TLM_MEMORY_MANAGER_OK")
