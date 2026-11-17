"""Exercise REPL loop edges in a fresh process with deterministic prompt input."""

import os
from pathlib import Path
import sys

from prompt_toolkit.auto_suggest import AutoSuggestFromHistory
from prompt_toolkit.history import InMemoryHistory

import amal.eda.systemc.repl as repl_module
from amal.eda.systemc.repl import Repl


class QueuedSession:
    """Prompt-session substitute that supplies queued lines and records state."""

    def __init__(self, values: list[str], **kwargs: object) -> None:
        """Store deterministic answers and the standard Prompt Toolkit options."""
        self.values = list(values)
        self.kwargs = kwargs
        self.prompts: list[str] = []
        self.buffers: list[list[str]] = []
        self.owner: Repl | None = None

    def prompt(self, message: str) -> str:
        """Return one queued command, recording its prompt and REPL buffer."""
        self.prompts.append(message)
        self.buffers.append(list(self.owner.console.buffer) if self.owner is not None else [])
        if not self.values:
            raise EOFError
        value = self.values.pop(0)
        if value == "<eof>":
            raise EOFError
        return value


def run_loop_edges() -> None:
    """Assert continuation, syntax recovery, interrupt recovery, and completion edges."""
    session: QueuedSession | None = None
    compile_interrupts = 0

    def interrupt_selected_compile(event: str, arguments: tuple[object, ...]) -> None:
        """Raise one interrupt only while compiling the designated queued command."""
        nonlocal compile_interrupts
        if event != "compile" or not arguments:
            return
        source = arguments[0]
        if isinstance(source, bytes):
            source = source.decode(errors="replace")
        if isinstance(source, str) and "compile_interrupt_marker" in source:
            compile_interrupts += 1
            raise KeyboardInterrupt("deterministic compile interruption")

    # This narrow audit hook lets the production InteractiveConsole receive a
    # KeyboardInterrupt from Python compilation without replacing its behavior.
    sys.addaudithook(interrupt_selected_compile)

    def session_factory(**kwargs: object) -> QueuedSession:
        nonlocal session
        session = QueuedSession(
            [
                "preserved_value = 41",
                "if True:",
                "    if True:\n",
                "        ???\n",
                "print('RECOVERED_AFTER_SYNTAX', preserved_value)",
                "if True:",
                "    if True:\n",
                "        raise KeyboardInterrupt\n\n",
                "if True:",
                "    compile_interrupt_marker = True\n",
                "print('RECOVERED_AFTER_INTERRUPT', preserved_value)",
            ],
            **kwargs,
        )
        return session

    repl = Repl(no_history=True, prompt_session_factory=session_factory)
    assert isinstance(repl.history, InMemoryHistory)
    assert isinstance(repl.session.kwargs["auto_suggest"], AutoSuggestFromHistory)
    assert session is not None
    session.owner = repl

    # Empty input has no identifier to complete and must yield no candidates.
    assert list(repl.completer.get_completions(repl_module.Document(""), repl_module.CompleteEvent())) == []

    repl.run()

    assert repl.namespace["preserved_value"] == 41
    assert session.prompts == [
        "pysystemc >>> ",
        "pysystemc >>> ",
        "... ",
        "... ",
        "pysystemc >>> ",
        "pysystemc >>> ",
        "... ",
        "... ",
        "pysystemc >>> ",
        "... ",
        "pysystemc >>> ",
        "pysystemc >>> ",
    ]
    assert session.buffers[3] == ["if True:", "    if True:"]
    assert session.buffers[4] == []
    assert session.buffers[9] == ["if True:"]
    assert session.buffers[10] == []
    assert compile_interrupts == 1


def run_main_system_exit(history_root: Path) -> None:
    """Exercise the public main entry point without reading a real terminal."""
    history_root.mkdir(parents=True, exist_ok=True)
    blocked_parent = history_root / "history-parent-is-a-file"
    blocked_parent.write_text("keep this temporary test file", encoding="utf-8")
    try:
        Repl(
            history_path=blocked_parent / "history",
            prompt_session_factory=lambda **kwargs: QueuedSession([], **kwargs),
        )
    except FileExistsError:
        pass
    else:
        raise AssertionError("a history path below a regular file must fail during setup")

    os.environ["XDG_STATE_HOME"] = str(history_root)
    sessions: list[QueuedSession] = []

    class MainSession(QueuedSession):
        """Queue a visible command, SystemExit, and a sentinel after the exit."""

        def __init__(self, **kwargs: object) -> None:
            """Initialize the default session created by ``Repl()`` in ``main``."""
            super().__init__(
                ["print('MAIN_ENTRY_REACHED')", "raise SystemExit(23)", "print('AFTER_SYSTEM_EXIT')"],
                **kwargs,
            )
            sessions.append(self)

    # This swaps only the line-editor construction boundary. main() creates and
    # runs the production Repl class with its real InteractiveConsole.
    repl_module.PromptSession = MainSession
    repl_module.main()

    assert len(sessions) == 1
    assert sessions[0].prompts == ["pysystemc >>> ", "pysystemc >>> "]
    assert len(sessions[0].values) == 1


if __name__ == "__main__":
    run_loop_edges()
    run_main_system_exit(Path(sys.argv[1]))
