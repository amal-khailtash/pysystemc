"""Check fixed-point constructor errors and recovery through public adapters."""

from .support import run_example


def test_fixed_constructor_errors_keep_context_and_recover(tmp_path):
    """Check explicit shape wording, native candidates, and later native use."""
    output = run_example(tmp_path, "tests/models/fixed_error_presentation.py")
    records = [line for line in output.splitlines() if line.startswith("FEP\t")]

    assert records == [
        "FEP\texplicit_shape\tsc_fix\t(sc_fxtype_params, sc_enc)",
        "FEP\tnative_candidates\tsc_fixed<73,41>",
        "FEP\twide_recovery\t" + str((1 << 90) + 0x13579) + "\t1",
    ]
