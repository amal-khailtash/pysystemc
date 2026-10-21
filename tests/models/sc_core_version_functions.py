"""Verify native version function return values and copied static strings."""

from amal.eda.systemc import sc_core as sc

for name, constant in (("sc_copyright", "sc_copyright_string"), ("sc_release", "sc_version_string")):
    value = getattr(sc, name)()
    assert value == getattr(sc, constant)
    assert value == getattr(sc, name)()
    print("VERSION_FUNCTION_REFERENCE", name, value.encode().hex())
value = sc.sc_version()
assert value and sc.sc_release() in value
assert value == sc.sc_version()
print("VERSION_FUNCTION_REFERENCE", "sc_version", value.encode().hex())
