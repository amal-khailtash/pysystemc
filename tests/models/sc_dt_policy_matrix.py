"""Emit exact Python observations for fixed-point policy semantics."""

from amal.eda.systemc import sc_dt as dt
import cppyy

FAMILIES = (
    ("sc_fix", dt.sc_fix, True),
    ("sc_ufix", dt.sc_ufix, False),
    ("sc_fix_fast", dt.sc_fix_fast, True),
    ("sc_ufix_fast", dt.sc_ufix_fast, False),
)
QUANTIZERS = (
    dt.SC_RND,
    dt.SC_RND_ZERO,
    dt.SC_RND_MIN_INF,
    dt.SC_RND_INF,
    dt.SC_RND_CONV,
    dt.SC_TRN,
    dt.SC_TRN_ZERO,
)
OVERFLOW_MODES = (dt.SC_SAT, dt.SC_SAT_ZERO, dt.SC_SAT_SYM, dt.SC_WRAP, dt.SC_WRAP_SM)
QUANTIZATION_CASES = (
    ("tie_pos_even", "1.03125"),
    ("tie_pos_odd", "1.09375"),
    ("tie_neg_even", "-1.03125"),
    ("tie_neg_odd", "-1.09375"),
    ("below_pos", "1.03124"),
    ("above_pos", "1.03126"),
    ("below_neg", "-1.03124"),
    ("above_neg", "-1.03126"),
)


def render(value):
    """Return exact decimal value, configured modes and cast status fields."""
    return "\t".join(
        map(
            str,
            (
                value.to_string(dt.SC_DEC),
                value.wl(),
                value.iwl(),
                int(value.q_mode()),
                int(value.o_mode()),
                value.n_bits(),
                int(value.cast_switch() == dt.SC_ON),
                int(value.quantization_flag()),
                int(value.overflow_flag()),
            ),
        )
    )


def emit_value(family, q_mode, o_mode, n_bits, cast, label, value):
    """Print one deterministic policy assignment observation."""
    print(
        "F2\tvalue\t"
        + "\t".join(map(str, (family, int(q_mode), int(o_mode), n_bits, cast, label)))
        + "\t"
        + render(value)
    )


def assign_case(family, number_type, q_mode, o_mode, n_bits, cast, label, source):
    """Assign one exact decimal source to a fresh runtime-configured number."""
    cast_mode = dt.SC_ON if cast == 1 else dt.SC_OFF
    number = number_type("0", 8, 4, q_mode, o_mode, n_bits, dt.sc_fxcast_switch(cast_mode))
    number.__assign__(source)
    emit_value(family, q_mode, o_mode, n_bits, cast, label, number)


handler = cppyy.gbl.sc_core.sc_report_handler
old_actions = handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, cppyy.gbl.sc_core.SC_THROW)
try:
    for family, number_type, signed in FAMILIES:
        for q_mode in QUANTIZERS:
            for o_mode in OVERFLOW_MODES:
                if not signed and o_mode == dt.SC_WRAP_SM:
                    rejected = False
                    try:
                        number_type("0", 8, 4, q_mode, o_mode, 2)
                    except cppyy.gbl.sc_core.sc_report:
                        rejected = True
                    assert rejected, f"{family} unexpectedly accepted SC_WRAP_SM"
                    print(
                        "F2\treject\t"
                        + "\t".join(
                            map(
                                str,
                                (family, int(q_mode), int(o_mode), 2, "unsigned_wrap_sm", 1),
                            )
                        )
                    )
                    continue

                for label, source in QUANTIZATION_CASES:
                    assign_case(family, number_type, q_mode, o_mode, 2, 1, label, source)

                positive_limit = "8" if signed else "16"
                negative_limit = "-8.0625" if signed else "-0.0625"
                assign_case(family, number_type, q_mode, o_mode, 2, 1, "overflow_positive", positive_limit)
                assign_case(family, number_type, q_mode, o_mode, 2, 1, "overflow_negative", negative_limit)

                # Cast OFF preserves the unrepresentable source; SC_ON applies
                # both the configured quantizer and overflow policy.
                assign_case(family, number_type, q_mode, o_mode, 2, 1, "lossy_positive", "17.03125")
                assign_case(family, number_type, q_mode, o_mode, 2, 0, "lossy_positive", "17.03125")
                assign_case(family, number_type, q_mode, o_mode, 2, 1, "lossy_negative", "-9.03125")
                assign_case(family, number_type, q_mode, o_mode, 2, 0, "lossy_negative", "-9.03125")

                if o_mode in (dt.SC_WRAP, dt.SC_WRAP_SM):
                    probe = "11.0625" if signed else "19.0625"
                    assign_case(family, number_type, q_mode, o_mode, 0, 1, "n_bits_probe", probe)
                    n_bits = 3 if o_mode == dt.SC_WRAP_SM else 2
                    assign_case(family, number_type, q_mode, o_mode, n_bits, 1, "n_bits_probe", probe)
finally:
    handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, old_actions)

print("SC_DT_POLICY_MATRIX_OK")
