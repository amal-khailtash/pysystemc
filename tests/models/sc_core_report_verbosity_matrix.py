"""Compare accepted native verbosity boundaries and severity filtering."""

from amal.eda.systemc import sc_core as sc

h = sc.sc_report_handler
levels = (-(1 << 31), -1, 0, 1, sc.SC_LOW, sc.SC_MEDIUM, sc.SC_HIGH, sc.SC_FULL, (1 << 31) - 1)
verbosity_values = levels
calls = []


def capture(report, actions):
    """Keep scalar dispatch evidence without retaining borrowed report storage."""
    calls.append((int(report.get_severity()), report.get_verbosity(), int(actions)))


sc.set_report_handler(capture)
previous_level = h.get_verbosity_level()
message = "verbosity/matrix"
h.set_actions(message, sc.SC_DO_NOTHING)
for level_index, level in enumerate(levels):
    old = h.set_verbosity_level(level)
    assert old == (previous_level if level_index == 0 else levels[level_index - 1])
    assert h.get_verbosity_level() == level
    for severity in (sc.SC_INFO, sc.SC_WARNING, sc.SC_ERROR, sc.SC_FATAL):
        for verbosity_index, verbosity in enumerate((None, *verbosity_values)):
            effective = sc.SC_MEDIUM if verbosity is None else verbosity
            before = (h.get_count(message), h.get_count(message, severity), h.get_count(severity), len(calls))
            if verbosity is None:
                h.report(severity, message, "boundary", "verbosity_matrix.cpp", 21)
            else:
                h.report(severity, message, "boundary", verbosity, "verbosity_matrix.cpp", 21)
            after = (h.get_count(message), h.get_count(message, severity), h.get_count(severity), len(calls))
            delta = tuple(end - start for start, end in zip(before, after, strict=True))
            expected = int(severity != sc.SC_INFO or effective <= level)
            assert delta == (expected,) * 4, (level, severity, verbosity, delta)
            if expected:
                assert calls[-1] == (int(severity), effective, int(sc.SC_DO_NOTHING))
            print("REPORT_VERBOSITY", level_index, level, int(severity), verbosity_index, effective, *delta)
assert h.set_verbosity_level(previous_level) == levels[-1]
sc.set_report_handler()
print("REPORT_VERBOSITY_OK cases=360 restored=1")
