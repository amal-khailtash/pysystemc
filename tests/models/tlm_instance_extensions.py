"""Verify native instance-specific extensions isolate accessors and transactions."""

from amal.eda.systemc import tlm, tlm_utils
import cppyy

cppyy.cppdef("""
/** @brief Borrowed private extension; Python owns storage until explicitly cleared. */
struct PrivateValue : tlm_utils::instance_specific_extension<PrivateValue> { int value = 0; };
""")
carrier_id = tlm_utils.instance_specific_extension_carrier.ID
first = tlm.tlm_generic_payload()
second = tlm.tlm_generic_payload()
first.resize_extensions()
second.resize_extensions()
a = tlm_utils.instance_specific_extension_accessor()
b = tlm_utils.instance_specific_extension_accessor()
x = cppyy.gbl.PrivateValue()
y = cppyy.gbl.PrivateValue()
x.value = 17
y.value = 23
av = a(first)
bv = b(first)
other = a(second)
identifier = cppyy.gbl.PrivateValue.priv_id
assert not av.set_extension(x)
assert not bv.set_extension(y)
assert cppyy.addressof(av.get_extension(identifier)) == cppyy.addressof(x)
assert cppyy.addressof(bv.get_extension(identifier)) == cppyy.addressof(y)
assert not other.get_extension(identifier)
assert cppyy.bind_object(av.get_extension(identifier), cppyy.gbl.PrivateValue).value == 17
copied = tlm.tlm_generic_payload()
copied.resize_extensions()
first.set_address(73)
copied.deep_copy_from(first)
assert copied.get_address() == 73
assert not copied.get_extension(carrier_id)
copy_access = a(copied)
assert not copy_access.get_extension(identifier)
copy_value = cppyy.gbl.PrivateValue()
copy_value.value = 41
assert not copy_access.set_extension(copy_value)
assert cppyy.addressof(av.get_extension(identifier)) == cppyy.addressof(x)
assert cppyy.addressof(copy_access.get_extension(identifier)) == cppyy.addressof(copy_value)
copy_access.clear_extension(identifier)
assert not copied.get_extension(carrier_id)
del copy_access
replacement = cppyy.gbl.PrivateValue()
replacement.value = 31
previous = av.set_extension(replacement)
assert cppyy.addressof(previous) == cppyy.addressof(x)
assert x.value == 17
assert cppyy.addressof(av.get_extension(identifier)) == cppyy.addressof(replacement)
assert cppyy.addressof(bv.get_extension(identifier)) == cppyy.addressof(y)
av.clear_extension(identifier)
assert not av.get_extension(identifier)
assert cppyy.addressof(bv.get_extension(identifier)) == cppyy.addressof(y)
bv.clear_extension(identifier)
assert not first.get_extension(carrier_id)
# Per-accessor references are invalid after the final clear releases their container.
del av, bv
fresh = a(first)
assert not fresh.get_extension(identifier)
assert not fresh.set_extension(x)
assert cppyy.addressof(fresh.get_extension(identifier)) == cppyy.addressof(x)
fresh.clear_extension(identifier)
assert not first.get_extension(carrier_id)
del fresh
print("PRIVATE_EXTENSIONS 17 23 31 41 73 detached")
print("INSTANCE_EXTENSIONS_OK")
