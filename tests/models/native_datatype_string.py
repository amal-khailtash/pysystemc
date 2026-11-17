"""Compare Python datatype strings with independent native stream insertion."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "native_datatype_string_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.native_datatype_string_probe


def check_string(label, value, replacement):
    """Compare Python text against native insertion before and after mutation.

    Args:
        label: Case name used in the success record.
        value: Live SystemC datatype to format and mutate.
        replacement: Defined finite value assigned after the initial checks.
    """
    address = int(cppyy.addressof(value))
    expected = str(probe.Reference[type(value)].text(value))
    assert str(value) == expected
    assert f"{value}" == expected
    assert int(cppyy.addressof(value)) == address

    value.__assign__(replacement)
    updated = str(probe.Reference[type(value)].text(value))
    assert str(value) == updated
    assert f"{value}" == updated
    assert int(cppyy.addressof(value)) == address
    print("NDS\tVALUE\t" + label + "\t" + updated, flush=True)


def check_stream_shifts():
    """Check native datatype insertion and extraction through Python streams."""
    source = dt.sc_biguint[257]("123456789")
    source_address = int(cppyy.addressof(source))

    output = cppyy.gbl.std.ostringstream()
    returned = output.__lshift__(source)
    assert returned is output
    spelling = str(output.str())
    assert spelling == str(probe.Reference[type(source)].text(source))
    assert int(cppyy.addressof(source)) == source_address

    target = dt.sc_biguint[257](0)
    input_stream = cppyy.gbl.std.istringstream(spelling)
    returned = input_stream.__rshift__(target)
    assert returned is input_stream
    assert str(probe.Reference[type(target)].text(target)) == spelling
    assert int(cppyy.addressof(source)) == source_address
    print("NDS\tSHIFT\t" + spelling)


def check_concat_reference():
    """Compare a live native concatenation reference with its source owners."""
    high = dt.sc_uint[9](0x12A)
    low = dt.sc_uint[8](0x6D)
    high_address = int(cppyy.addressof(high))
    low_address = int(cppyy.addressof(low))
    joined = dt.concat(high, low)
    reference = str(probe.Reference[type(joined)].text(joined))
    assert str(joined) == reference
    assert f"{joined}" == reference

    high.__assign__(0x155)
    changed = str(probe.Reference[type(joined)].text(joined))
    assert str(joined) == changed
    assert f"{joined}" == changed
    assert int(cppyy.addressof(high)) == high_address
    assert int(cppyy.addressof(low)) == low_address
    print("NDS\tCONCAT\t" + reference + "\t" + changed)


wide_unsigned = dt.sc_biguint[257]("123456789")
wide_signed = dt.sc_bigint[257]("-123456789")
check_string("biguint257", wide_unsigned, "987654321")
check_string("bigint257", wide_signed, "-987654321")

dynamic_unsigned = dt.sc_unsigned(257)
dynamic_unsigned.__assign__("123456789")
dynamic_signed = dt.sc_signed(257)
dynamic_signed.__assign__("-123456789")
check_string("unsigned257", dynamic_unsigned, "987654321")
check_string("signed257", dynamic_signed, "-987654321")

check_string("uint37", dt.sc_uint[37](123456789), 987654321)
check_string("int37", dt.sc_int[37](-123456789), -987654321)

check_string("fixed137_65", dt.sc_fixed[137, 65]("12345.125"), "98765.375")
check_string("ufixed137_65", dt.sc_ufixed[137, 65]("12345.125"), "98765.375")
check_string("fixed_fast137_65", dt.sc_fixed_fast[137, 65]("12345.125"), "98765.375")
check_string("ufixed_fast137_65", dt.sc_ufixed_fast[137, 65]("12345.125"), "98765.375")

check_string("fxval", dt.sc_fxval("12345.125"), "98765.375")
check_string("fxval_fast", dt.sc_fxval_fast("12345.125"), "98765.375")
check_string("fxnum", dt.sc_fixed[137, 65]("12345.125"), "98765.375")
check_string("fxnum_fast", dt.sc_fixed_fast[137, 65]("12345.125"), "98765.375")

fixed_parameters = dt.sc_fxtype_params(137, 65, dt.SC_TRN, dt.SC_WRAP)
fixed_cast = dt.sc_fxcast_switch(dt.SC_OFF)
check_string(
    "fxnum_base",
    dt.sc_fxnum("12345.125", fixed_parameters, dt.SC_TC_, fixed_cast, cppyy.nullptr),
    "98765.375",
)
check_string(
    "fxnum_fast_base",
    dt.sc_fxnum_fast("12345.125", fixed_parameters, dt.SC_TC_, fixed_cast, cppyy.nullptr),
    "98765.375",
)

check_stream_shifts()
check_concat_reference()
print("NATIVE_DATATYPE_STRING_OK", flush=True)
