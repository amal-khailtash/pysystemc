"""Exercise native-free SystemC C++ CLI startup and error handling."""

import builtins
from collections.abc import Callable, Sequence
from types import ModuleType

import pytest

from amal.eda.cli.cpp import main


def _patch_runner_import(
    monkeypatch: pytest.MonkeyPatch,
    *,
    runner: Callable[[list[str], list[str], list[str]], int] | None = None,
    failure: BaseException | None = None,
    attempts: list[str] | None = None,
) -> None:
    """Replace only the lazy native runner import with a controlled module.

    Args:
        monkeypatch: Pytest fixture restoring the import hook after each test.
        runner: Optional fake ``run_sources`` implementation to expose.
        failure: Optional exception raised when startup imports the runner.
        attempts: Optional list receiving every intercepted runner import.
    """
    original_import = builtins.__import__

    def importing(name, globals=None, locals=None, fromlist=(), level=0):
        """Intercept the runner import and delegate every other import."""
        if name == "amal.eda.systemc._runner":
            if attempts is not None:
                attempts.append(name)
            if failure is not None:
                raise failure
            module = ModuleType(name)
            module.run_sources = runner
            return module
        return original_import(name, globals, locals, fromlist, level)

    monkeypatch.setattr(builtins, "__import__", importing)


@pytest.mark.parametrize(
    "failure",
    [ImportError("cppyy is unavailable"), RuntimeError("native setup failed")],
    ids=("import-error", "initialization-error"),
)
def test_startup_failures_use_one_formatted_diagnostic(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    failure: Exception,
) -> None:
    """Format runner import and native initialization errors without tracebacks."""
    _patch_runner_import(monkeypatch, failure=failure)

    assert main(["run", "model.cpp"]) == 1

    captured = capsys.readouterr()
    assert captured.out == ""
    assert captured.err == f"systemc-cpp: {failure}\n"
    assert "Traceback" not in captured.err
    assert len(captured.err.splitlines()) == 1


def test_runner_exception_keeps_existing_error_contract(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Format ordinary exceptions raised after native runner startup."""

    def run_sources(sources: list[str], arguments: list[str], includes: list[str]) -> int:
        """Raise a representative runner error after receiving parsed values."""
        del sources, arguments, includes
        raise ValueError("compiler invocation failed")

    _patch_runner_import(monkeypatch, runner=run_sources)

    assert main(["run", "model.cpp"]) == 1
    captured = capsys.readouterr()
    assert captured.out == ""
    assert captured.err == "systemc-cpp: compiler invocation failed\n"
    assert "Traceback" not in captured.err


def test_source_include_and_model_arguments_are_forwarded_unchanged(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Preserve source ordering, include paths, and arguments after `--`."""
    received: list[tuple[list[str], list[str], list[str]]] = []

    def run_sources(sources: list[str], arguments: list[str], includes: list[str]) -> int:
        """Record exact runner inputs and return a representative native status."""
        received.append((sources, arguments, includes))
        return 17

    _patch_runner_import(monkeypatch, runner=run_sources)
    argv = [
        "-I",
        "include/one",
        "-Iinclude/two",
        "model.cpp",
        "helper.cpp",
        "--",
        "model-argument",
        "-I",
        "native/include",
    ]

    assert main(argv) == 17
    assert received == [
        (
            ["model.cpp", "helper.cpp"],
            ["model-argument", "-I", "native/include"],
            ["include/one", "include/two"],
        )
    ]


@pytest.mark.parametrize("phase", ("import", "runner"))
def test_keyboard_interrupt_propagates(monkeypatch: pytest.MonkeyPatch, phase: str) -> None:
    """Let KeyboardInterrupt escape both runner startup and runner execution."""
    if phase == "import":
        _patch_runner_import(monkeypatch, failure=KeyboardInterrupt())
    else:

        def run_sources(sources: list[str], arguments: list[str], includes: list[str]) -> int:
            """Raise interruption from the simulated native-runner call."""
            del sources, arguments, includes
            raise KeyboardInterrupt

        _patch_runner_import(monkeypatch, runner=run_sources)

    with pytest.raises(KeyboardInterrupt):
        main(["run", "model.cpp"])


@pytest.mark.parametrize(
    "argv",
    ([], ["--help"], ["run", "--help"], ["version"], ["--version"], ["help"], ["help", "run"]),
)
def test_help_version_and_no_arguments_do_not_import_runtime(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    argv: Sequence[str],
) -> None:
    """Keep all informational paths independent from native runtime startup."""
    attempts: list[str] = []
    _patch_runner_import(monkeypatch, attempts=attempts)

    try:
        result = main(argv)
    except SystemExit as error:
        assert error.code == 0
    else:
        assert result == 0

    captured = capsys.readouterr()
    assert attempts == []
    assert captured.err == ""
    assert "systemc-cpp" in captured.out
