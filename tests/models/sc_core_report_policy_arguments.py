"""Reject malformed report-policy calls without disturbing seeded state."""

from amal.eda.systemc import sc_core as sc


handler = sc.sc_report_handler
seed_type = "policy/arguments/seed"
recovery_type = "policy/arguments/recovery"
records = []


def capture(report, actions):
    """Copy callback fields while the borrowed native report remains valid."""
    records.append(
        (
            report.get_msg_type(),
            report.get_msg(),
            int(report.get_severity()),
            report.get_verbosity(),
            int(actions),
        )
    )


def read_actions(*policy):
    """Read one action policy through its setter, then immediately restore it."""
    previous = handler.set_actions(*policy, sc.SC_UNSPECIFIED)
    handler.set_actions(*policy, previous)
    return int(previous)


def read_limit(*policy):
    """Read one report limit through its reset form, then restore it."""
    previous = handler.stop_after(*policy, -1)
    handler.stop_after(*policy, previous)
    return int(previous)


def read_suppression_mask():
    """Read the suppression mask and undo the native getter's reset side effect."""
    previous = handler.suppress()
    handler.suppress(previous)
    return int(previous)


def read_forced_mask():
    """Read the forced-action mask and restore it after the native getter resets it."""
    previous = handler.force()
    handler.force(previous)
    return int(previous)


def cached_fields():
    """Copy the current cached report fields, or return None when it is empty."""
    report = handler.get_cached_report()
    if not report:
        return None
    return (
        report.get_msg_type(),
        report.get_msg(),
        int(report.get_severity()),
        report.get_file_name(),
        report.get_line_number(),
        report.get_verbosity(),
    )


def policy_state():
    """Snapshot all seeded policy, counter, cache and callback evidence."""
    return (
        read_actions(sc.SC_WARNING),
        read_actions(seed_type),
        read_actions(seed_type, sc.SC_WARNING),
        read_limit(sc.SC_WARNING),
        read_limit(seed_type),
        read_limit(seed_type, sc.SC_WARNING),
        read_suppression_mask(),
        read_forced_mask(),
        handler.get_verbosity_level(),
        int(handler.get_catch_actions()),
        (
            handler.get_count(sc.SC_WARNING),
            handler.get_count(seed_type),
            handler.get_count(seed_type, sc.SC_WARNING),
            handler.get_count(recovery_type),
        ),
        cached_fields(),
        tuple(records),
    )


def expect_rejection(label, expected, invoke):
    """Require a malformed call to raise and preserve every seeded state value."""
    before = policy_state()
    expected_types = expected if isinstance(expected, tuple) else (expected,)
    try:
        invoke()
    except expected as error:
        assert type(error) in expected_types, (label, type(error).__name__, expected_types)
    except Exception as error:
        raise AssertionError(f"{label} raised {type(error).__name__}, expected {expected}") from error
    else:
        raise AssertionError(f"{label} was accepted")
    after = policy_state()
    assert after == before, (label, before, after)


def check_mask_roundtrip(label, setter, getter, value):
    """Verify an accepted mask value round-trips and restore the seeded setting."""
    previous = setter(value)
    assert int(getter()) == value, (label, value, getter())
    replaced = setter(previous)
    assert int(replaced) == value, (label, value, replaced)
    assert int(getter()) == int(previous), (label, previous, getter())


# Give each policy level a recognizable value, then cache one report before
# installing the callback used to detect accidental dispatch during rejection.
old_severity_actions = handler.set_actions(sc.SC_WARNING, sc.SC_CACHE_REPORT)
old_message_actions = handler.set_actions(seed_type, sc.SC_CACHE_REPORT)
old_specific_actions = handler.set_actions(seed_type, sc.SC_WARNING, sc.SC_CACHE_REPORT | sc.SC_DO_NOTHING)
old_severity_limit = handler.stop_after(sc.SC_WARNING, 17)
old_message_limit = handler.stop_after(seed_type, 19)
old_specific_limit = handler.stop_after(seed_type, sc.SC_WARNING, 23)
old_verbosity = handler.set_verbosity_level(37)
old_suppress = handler.suppress(sc.SC_DISPLAY)
old_force = handler.force(sc.SC_LOG)
old_catch_actions = handler.set_catch_actions(sc.SC_DO_NOTHING)
handler.report(sc.SC_WARNING, seed_type, "retained seed", "policy.cpp", 9)
assert cached_fields() == (
    seed_type,
    "retained seed",
    int(sc.SC_WARNING),
    "policy.cpp",
    9,
    sc.SC_MEDIUM,
)
sc.set_report_handler(capture)

# These values are valid native unsigned masks even when they do not name a
# standard action. No report is emitted while the unusual bits are installed.
mask_values = (True, (1 << 32) - 1, 1 << 31)
accepted_state = policy_state()
check_mask_roundtrip(
    "severity_enum_action_mask",
    lambda value: handler.set_actions(sc.SC_WARNING, value),
    lambda: read_actions(sc.SC_WARNING),
    sc.SC_ERROR,
)
for mask_value in mask_values:
    check_mask_roundtrip(
        "severity_actions",
        lambda value: handler.set_actions(sc.SC_WARNING, value),
        lambda: read_actions(sc.SC_WARNING),
        mask_value,
    )
    check_mask_roundtrip(
        "message_actions",
        lambda value: handler.set_actions(seed_type, value),
        lambda: read_actions(seed_type),
        mask_value,
    )
    check_mask_roundtrip(
        "specific_actions",
        lambda value: handler.set_actions(seed_type, sc.SC_WARNING, value),
        lambda: read_actions(seed_type, sc.SC_WARNING),
        mask_value,
    )
    check_mask_roundtrip("suppress", handler.suppress, read_suppression_mask, mask_value)
    check_mask_roundtrip("force", handler.force, read_forced_mask, mask_value)
    check_mask_roundtrip("catch_actions", handler.set_catch_actions, handler.get_catch_actions, mask_value)
assert policy_state() == accepted_state

overflow_int = 1 << 31
underflow_int = -(1 << 31) - 1
overflow_unsigned = 1 << 32

rejections = (
    ("set_actions_zero_args", TypeError, lambda: handler.set_actions()),
    (
        "set_actions_too_many_args",
        TypeError,
        lambda: handler.set_actions(seed_type, sc.SC_WARNING, sc.SC_DO_NOTHING, 1),
    ),
    ("set_actions_severity_object", TypeError, lambda: handler.set_actions(object(), sc.SC_DO_NOTHING)),
    ("set_actions_message_object", TypeError, lambda: handler.set_actions(object(), sc.SC_DO_NOTHING)),
    ("set_actions_severity_action_object", TypeError, lambda: handler.set_actions(sc.SC_WARNING, object())),
    ("set_actions_message_action_object", TypeError, lambda: handler.set_actions(seed_type, object())),
    (
        "set_actions_specific_severity_object",
        TypeError,
        lambda: handler.set_actions(seed_type, object(), sc.SC_DO_NOTHING),
    ),
    (
        "set_actions_specific_message_object",
        TypeError,
        lambda: handler.set_actions(object(), sc.SC_WARNING, sc.SC_DO_NOTHING),
    ),
    (
        "set_actions_specific_action_object",
        TypeError,
        lambda: handler.set_actions(seed_type, sc.SC_WARNING, object()),
    ),
    (
        "set_actions_severity_unsigned_overflow",
        OverflowError,
        lambda: handler.set_actions(sc.SC_WARNING, overflow_unsigned),
    ),
    (
        "set_actions_message_unsigned_overflow",
        OverflowError,
        lambda: handler.set_actions(seed_type, overflow_unsigned),
    ),
    (
        "set_actions_specific_unsigned_overflow",
        OverflowError,
        lambda: handler.set_actions(seed_type, sc.SC_WARNING, overflow_unsigned),
    ),
    (
        "set_actions_severity_unsigned_underflow",
        OverflowError,
        lambda: handler.set_actions(sc.SC_WARNING, -1),
    ),
    (
        "set_actions_message_unsigned_underflow",
        OverflowError,
        lambda: handler.set_actions(seed_type, -1),
    ),
    (
        "set_actions_specific_unsigned_underflow",
        OverflowError,
        lambda: handler.set_actions(seed_type, sc.SC_WARNING, -1),
    ),
    ("set_verbosity_zero_args", TypeError, lambda: handler.set_verbosity_level()),
    (
        "set_verbosity_too_many_args",
        TypeError,
        lambda: handler.set_verbosity_level(sc.SC_LOW, sc.SC_HIGH),
    ),
    ("set_verbosity_object", TypeError, lambda: handler.set_verbosity_level(object())),
    (
        "set_verbosity_signed_overflow",
        ValueError,
        lambda: handler.set_verbosity_level(overflow_int),
    ),
    (
        "set_verbosity_signed_underflow",
        ValueError,
        lambda: handler.set_verbosity_level(underflow_int),
    ),
    ("get_verbosity_extra_arg", TypeError, lambda: handler.get_verbosity_level(object())),
    ("get_verbosity_one_arg", TypeError, lambda: handler.get_verbosity_level(0)),
    ("suppress_too_many_args", TypeError, lambda: handler.suppress(sc.SC_DISPLAY, sc.SC_LOG)),
    ("suppress_object", TypeError, lambda: handler.suppress(object())),
    (
        "suppress_unsigned_overflow",
        OverflowError,
        lambda: handler.suppress(overflow_unsigned),
    ),
    ("suppress_unsigned_underflow", OverflowError, lambda: handler.suppress(-1)),
    ("force_too_many_args", TypeError, lambda: handler.force(sc.SC_DISPLAY, sc.SC_LOG)),
    ("force_object", TypeError, lambda: handler.force(object())),
    ("force_unsigned_overflow", OverflowError, lambda: handler.force(overflow_unsigned)),
    ("force_unsigned_underflow", OverflowError, lambda: handler.force(-1)),
    ("set_catch_actions_zero_args", TypeError, lambda: handler.set_catch_actions()),
    (
        "set_catch_actions_too_many_args",
        TypeError,
        lambda: handler.set_catch_actions(sc.SC_DO_NOTHING, sc.SC_LOG),
    ),
    ("set_catch_actions_object", TypeError, lambda: handler.set_catch_actions(object())),
    (
        "set_catch_actions_unsigned_overflow",
        OverflowError,
        lambda: handler.set_catch_actions(overflow_unsigned),
    ),
    (
        "set_catch_actions_unsigned_underflow",
        OverflowError,
        lambda: handler.set_catch_actions(-1),
    ),
    ("get_catch_actions_arg", TypeError, lambda: handler.get_catch_actions(object())),
    ("get_count_zero_args", TypeError, lambda: handler.get_count()),
    (
        "get_count_too_many_args",
        TypeError,
        lambda: handler.get_count(seed_type, sc.SC_WARNING, 1),
    ),
    ("get_count_subject_object", TypeError, lambda: handler.get_count(object())),
    (
        "get_count_message_severity_object",
        TypeError,
        lambda: handler.get_count(seed_type, object()),
    ),
    (
        "get_count_message_object_with_severity",
        TypeError,
        lambda: handler.get_count(object(), sc.SC_WARNING),
    ),
)

for label, expected, call in rejections:
    expect_rejection(label, expected, call)

before_recovery = policy_state()
handler.set_actions(recovery_type, sc.SC_WARNING, sc.SC_CACHE_REPORT | sc.SC_DO_NOTHING)
handler.report(sc.SC_WARNING, recovery_type, "recovered", "policy.cpp", 41)
assert len(records) == 1, records
assert records[0][:4] == (
    recovery_type,
    "recovered",
    int(sc.SC_WARNING),
    sc.SC_MEDIUM,
), records[0]
after_recovery = policy_state()
assert after_recovery[10] == (
    before_recovery[10][0] + 1,
    before_recovery[10][1],
    before_recovery[10][2],
    before_recovery[10][3] + 1,
), (before_recovery[10], after_recovery[10])
assert after_recovery[:10] == before_recovery[:10]
assert after_recovery[12] == before_recovery[12] + (records[0],)

sc.set_report_handler()
handler.set_actions(sc.SC_WARNING, old_severity_actions)
handler.set_actions(seed_type, old_message_actions)
handler.set_actions(seed_type, sc.SC_WARNING, old_specific_actions)
handler.stop_after(sc.SC_WARNING, old_severity_limit)
handler.stop_after(seed_type, old_message_limit)
handler.stop_after(seed_type, sc.SC_WARNING, old_specific_limit)
handler.suppress(old_suppress)
handler.force(old_force)
handler.set_verbosity_level(old_verbosity)
handler.set_catch_actions(old_catch_actions)
print(f"REPORT_POLICY_ARGUMENT_REJECTIONS {len(rejections)}")
print("REPORT_POLICY_ARGUMENT_RECOVERY", records[0][0], handler.get_count(recovery_type))
