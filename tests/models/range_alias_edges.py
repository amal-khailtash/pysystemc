"""Compare legacy datatype aliases and fixed-range vector copies with native SystemC."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_concatref_probe.h"))
concat_probe = cppyy.gbl.concatref_probe
cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_fixed_subrefs_probe.h"))
fixed_probe = cppyy.gbl.sc_dt_fixed_subrefs_probe


def emit(*fields):
    """Emit one deterministic record for comparison with the native model."""
    print("RANGEALIAS\t" + "\t".join(map(str, fields)))


def fixed_alias_pair(family, owner, const_selector):
    """Read mutable and read-only aliases around one live-parent update.

    Args:
        family: Slow or fast fixed-point label for the record.
        owner: Arbitrary-width fixed-point parent retained through all reads.
        const_selector: Native probe returning an owned const view of owner.
    """
    mutable = owner.range(103, 96)
    readonly = const_selector(owner, 103, 96)
    emit(family, "mutable_before", mutable.length(), int(mutable.to_signed()), int(mutable.to_unsigned()))
    emit(family, "const_before", readonly.length(), int(readonly.to_signed()), int(readonly.to_unsigned()))
    owner.range(103, 96).__assign__(0x5A)
    emit(family, "mutable_after", mutable.length(), int(mutable.to_signed()), int(mutable.to_unsigned()))
    emit(family, "const_after", readonly.length(), int(readonly.to_signed()), int(readonly.to_unsigned()))


def fixed_aliases():
    """Cover mutable/read-only slow and fast aliases plus wide vector snapshots."""
    owner = dt.sc_fixed[137, 69](-13.375)
    fixed_alias_pair("fixed", owner, fixed_probe.fixed_const_range)
    fast = dt.sc_fixed_fast[137, 69](-13.375)
    fixed_alias_pair("fast", fast, fixed_probe.fast_const_range)

    whole = owner.range(136, 0)
    base_copy = dt.sc_bv_base(whole)
    template_copy = dt.sc_bv[137](whole)
    before = str(base_copy.to_string(dt.SC_BIN, True)), str(template_copy.to_string(dt.SC_BIN, True))
    whole.__assign__(0)
    after = str(base_copy.to_string(dt.SC_BIN, True)), str(template_copy.to_string(dt.SC_BIN, True))
    emit(
        "vector_copy",
        base_copy.length(),
        before[0],
        template_copy.length(),
        before[1],
        str(whole.to_string(dt.SC_BIN, True)),
        after[0],
        after[1],
    )


def integer_owner_aliases():
    """Read deprecated owner conversions at arbitrary native widths."""
    signed = dt.sc_signed(137)
    signed.__assign__("-81985529216486895")
    emit("signed_owner", "before", signed.length(), int(signed.to_signed()), int(signed.to_unsigned()))
    signed.__assign__("1234567890123456789012345")
    emit("signed_owner", "after", signed.length(), int(signed.to_signed()), int(signed.to_unsigned()))

    unsigned = dt.sc_unsigned(191)
    unsigned.__assign__("0x123456789abcdef001122334455667788")
    emit("unsigned_owner", "before", unsigned.length(), int(unsigned.to_signed()), int(unsigned.to_unsigned()))
    unsigned.__assign__("0xfedcba98765432100123456789abcdef")
    emit("unsigned_owner", "after", unsigned.length(), int(unsigned.to_signed()), int(unsigned.to_unsigned()))


def integer_reference_aliases():
    """Check bitwidth aliases on live bounded integer subrefs and bitrefs."""
    signed = dt.sc_int[37](-0x1234567)
    signed_range = signed(29, 17)
    signed_bit = signed[9]
    emit("int_range", "before", signed_range.bitwidth(), signed_range.length(), int(signed_range.to_int()))
    emit("int_bit", "before", signed_bit.bitwidth(), signed_bit.length(), int(signed_bit.to_bool()))
    signed.__assign__(0x123456789)
    emit("int_range", "after", signed_range.bitwidth(), signed_range.length(), int(signed_range.to_int()))
    emit("int_bit", "after", signed_bit.bitwidth(), signed_bit.length(), int(signed_bit.to_bool()))

    unsigned = dt.sc_uint[53](0x123456789ABCD)
    unsigned_range = unsigned(47, 31)
    unsigned_bit = unsigned[41]
    emit("uint_range", "before", unsigned_range.bitwidth(), unsigned_range.length(), int(unsigned_range.to_uint()))
    emit("uint_bit", "before", unsigned_bit.bitwidth(), unsigned_bit.length(), int(unsigned_bit.to_bool()))
    unsigned.__assign__(0x1ABCDE1234567)
    emit("uint_range", "after", unsigned_range.bitwidth(), unsigned_range.length(), int(unsigned_range.to_uint()))
    emit("uint_bit", "after", unsigned_bit.bitwidth(), unsigned_bit.length(), int(unsigned_bit.to_bool()))


def concat_alias():
    """Check concat width aliases while its borrowed parents remain live."""
    left = dt.sc_unsigned(137)
    left.__assign__("0x123456789abcdef")
    right = dt.sc_unsigned(191)
    right.__assign__("0xabcdef00112233445566778899aabbccddeeff")
    node = concat_probe.select(left, right)
    emit("concat", "before", node.bitwidth(), node.length(), str(node.to_string(dt.SC_HEX)))
    left.__assign__("0xfedcba987654321")
    right.__assign__("0x112233445566778899aabbccddeeff")
    emit("concat", "after", node.bitwidth(), node.length(), str(node.to_string(dt.SC_HEX)))


fixed_aliases()
integer_owner_aliases()
integer_reference_aliases()
concat_alias()
