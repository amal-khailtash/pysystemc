"""Verify signed remainder mapping, native signs and wide arithmetic values."""

from amal.eda.systemc import sc_dt

import cppyy

cppyy.cppdef(
    """
    namespace signed_remainder_audit {
    std::string remainder(const char* av, const char* bv, bool inplace) {
        sc_dt::sc_signed a(137), b(91);
        a = av;
        b = bv;
        if (inplace) {
            a %= b;
            return a.to_string(sc_dt::SC_DEC);
        }
        sc_dt::sc_signed result = a % b;
        return result.to_string(sc_dt::SC_DEC);
    }
    }
    """
)

left = sc_dt.sc_signed(137)
right = sc_dt.sc_signed(91)
left.__assign__(100)
right.__assign__(7)
assert (left % right).to_int() == 2

for numerator, denominator in [
    (100, 7),
    (123456789, 100003),
    (2**80 + 12345, 2**45 + 7),
    (-100, 7),
    (100, -7),
    (-100, -7),
    (0, 7),
]:
    expected = (abs(numerator) % abs(denominator)) * (-1 if numerator < 0 else 1)
    left.__assign__(str(numerator))
    right.__assign__(str(denominator))
    assert (left % right).to_string(sc_dt.SC_DEC) == str(expected)
    for inplace in (False, True):
        result = cppyy.gbl.signed_remainder_audit.remainder(str(numerator), str(denominator), inplace)
        assert str(result) == str(expected), (numerator, denominator, inplace, result)
print("NATIVE_SIGNED_REMAINDER_AUDIT_OK")
