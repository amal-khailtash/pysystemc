"""Verify safe legacy string conversion recovery and copied ownership."""

import gc
import weakref

from amal.eda.systemc import sc_dt
import cppyy


STRING = sc_dt.sc_string_old

# An unsupported Python object must fail while cppyy selects the native
# template, before the owner's printf format is entered. The same owner remains
# unchanged and accepts its valid integer argument afterward.
formatted_owner = STRING("value=%04d")
try:
    formatted_owner.fmt(object())
except TypeError as error:
    assert "string_fmt<object>" in str(error)
else:
    raise AssertionError("sc_string_old.fmt accepted an unsupported argument")
assert str(formatted_owner) == "value=%04d"
fmt_result = formatted_owner.fmt(7)
assert cppyy.addressof(fmt_result) == cppyy.addressof(formatted_owner)
assert str(formatted_owner) == "value=0007"

# Python bytes are rejected before the native C varargs boundary. The
# incremental formatter preserves its original format and then accepts str.
try:
    STRING.to_string("%s", b"native")
except TypeError as error:
    assert "printf bytes arguments require str or native std::string" in str(error)
else:
    raise AssertionError("sc_string_old.to_string accepted a bytes argument")
bytes_fmt_owner = STRING("%s")
try:
    bytes_fmt_owner.fmt(b"native")
except TypeError as error:
    assert "printf bytes arguments require str or native std::string" in str(error)
else:
    raise AssertionError("sc_string_old.fmt accepted a bytes argument")
assert str(bytes_fmt_owner) == "%s"
bytes_fmt_owner.fmt("native")
assert str(bytes_fmt_owner) == "native"

# Invalid character conversion and index conversion are rejected before the
# native unchecked subscript is touched. A valid write then recovers normally.
edited = STRING("abc")
try:
    edited[1] = "XY"
except ValueError as error:
    assert "char expected" in str(error)
else:
    raise AssertionError("sc_string_old accepted a multi-character char value")
try:
    edited["one"] = "Z"
except TypeError as error:
    assert "integer object" in str(error)
else:
    raise AssertionError("sc_string_old accepted a non-integer index")
assert str(edited) == "abc"
edited[1] = "Z"
assert str(edited) == "aZc"

# The native std::string argument is borrowed for formatting only. The returned
# value owns its bytes and copy-on-write detaches later mutation from its copy.
native_argument = cppyy.gbl.std.string("native")
rendered = STRING.to_string("%s:%d", native_argument, 17)
del native_argument
gc.collect()
assert str(rendered) == "native:17"
rendered_copy = STRING(rendered)
rendered[0] = "N"
assert str(rendered) == "Native:17"
assert str(rendered_copy) == "native:17"

# Concatenation returns independent native storage. Releasing its source proxy
# does not invalidate the result.
source = STRING("suffix")
source_ref = weakref.ref(source)
concatenated = "pre-" + source
assert cppyy.addressof(concatenated) != cppyy.addressof(source)
del source
gc.collect()
assert source_ref() is None
assert str(concatenated) == "pre-suffix"

print("LEGACY_STRING_FMT_REJECTED_RECOVERED value=0007 alias=owner")
print("LEGACY_STRING_BYTES_REJECTED_RECOVERED native")
print("LEGACY_STRING_MUTATION_REJECTED_RECOVERED aZc")
print("LEGACY_STRING_FORMAT_COPY native:17 Native:17 native:17")
print("LEGACY_STRING_CONCAT_SOURCE_RELEASED pre-suffix")
