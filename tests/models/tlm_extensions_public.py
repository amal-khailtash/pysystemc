"""Exercise public TLM extension virtuals, IDs, managers and private accessors."""

from pathlib import Path

from amal.eda.systemc import tlm, tlm_utils

import cppyy


PROBE = Path(__file__).resolve().parents[1] / "native/tlm_extensions_probe.h"
cppyy.include(str(PROBE))
probe = cppyy.gbl.tlm_extensions_probe
wide_type = probe.WideValue
private_type = probe.PrivateWide
other_type = probe.PrivateOther


def address(value):
    """Return the native identity of an extension proxy."""
    return int(cppyy.addressof(value))


assert hasattr(tlm, "tlm_mm_interface")
assert hasattr(tlm, "tlm_extension_base")
assert hasattr(tlm, "tlm_extension")
assert hasattr(tlm_utils, "ispex_base")
assert hasattr(tlm_utils, "instance_specific_extension")
assert hasattr(tlm_utils, "instance_specific_extension_accessor")
assert hasattr(tlm_utils, "instance_specific_extensions_per_accessor")

# The public parameterized alias resolves an arbitrary native extension type.
typed_base = tlm.tlm_extension[wide_type]
extension_id = int(wide_type.ID)
assert int(typed_base.ID) == extension_id
assert extension_id == int(probe.WideValue.registered_id())
assert extension_id == int(probe.WideValue.register_again())
assert extension_id < int(tlm.max_num_extensions())
endian_context_type = cppyy.gbl.tlm.tlm_endian_context
endian_context_id = int(endian_context_type.ID)
carrier_type = tlm_utils.instance_specific_extension_carrier
carrier_id = int(carrier_type.ID)
assert endian_context_id < int(tlm.max_num_extensions())
assert carrier_id < int(tlm.max_num_extensions())

source = wide_type(7000)
source.__python_owns__ = False
payload = tlm.tlm_generic_payload()
payload.resize_extensions()
assert not payload.set_extension(source)
assert address(payload.get_extension(extension_id)) == address(source)
typed_payload_out = cppyy.bind_object(0, wide_type)
payload.get_extension[wide_type](typed_payload_out)
assert address(typed_payload_out) == address(source)

clone_base = source.clone()
clone = cppyy.bind_object(clone_base, wide_type)
clone_base.__python_owns__ = False
clone.__python_owns__ = False
assert clone.words[0] == source.words[0]
assert clone.words[136] == 7000 + 136 * 17
target = wide_type()
target.__python_owns__ = False
target.copy_from(clone)
assert target.words[136] == clone.words[136]
assert int(wide_type.cloned) == 1 and int(wide_type.copied) == 1
clone.free()
target.free()
assert int(wide_type.freed) == 2

# The base implementation deletes through its protected virtual destructor.
base_default_free = probe.BaseDefaultFree()
base_default_free.__python_owns__ = False
base_default_free.free()
assert int(probe.BaseDefaultFree.created) == 1
assert int(probe.BaseDefaultFree.destroyed) == 1

accessor_a = tlm_utils.instance_specific_extension_accessor()
accessor_b = tlm_utils.instance_specific_extension_accessor()
view_a = accessor_a(payload)
view_b = accessor_b(payload)
private_value = private_type(801)
other_value = other_type(902)
private_id = int(private_type.priv_id)
other_id = int(other_type.priv_id)
private_template = tlm_utils.instance_specific_extension[private_type]
assert int(private_template.priv_id) == private_id
assert private_id == int(probe.PrivateRegistry.register_again())
assert not view_a.set_extension(private_value)
assert not view_b.set_extension(other_id, other_value)
assert address(view_a.get_extension(private_id)) == address(private_value)
assert address(view_a.get_extension(private_type.priv_id)) == address(private_value)
assert address(view_b.get_extension(other_id)) == address(other_value)
assert not view_a.get_extension(other_id)
typed_out = cppyy.bind_object(0, private_type)
view_a.get_extension[private_type](typed_out)
assert address(typed_out) == address(private_value)
view_a.resize_extensions()
view_b.clear_extension(other_value)
view_a.clear_extension(private_id)
assert not view_a.get_extension(private_id)
assert address(payload.get_extension(extension_id)) == address(source)
payload.clear_extension(source)
source.free()
assert int(wide_type.freed) == 3
del typed_out, view_a, view_b, accessor_a, accessor_b

# Two concrete tlm_extension specializations intentionally clone to null.
endian_context = endian_context_type()
endian_context.copy_from(endian_context)
assert not endian_context.clone()
endian_context.free()
endian_context.__python_owns__ = False
carrier = carrier_type()
carrier.copy_from(carrier)
assert not carrier.clone()
carrier.free()

# The public nested typedef and constructor accept the native container pointer.
per_accessor_type = tlm_utils.instance_specific_extensions_per_accessor
container_type = per_accessor_type.container_type
null_container = cppyy.bind_object(0, container_type)
detached_view = per_accessor_type(null_container)
detached_view.resize_extensions()
assert hasattr(per_accessor_type, "container_type")


class PythonManager(tlm.tlm_mm_interface):
    """Record borrowed generic payloads returned at zero references."""

    def __init__(self):
        """Initialize the native virtual interface and callback records."""
        super().__init__()
        self.records = []

    def free(self, transaction):
        """Record callback values while retaining payload storage with Python."""
        self.records.append((int(transaction.get_ref_count()), int(transaction.get_address())))


manager = PythonManager()
managed = tlm.tlm_generic_payload(manager)
managed.set_address(0x123456789)
managed.acquire()
assert managed.get_ref_count() == 1
managed.release()
assert manager.records == [(0, 0x123456789)]
print("EXTENSION_IDS stable 137")
print("EXTENSION_LIFECYCLE 3 1 1 3")
print("BASE_EXTENSION_FREE 1 1")
print("EXTENSION_SPECIALIZATIONS nullclone pooled")
print("PRIVATE_ACCESS 801 902 isolated cleared")
print("MEMORY_MANAGER 1 0 4886718345")
print("TLM_EXTENSIONS_PUBLIC_OK")
