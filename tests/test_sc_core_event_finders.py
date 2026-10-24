"""Compare Python event finder behavior with an independent native model."""

import os
import signal
import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_event_finders(tmp_path):
    """Verify wide-interface lookup, cached finders, reports, and native lifetimes."""
    output = run_example(tmp_path, "tests/models/sc_core_event_finders.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_event_finders.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual_reference = [line for line in output.splitlines() if line.startswith("FINDER_REFERENCE")]
    expected_reference = [line for line in native.stdout.splitlines() if line.startswith("FINDER_REFERENCE")]
    assert len(actual_reference) == 1 and actual_reference == expected_reference

    actual_reports = [line for line in output.splitlines() if line.startswith("FINDER_REPORT")]
    expected_reports = [line for line in native.stdout.splitlines() if line.startswith("FINDER_REPORT")]
    assert len(actual_reports) == 2 and actual_reports == expected_reports

    for marker in (
        "FINDER_CACHE_REFERENCE",
        "FINDER_CONTINUATION",
        "FINDER_PROBE_REPORT",
        "FINDER_PROBE_LIFETIME",
        "FINDER_SPECIALIZATION_REFERENCE",
        "FINDER_OVERLOADED_METHODS",
    ):
        actual = [line for line in output.splitlines() if line.startswith(marker)]
        expected = [line for line in native.stdout.splitlines() if line.startswith(marker)]
        assert actual == expected, (marker, actual, expected)

    native_lifetime = [line for line in native.stdout.splitlines() if line.startswith("FINDER_NATIVE_LIFETIME")]
    assert native_lifetime == ["FINDER_NATIVE_LIFETIME\t1\t1"]
    assert [line for line in output.splitlines() if line.startswith("FINDER_FACTORY_CONTRACT")] == [
        "FINDER_FACTORY_CONTRACT\t1\t1\t1\t1"
    ]


def test_sc_core_finder_cache_rejects_cross_port_reuse(tmp_path):
    """Compare debug assertion and NDEBUG cache behavior in fresh processes."""
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    results = []
    for label, command in (
        (
            "python",
            [sys.executable, str(ROOT / "tests/models/sc_core_finder_cache_assert.py")],
        ),
        (
            "native",
            [
                sys.executable,
                str(ROOT / "examples/run_sysc_cpp.py"),
                str(ROOT / "tests/native/sc_core_finder_cache_assert.cpp"),
            ],
        ),
    ):
        directory = tmp_path / label
        directory.mkdir()
        result = subprocess.run(
            command,
            cwd=directory,
            capture_output=True,
            text=True,
            timeout=60,
            env=env,
        )
        transcript = result.stdout + result.stderr
        configuration = [
            line.split()[-1] for line in result.stdout.splitlines() if line.startswith("FINDER_CACHE_ASSERT_CONFIG")
        ]
        assert len(configuration) == 1, (label, result.stdout, result.stderr)
        assert "FINDER_CACHE_ASSERT_ENTER" in result.stdout
        assert "FINDER_CACHE_ASSERT_RETURNED" not in result.stdout
        results.append((label, result.returncode, configuration[0], transcript, result.stdout))

    assert results[0][2] == results[1][2], results
    for label, returncode, configuration, transcript, stdout in results:
        if configuration == "enabled":
            assert returncode in (-signal.SIGABRT, 256 - signal.SIGABRT), (label, returncode, transcript)
            assert "&port_ == &cache_p->port()" in transcript, transcript
            assert "FINDER_CACHE_ASSERT_RELEASE" not in stdout
        else:
            assert returncode == 0, (label, returncode, transcript)
            assert [line for line in stdout.splitlines() if line.startswith("FINDER_CACHE_ASSERT_RELEASE")] == [
                "FINDER_CACHE_ASSERT_RELEASE\t1\t1\t1"
            ]
            assert "&port_ == &cache_p->port()" not in transcript, transcript
