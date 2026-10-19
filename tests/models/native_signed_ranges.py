"""Preserve native signed-range anomalies as evidence for the API audit."""

from amal.eda.systemc import sc_dt

import cppyy

cppyy.cppdef(
    """
    namespace signed_range_audit {
    void check() {
        sc_dt::sc_signed parent(16);
        parent = 0x12;
        auto& reversed = parent.range(0, 3);
        if (reversed.length() != 4 || reversed.concat_length(nullptr) != -2)
            throw std::runtime_error("reversed range capacity differs");
        if (!reversed.and_reduce() || reversed.or_reduce() || reversed.xor_reduce())
            throw std::runtime_error("reversed reductions differ");
        reversed = 15;
        if (parent.to_int() != 0x12)
            throw std::runtime_error("reversed assignment differs");

        parent = 0;
        auto& destination = parent.range(7, 4);
        sc_dt::sc_unsigned source(1);
        source = 1;
        destination.concat_set(source, 0);
        if (parent.to_int() != 0x11)
            throw std::runtime_error("unsigned concat padding differs");
        parent = 0;
        destination.concat_set(sc_dt::uint64(1), 0);
        if (parent.to_int() != 0x10)
            throw std::runtime_error("uint64 concat value differs");
    }
    }
    """
)

cppyy.gbl.signed_range_audit.check()

parent = sc_dt.sc_signed(16)
parent.__assign__(0x12)
view = parent.range(0, 3)
assert view.length() == 4
assert view.and_reduce() and not view.or_reduce() and not view.xor_reduce()
view.__assign__(15)
assert parent.to_int() == 0x12
print("NATIVE_SIGNED_RANGES_AUDIT_OK")
