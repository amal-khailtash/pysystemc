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
av.clear_extension(identifier)
assert not av.get_extension(identifier)
assert cppyy.addressof(bv.get_extension(identifier)) == cppyy.addressof(y)
bv.clear_extension(identifier)
print("INSTANCE_EXTENSIONS_OK")
